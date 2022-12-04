#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define _GNU_SOURCE // for FNM_CASEFOLD, FNM_EXTMATCH
#include "dictionaries.h"
#include "dictzip.h"
#include "utils.h"
#include "cvec.h"
#include <fnmatch.h> //fnmatch
#include <fts.h>     //fts_open fts_read fts_close

inline void dictionary_open(Dictionary *interface) {
  interface->open(interface);
}

inline void dictionary_close(Dictionary *interface) {
  interface->close(interface);
}

void worddefinition_utf8_cleaning(WordDefinition *self) {
  int len =
      utf8proc_map((utf8proc_uint8_t *)self->word, -1,
                   (utf8proc_uint8_t **)&(self->word_utf_clean),
                   UTF8PROC_DECOMPOSE | UTF8PROC_NULLTERM | UTF8PROC_IGNORE |
                       UTF8PROC_CASEFOLD | UTF8PROC_STRIPMARK);
  if (len < 0) {
    self->word_utf_clean = NULL;
  }
  if (!strcmp(self->word, self->word_utf_clean)) {
    free(self->word);
    self->word = xstrdup("");
  }
}

void worddefinition_free(WordDefinition *self) {
  free(self->word);
  free(self->definition);
  free(self->word_utf_clean);
}

WordDefinition worddefinition_init() {
  WordDefinition result = {0};
  return result;
}

inline void dictionary_init(Dictionary *interface, char * basic_filename,
		     void (*save)(Dictionary *interface, FILE *out),
		     void (*open)(Dictionary *interface),
		     uint (*next_word)(Dictionary *interface, WordDefinition *result),
		     void (*close)(Dictionary *interface),
		     void (*free)(Dictionary *interface),
		     size_t (*definition)(Dictionary *interface, WordDefinition *result)) {
  interface->name = NULL;
  interface->to_index = 0;
  interface->words_file = NULL;
  interface->basic_filename=basic_filename;
  interface->save=save;
  interface->open=open;
  interface->next_word=next_word;
  interface->close=close;
  interface->free=free;
  interface->definition=definition;
}

size_t dictionary_get_definition(Dictionary *interface,
                                       WordDefinition *result) {
  dictData *dz = dict_data_open(interface->words_file, 0);
  if (dz) {
    result->definition =
        dict_data_read_(dz, result->offset, result->size, NULL, NULL);
    dict_data_close(dz);
    return 1;
  }
  result->definition = NULL;
  return 0;
}

void dictionary_free(Dictionary *interface) {
  free(interface->name);
  free(interface->words_file);
  free(interface->basic_filename);
}

char *append_extension(char *basename, const char *extension) {
  size_t len_base = strlen(basename);
  strcat(basename, extension);
  char *result = NULL;
  if (check_if_file_exist(basename)) {
    result = xstrdup(basename);
  };
  basename[len_base] = '\0';
  return result;
}

void dictionary_array_free(DictionaryArray self) {
  for (uint i = 0; i < VECTOR_SIZE(self.dictionary); i++) {
    self.dictionary[i]->free(self.dictionary[i]);
  };
  VECTOR_FREE(self.dictionary);
}

DictionaryArray dictionary_array_init(RealPaths realpath) {
  DictionaryArray result = {.dictionary = NULL};
  FTS *tree = fts_open(realpath.dirs, FTS_LOGICAL | FTS_NOSTAT, NULL);
  CHECK_PTR(tree);
  Dictionary *interface;
  FTSENT *f;
  while ((f = fts_read(tree))) {
    if (f->fts_info == FTS_F) {
      for (uint i = 0; i < ARRAY_LENGTH(supported_dictionary); i++) {
        if (fnmatch(supported_dictionary[i].extension, f->fts_name,
                    FNM_CASEFOLD | FNM_EXTMATCH) == 0) {
	  interface = supported_dictionary[i].create(f->fts_path);
	  VECTOR_PUSH(result.dictionary, interface);
        };
      };
    };
  };

  if (fts_close(tree) < 0)
    exit(EXIT_FAILURE);

  return result;
}


void dictionary_array_write(DictionaryArray *D, FILE *out) {
  uint32_t dN = VECTOR_SIZE(D->dictionary);
  write_long(dN, out);
  for (size_t i = 0; i < dN; i++) {
    Dictionary *p = D->dictionary[i];
    if (p->to_index) {
      write_string(out, p->basic_filename);
      p->save(p, out);
    }
  }
}

DictionaryArray dictionary_array_load(void *p) {
  DictionaryArray result={.dictionary =  NULL};
  Dictionary *interface;
  uint32_t dN = read_long_mm(&p);
  for (size_t j = 0; j < dN; j++) {
    const char *base =  read_string_mm(&p);
    for (uint i = 0; i < ARRAY_LENGTH(supported_dictionary); i++) {
      if (fnmatch(supported_dictionary[i].extension, base,
		  FNM_CASEFOLD | FNM_EXTMATCH) == 0) {
	interface = supported_dictionary[i].load(&p, base);
	VECTOR_PUSH(result.dictionary, interface);
      };
    };
  }
  return result;
}
