#include "../dictionaries.h"
#include "../utils.h"
#include "../cvec.h"
#include <stddef.h> //size_t
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

int UTF_BOM_SIZE = 3;

typedef struct {
  char **synonyms; //VECTOR
  char *new_word;
  size_t offset_new_word;
  size_t offset;
  size_t size;
} DslParsingState;

void dsl_clean_word(char *str) {
  char *src = str;
  char *dst = str;
  size_t opened = 0;
  for (char *s = src; *s && *s != '\n' && *s != '\r'; s++) {
    if (*s == '{')
      opened++;
    else if (*s == '}')
      opened -= opened ? 1 : 0;
    else if (!opened) {
      if (*s == '\\')
        s++;
      *dst++ = *s;
    }
  }
  *dst = '\0';
}

static void dsl_parsing_state_add_synonym(DslParsingState *parse,
                                          char *synonym) {
  dsl_clean_word(synonym);
  if (strlen(synonym) > 0)
    VECTOR_PUSH(parse->synonyms, synonym);
  else {
    free(synonym);
  }
}

static void dsl_parsing_state_remove_synonym(DslParsingState *parse,
                                             char **synonym) {
  *synonym=VECTOR_POP(parse->synonyms);
  if(VECTOR_SIZE(parse->synonyms)==0)
    VECTOR_FREE(parse->synonyms);
}

typedef struct {
  char *dsl; //file with words in  dsl format
  char *abrv;  //file with abbreviation
  char *ann;  //the dictionary annotation file
  char *bmp; //the dictionary pictogram
  char *abrv_ann; //annotation for abbreviation
  char *files; //file in zip format with all resources
  gzFile handle; 
  DslParsingState parse;
  Dictionary interface;
} Dsl;

static void dsl_cut_extension(char *basename) {
  size_t len = strlen(basename);
  if (basename[len - 4] == '.')
    basename[len - 4] = '\0';
  else
    basename[len - 7] = '\0';
}

static void dsl_dictionary_open(Dictionary *interface) {
  Dsl *dsl = CONTAINER_OF(interface, Dsl, interface);
  dsl->handle = gzopen(dsl->dsl, "rb");
  gzseek(dsl->handle, UTF_BOM_SIZE, SEEK_SET);
}

static inline bool is_string_empty(const char c) {
  return  '\n' == c ||  '\r' == c ||  '#' == c;
}

static inline bool is_definition(const  char c) { return '\t' == c || ' ' == c; }

static inline bool is_word(const  char c) {
  return !(is_string_empty(c) || is_definition(c));
}

static void dsl_return_synonyms(DslParsingState *parse,
                                WordDefinition *result) {
  dsl_parsing_state_remove_synonym(parse, &result->word);
  result->def.offset = (uint)parse->offset;
  result->def.size = (uint)parse->size;
}

bool dsl_dictionary_next_word(Dictionary *interface, WordDefinition *result) {
  Dsl *dsl = CONTAINER_OF(interface, Dsl, interface);
  gzFile handle = dsl->handle;
  char buf[CHAR_BUFFER_SIZE];
  long int gzp;
  if(!(gzp=gztell(handle)))
    return false;
  size_t position=(size_t)gzp;
  uint8_t state = 0;
  uint8_t CARD = 1;
  uint8_t DEFINITION = 1 << 1;
  DslParsingState *parse = &dsl->parse;
  if (VECTOR_SIZE(parse->synonyms) > 0) {
    dsl_return_synonyms(parse, result);
    return true;
  }

  if (parse->offset_new_word) {
    state |= CARD;
    parse->offset = parse->offset_new_word;
    parse->offset_new_word = 0;
    dsl_parsing_state_add_synonym(parse, parse->new_word);
  }

  while (gzgets(handle, buf, CHAR_BUFFER_SIZE)) {
    if (is_definition(buf[0]))
      state |= DEFINITION;

    if (is_string_empty(buf[0])) {
      if (state & CARD) {
        parse->size = position - parse->offset;
        return dsl_dictionary_next_word(interface, result);
      }
      state = 0;
    }
    if (is_word(buf[0])) {
      char *word=xstrdup(buf);
      if (state & CARD) {
        if (!(state & DEFINITION)) {
          dsl_parsing_state_add_synonym(parse, word);
        } else {
          parse->offset_new_word = position;
          parse->size = position - parse->offset;
	  parse->new_word=word;
          return dsl_dictionary_next_word(interface, result);
        }
      } else {
        dsl_parsing_state_add_synonym(parse, word);
        state |= CARD;
        parse->offset = position;
      }
    }
    if(!(gzp=gztell(handle)))
      return false;
    position=(size_t)gzp;
  }

  if (VECTOR_SIZE(parse->synonyms) > 0) {
    dsl_return_synonyms(parse, result);
    return true;
  }
  return false;
}

void dsl_dictionary_close(Dictionary *interface) {
  Dsl *dsl = CONTAINER_OF(interface, Dsl, interface);
  gzclose(dsl->handle);
}
void dsl_dictionary_free(Dictionary *interface) {
  dictionary_free(interface);
  Dsl *dsl = CONTAINER_OF(interface, Dsl, interface);
  free(dsl->abrv);
  free(dsl->ann);
  free(dsl->bmp);
  free(dsl->abrv_ann);
  free(dsl->files);
  free(dsl);
}

