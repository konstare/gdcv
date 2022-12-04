#ifndef DICTIONARY_H
#define DICTIONARY_H
#include "dictzip.h" //dict_data_read_, dict_data_close, dict_data_open
#include "utf8proc/utf8proc.h"
#include "utils.h"
#include <stddef.h>
#include <stdlib.h>    //free
#include <string.h>    //strchr
#include <sys/types.h> //uint

#define CHAR_BUFFER_SIZE 2097152
#define MAX_WORD_LENGHT 1024

#if __STDC_VERSION__ >= 201112L
#define CONTAINER_OF(PTR, TYPE, MEMBER)                                        \
  (_Generic((PTR),						      \
            const __typeof__ (((TYPE *) 0)->MEMBER) *:		      \
	    ((TYPE *) ((char *) (PTR) - offsetof(TYPE, MEMBER))),     \
            __typeof__ (((TYPE *) 0)->MEMBER) *:		      \
	    ((TYPE *) ((char *) (PTR) - offsetof(TYPE, MEMBER)))      \
	    ))
#elif defined __GNUC__
#define CONTAINER_OF(PTR, TYPE, MEMBER)                                        \
  (__extension__({                                                             \
    __extension__ const __typeof__(((TYPE *)0)->MEMBER) *__m____ = (PTR);      \
    ((TYPE *)((char *)__m____ - offsetof(TYPE, MEMBER)));                      \
  }))
#else
#define CONTAINER_OF(PTR, TYPE, MEMBER)                                        \
  ((TYPE *)((char *)(PTR)-offsetof(TYPE, MEMBER)))
#endif


typedef struct {
  char *word;
  char *definition;
  char *word_utf_clean;
  uint_fast16_t dictionary_number;
  uint offset; // Files more than 4 Gb are not supported.
  uint size;
} WordDefinition;


typedef struct Dictionary {
  char *name; //dictionary name
  char *words_file;  //the file where words are
  char *basic_filename; // the file which we use to identify the dictionary type
  uint to_index; //to index file or not after all checks
  uint id; //dictionary id
  void (*save)(struct Dictionary *interface, FILE *out);
  void (*open)(struct Dictionary *interface);
  uint (*next_word)(struct Dictionary *interface, WordDefinition *result);
  void (*close)(struct Dictionary *interface);
  void (*free)(struct Dictionary *interface);
  size_t (*definition)(struct Dictionary *interface,
                       WordDefinition *result);
} Dictionary;

void dictionary_init(Dictionary *interface, char * basic_filename,
		     void (*save)(Dictionary *interface, FILE *out),
		     void (*open)(Dictionary *interface),
		     uint (*next_word)(Dictionary *interface, WordDefinition *result),
		     void (*close)(Dictionary *interface),
		     void (*free)(Dictionary *interface),
		     size_t (*definition)(Dictionary *interface, WordDefinition *result));

void dictionary_open(Dictionary *interface);
void dictionary_close(Dictionary *interface);

size_t dictionary_get_definition(Dictionary *interface,
                                       WordDefinition *result);
void dictionary_free(Dictionary *interface);

Dictionary *stardict_create(const char *filename);
Dictionary *dictd_create(const char *filename);
Dictionary *dsl_create(const char *filename);

Dictionary *stardict_load(void **p, const char *filename);
Dictionary *dictd_load(void **p, const char *filename);
Dictionary *dsl_load(void **p, const char *filename);


char *append_extension(char *basename, const char *extension);

typedef struct {
  const char *extension;
  Dictionary *(*create)(const char *);
  Dictionary *(*load)(void **p, const char *);
} DictionaryFiles;

void worddefinition_utf8_cleaning(WordDefinition *self);
void worddefinition_free(WordDefinition *self);
WordDefinition worddefinition_init();

static const DictionaryFiles supported_dictionary[] = {
  {.extension = "*.idx?(.gz)", .create = stardict_create, .load = stardict_load},
  {.extension = "*.index", .create = dictd_create, .load = dictd_load},
  {.extension = "!(*_abrv).dsl?(.dz)", .create = dsl_create, .load = dsl_load}};

typedef struct {
  Dictionary **dictionary; //vector of dictionaries
} DictionaryArray;

void dictionary_array_free(DictionaryArray self);
DictionaryArray dictionary_array_init(RealPaths realpath);
void dictionary_array_write(DictionaryArray *D, FILE *out);
DictionaryArray dictionary_array_load(void *p);

#endif