#include "dictionaries.h"
#include "utils.h"
#include <arpa/inet.h> //ntohl
#include <stdio.h>

typedef struct {
  int filesize;
  int offsetbytes;
  int wordcount;
  int current;
  char *path;
  char *file;
  char **words;
} IndexFile;

typedef struct {
  char sametypesequence;
  gzFile handle_syn;
  char *ifo;
  char *syn;
  char *dict;
  IndexFile idx;
  Dictionary interface;
} StarDict;

void stardict_cut_extension(char *basename) {
  size_t len = strlen(basename);
  if (basename[len - 4] == '.')
    basename[len - 4] = '\0';
  else
    basename[len - 7] = '\0';
}

void stardict_parse_ifo(StarDict *self) {
  char buf[CHAR_BUFFER_SIZE];
  char *subString;
  gzFile file = gzopen(self->ifo, "rb");
  gzgets(file, buf, CHAR_BUFFER_SIZE);
  if (!strcmp(buf, "StarDict's dict ifo file\n")) // magick data
  {
    while (gzgets(file, buf, CHAR_BUFFER_SIZE) != NULL) {
      subString = strtok(buf, "=");
      subString = strtok(NULL, "\n");
      if (strstr(buf, "bookname"))
        self->interface.name = xstrdup(subString);
      if (strstr(buf, "sametypesequence"))
        self->sametypesequence = subString[0];
      if (strstr(buf, "wordcount") && !strstr(buf, "synwordcount"))
        self->idx.wordcount = atoi(subString);
      if (strstr(buf, "idxfilesize"))
        self->idx.filesize = atoi(subString);
      if (strstr(buf, "idxoffsetbits"))
        if (atoi(buf) == 64)
          self->idx.offsetbytes = 8;
    }
  }
  gzclose(file);
}

void stardict_dictionary_open(Dictionary *interface) {
  StarDict *stardict = CONTAINER_OF(interface, StarDict, interface);
  gzFile file = gzopen(stardict->idx.path, "rb");
  int idxfilesize = stardict->idx.filesize * (sizeof *stardict->idx.file);
  stardict->idx.file = xmalloc(idxfilesize);
  const int len = gzread(file, stardict->idx.file, idxfilesize);
  gzclose(file);
  if (len < 0 || len != idxfilesize) {
    stardict->interface.to_index = 0;
  }
  stardict->idx.words =
      xmalloc(stardict->idx.wordcount * (sizeof *stardict->idx.words));
  char *p = stardict->idx.file;
  for (int i = 0; i < stardict->idx.wordcount; i++) {
    stardict->idx.words[i] = p;
    p += strlen(p) + 1 + 2 * sizeof(uint32_t);
  };

  stardict->idx.current = 0;

  if (stardict->syn)
    stardict->handle_syn = gzopen(stardict->syn, "rb");
}

void stardict_fill_word(gzFile handle, char *p) {
  while (gzread(handle, p, 1) == 1 && *p != '\0')
    p++;
}

uint stardict_dictionary_read_word(StarDict *self, WordDefinition *result,
                                   int idx) {
  if (idx >= self->idx.wordcount) {
    return 0;
  }
  char *p = self->idx.words[idx];
  xstrcpy(&result->word, p);
  p += strlen(p) + 1;
  result->offset = ntohl(*(uint32_t *)p);
  result->size = ntohl(*(uint32_t *)(p + sizeof(uint32_t)));
  return 1;
}

uint stardict_syn_next_word(StarDict *stardict, WordDefinition *result) {
  WordDefinition tmp_result = worddefinition_init();
  unsigned char offset[sizeof(uint32_t)] = {0};
  char word[MAX_WORD_LENGHT];
  stardict_fill_word(stardict->handle_syn, word);
  if (gzread(stardict->handle_syn, offset, sizeof(uint32_t)) !=
      sizeof(uint32_t))
    return 0;
  uint pos = ntohl(*(int32_t *)&offset);
  uint rc = stardict_dictionary_read_word(stardict, &tmp_result, pos);
  worddefinition_free(&tmp_result);
  if (rc != 1)
    return 0;
  xstrcpy(&result->word, word);
  result->offset = tmp_result.offset;
  result->size = tmp_result.size;
  return 1;
}

uint stardict_dictionary_next_word(Dictionary *interface,
                                   WordDefinition *result) {
  StarDict *stardict = CONTAINER_OF(interface, StarDict, interface);
  uint rc =
      stardict_dictionary_read_word(stardict, result, stardict->idx.current++);
  if (rc != 1 && stardict->syn)
    return stardict_syn_next_word(stardict, result);
  else if (rc != 1)
    return 0;
  return 1;
}

void stardict_dictionary_close(Dictionary *interface) {
  StarDict *stardict = CONTAINER_OF(interface, StarDict, interface);
  if (stardict->syn)
    gzclose(stardict->handle_syn);
}

void stardict_dictionary_free(Dictionary *interface) {
  dictionary_free(interface);
  StarDict *stardict = CONTAINER_OF(interface, StarDict, interface);
  free(stardict->ifo);
  free(stardict->syn);
  free(stardict->idx.file);
  free(stardict->idx.path);
  free(stardict->idx.words);
  free(stardict);
}


void stardict_save(Dictionary *interface, FILE *out) {
  StarDict *stardict = CONTAINER_OF(interface, StarDict, interface);
  write_long(interface->id, out);
  fputc(stardict->sametypesequence, out);
  char *list[] = {interface->name, stardict->ifo, stardict->syn, stardict->dict};
  write_strings(out, ARRAY_LENGTH(list), list);
}

Dictionary *stardict_create(const char *filename) {
  StarDict *stardict;
  stardict = xmalloc(sizeof *stardict);
  Dictionary *interface = &stardict->interface;
  stardict->idx.path = xstrdup(filename);
  stardict->idx.offsetbytes = 4;
  stardict->idx.file = NULL;
  stardict->idx.words = NULL;
  stardict->sametypesequence = 'm';
  dictionary_init(interface, xstrdup(filename), stardict_save,
                  stardict_dictionary_open, stardict_dictionary_next_word,
                  stardict_dictionary_close, stardict_dictionary_free,
                  dictionary_get_definition);

  char basename[strlen(filename) + 20];
  strcpy(basename, filename);
  stardict_cut_extension(basename);

  char *dict_file = append_extension(basename, ".dict");
  stardict->dict =
      dict_file ? dict_file : append_extension(basename, ".dict.dz");
  interface->words_file = stardict->dict;
  stardict->ifo = append_extension(basename, ".ifo");
  stardict->syn = append_extension(basename, ".syn");
  stardict_parse_ifo(stardict);

  if (stardict->dict && interface->name && stardict->idx.offsetbytes == 4)
    interface->to_index = 1;
  return interface;
}



Dictionary *stardict_load(void **p, const char *filename) {
  StarDict *stardict;
  stardict = xmalloc(sizeof *stardict);
  Dictionary *interface = &stardict->interface;
  dictionary_init(interface, xstrdup(filename), stardict_save,
                  stardict_dictionary_open, stardict_dictionary_next_word,
                  stardict_dictionary_close, stardict_dictionary_free,
                  dictionary_get_definition);

  interface->to_index = 1;
  interface->id=read_long_mm(p);
  stardict->sametypesequence=read_char_mm(p);
  stardict->idx.file = NULL;
  stardict->idx.path = NULL;
  stardict->idx.words= NULL;

  char ** const list[] = {&interface->name, &stardict->ifo, &stardict->syn, &stardict->dict};
  read_strings_mm(p, ARRAY_LENGTH(list), list);

  interface->words_file = stardict->dict;

  return interface;
}