void dsl_get_name(Dsl *dsl) {
  char buf[CHAR_BUFFER_SIZE];
  char *subString;
  while (gzgets(dsl->handle, buf, CHAR_BUFFER_SIZE) != NULL && buf[0] == '#') {
    subString = strtok(buf, "\"");
    subString = strtok(NULL, "\"");
    if (strstr(buf, "NAME")) {
      dsl->interface.name = xstrdup(subString);
      break;
    }
  }
}

// The encoding length should be at least 13.
uint dsl_check_encoding(const char *filename, char *encoding) {
  gzFile file = gzopen(filename, "rb");
  unsigned char firstBytes[2];
  if (gzread(file, firstBytes, sizeof(firstBytes)) != sizeof(firstBytes)) {
    // Apparently the file's too short
    encoding[0] = 0;
    return 0;
  }
  // If the file begins with the dedicated Unicode marker, we just consume
  // it. If, on the other hand, it's not, we return the bytes back

  if (firstBytes[0] == 0xFF && firstBytes[1] == 0xFE)
    strcpy(encoding, "UTF-16LE");
  else if (firstBytes[0] == 0xFE && firstBytes[1] == 0xFF)
    strcpy(encoding, "UTF-16BE");
  else if (firstBytes[0] == 0xEF && firstBytes[1] == 0xBB) {
    // Looks like Utf8, read one more byte
    if (gzread(file, firstBytes, 1) != 1 || firstBytes[0] != 0xBF) {
      // Either the file's too short, or the BOM is weird
      encoding[0] = 0;
      gzclose(file);
      return 0;
    }
    strcpy(encoding, "UTF-8");
  } else {
    if (firstBytes[0] && !firstBytes[1])
      strcpy(encoding, "UTF-16LE");
    else if (!firstBytes[0] && firstBytes[1])
      strcpy(encoding, "UTF-16BE");
    else
      strcpy(encoding, "WINDOWS-1251");
  }
  gzclose(file);
  return 1;
}


void dsl_save(Dictionary *interface, FILE *out) {
  Dsl *dsl = CONTAINER_OF(interface, Dsl, interface);
  write_short(interface->id, out);
  char *list[] = {interface->name, dsl->dsl, dsl->ann, dsl->abrv, dsl->abrv_ann, dsl->bmp, dsl->files};
  write_strings(out, ARRAY_LENGTH(list), list);
}


Dictionary *dsl_create(const char *filename) {
  Dsl *dsl;
  dsl = xmalloc(sizeof *dsl);
  Dictionary *interface = &dsl->interface;
  dictionary_init(interface, xstrdup(filename), dsl_save, dsl_dictionary_open,
                  dsl_dictionary_next_word, dsl_dictionary_close,
                  dsl_dictionary_free, dictionary_get_definition);

  dsl->dsl = xstrdup(filename);
  interface->words_file = dsl->dsl;

  dsl->parse.synonyms = NULL;
  dsl->parse.offset = 0;
  dsl->parse.size = 0;
  dsl->parse.offset_new_word = 0;

  char basename[strlen(filename) + 20];
  strcpy(basename, filename);
  dsl_cut_extension(basename);

  dsl->ann = append_extension(basename, ".ann");
  dsl->bmp = append_extension(basename, ".bmp");
  dsl->abrv_ann = append_extension(basename, "_abrv.ann");
  char *abrv_dsl = append_extension(basename, "_abrv.dsl");
  dsl->abrv = abrv_dsl ? abrv_dsl : append_extension(basename, "_abrv.dsl.dz");
  char *files = append_extension(basename, ".dsl.dz.files.zip");
  char *files2 = files ? files : append_extension(basename, ".dsl.files.zip");
  dsl->files = files2 ? files2 : append_extension(basename, ".files.zip");

  char encoding[15];
  dsl_check_encoding(dsl->dsl, encoding);
  if (strcmp(encoding, "UTF-8") == 0) {
    interface->to_index = true;
    dictionary_open(interface);
    dsl_get_name(dsl);
    dictionary_close(interface);
  }

  return interface;
}



Dictionary *dsl_load(void **p, const char *filename) {
  Dsl *dsl;
  dsl = xmalloc(sizeof *dsl);
  Dictionary *interface = &dsl->interface;
  dictionary_init(interface, xstrdup(filename), dsl_save, dsl_dictionary_open,
                  dsl_dictionary_next_word, dsl_dictionary_close,
                  dsl_dictionary_free, dictionary_get_definition);

  interface->id=read_short_mm(p);
  interface->to_index = true;
  char** const list[] = {&interface->name, &dsl->dsl, &dsl->ann, &dsl->abrv, &dsl->abrv_ann, &dsl->bmp, &dsl->files};
  read_strings_mm(p, ARRAY_LENGTH(list), list);

  interface->words_file = dsl->dsl;

  
  return interface;
}


