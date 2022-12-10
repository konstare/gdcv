#include "../dictionaries.h"
#include "../utf8proc/utf8proc.h"
#include "../utils.h"
#include <stdbool.h>
#include <stddef.h> //size_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  gzFile handle;
  char *dict;
  char *index;
  Dictionary interface;
} Dictd;

uint decodeBase64(const char *str) {
  char const digits[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  ptrdiff_t number = 0;
  for (char const *next = str; *next; ++next) {
    char const *d = strchr(digits, *next);
    number = number * 64 + (d - digits);
  }
  return (uint)number;
}

void dictd_cut_extension(char *basename) {
  size_t len = strlen(basename);
  if (basename[len - 6] == '.')
    basename[len - 6] = '\0';
  else
    basename[len - 9] = '\0';
}

void dictd_dictionary_open(Dictionary *interface) {
  Dictd *dictd = CONTAINER_OF(interface, Dictd, interface);
  dictd->handle = gzopen(dictd->index, "rb");
}
bool dictd_dictionary_next_word(Dictionary *interface,
                                WordDefinition *result) {
  Dictd *dictd = CONTAINER_OF(interface, Dictd, interface);
  const int len = MAX_WORD_LENGHT + 1 + 6 + 1 + 6 + 2;
  char word[MAX_WORD_LENGHT];
  char buf[len];
  char offset[6]; // 64^6 much larger than 2^64.
  char size[6];
  while (gzgets(dictd->handle, buf, len)) {
    if (sscanf(buf, "%[^\t]\t%[^\t]\t%[^\t\n]", word, offset, size) &&
        strcmp(word, "00databasealphabet") != 0 &&
        strcmp(word, "00databasedictfmt1130") != 0 &&
        strcmp(word, "00databaseinfo") != 0 &&
        strcmp(word, "00databaseshort") != 0 &&
        strcmp(word, "00databaseurl") != 0 &&
        strcmp(word, "00databaseutf8") != 0) {
      xstrcpy(&result->word, word);
      result->def.offset = decodeBase64(offset);
      result->def.size = decodeBase64(size);
      return true;
    }
  }
  return false;
}

void dictd_dictionary_close(Dictionary *interface) {
  Dictd *dictd = CONTAINER_OF(interface, Dictd, interface);
  gzclose(dictd->handle);
}

void dictd_dictionary_free(Dictionary *interface) {
  dictionary_free(interface);
  Dictd *dictd = CONTAINER_OF(interface, Dictd, interface);
  free(dictd->index);
  free(dictd);
}

void dicd_clean_name(WordDefinition *result) {
  char word[strlen(result->definition) + 1];
  sscanf(result->definition, "00-database-short\n%[^\n]\n", word);
  strcpy(result->definition, word);
}

void dictd_find_name(Dictd *self) {
  Dictionary *interface = &self->interface;
  WordDefinition result = worddefinition_init();
  dictionary_open(interface);
  const int len = MAX_WORD_LENGHT;
  char buf[len];
  char offset[6]; // 64^6 much larger than 2^32.
  char size[6];
  while (gzgets(self->handle, buf, len)) {
    if (sscanf(buf, "00databaseshort\t%[^\t]\t%[^\t\n]", offset, size)) {
      result.def.offset = decodeBase64(offset);
      result.def.size = decodeBase64(size);
      dictionary_get_definition(interface, &result);
      dicd_clean_name(&result);
      break;
    }
  }
  interface->name = result.definition;
  dictionary_close(interface);
}

void dictd_save(Dictionary *interface, FILE *out) {
  Dictd *dictd = CONTAINER_OF(interface, Dictd, interface);
  write_short(interface->id, out);
  char *list[] = {interface->name, dictd->dict,dictd->index};
  write_strings(out, ARRAY_LENGTH(list), list);
}


Dictionary *dictd_create(const char *filename) {
  Dictd *dictd;
  dictd = xmalloc(sizeof *dictd);
  Dictionary *interface = &dictd->interface;
  dictionary_init(interface, xstrdup(filename), dictd_save,
                  dictd_dictionary_open, dictd_dictionary_next_word,
                  dictd_dictionary_close, dictd_dictionary_free,
                  dictionary_get_definition);
  dictd->index = xstrdup(filename);

  char basename[strlen(filename) + 20];
  strcpy(basename, filename);
  dictd_cut_extension(basename);

  char *dict_file = append_extension(basename, ".dict");
  dictd->dict = dict_file ? dict_file : append_extension(basename, ".dict.dz");

  interface->words_file = dictd->dict;

  if (dictd->dict) {
    dictd_find_name(dictd);
    if (dictd->interface.name)
      interface->to_index = true;
  }
  return interface;
}

Dictionary *dictd_load(void **p, const char *filename) {
  Dictd *dictd;
  dictd = xmalloc(sizeof *dictd);
  Dictionary *interface = &dictd->interface;
  dictionary_init(interface, xstrdup(filename), dictd_save,
                  dictd_dictionary_open, dictd_dictionary_next_word,
                  dictd_dictionary_close, dictd_dictionary_free,
                  dictionary_get_definition);

  interface->to_index=1;
  interface->id=read_short_mm(p);
  char **const list[] = {&interface->name, &dictd->dict,&dictd->index};
  read_strings_mm(p, ARRAY_LENGTH(list), list);
  interface->words_file = dictd->dict;
  return interface;
}

