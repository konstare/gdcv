#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> //malloc, realloc, free, size_t, NULL, exit, EXIT_FAILURE , EXIT_SUCCESS, system

static const char EMPTY_STR[] = "";

#define GET_UNALIGNED(ptr)                                                     \
  __extension__({                                                              \
    struct __attribute__((packed)) {                                           \
      typeof(*(ptr)) __v;                                                      \
    } *__p = (typeof(__p))(ptr);                                               \
    __p->__v;                                                                  \
  })

#define ARRAY_LENGTH(x)                                                        \
  ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

#define CHECK_PTR(ptr)                                                         \
  do {                                                                         \
    if (ptr == NULL) {                                                         \
      perror("Out of memory");                                                 \
      exit(EXIT_FAILURE);                                                      \
    };                                                                         \
  } while (0)

/* Allocate NMEMB elements of SIZE bytes each, all initialized to 0.  */
void *xcalloc(size_t nmemb, size_t size);
void *xrealloc(void *ptr, size_t size);
void *xmalloc(size_t size);
char *xstrdup(const char *s1);
void xstrcpy(char **s1, const char *s2);
void combine_full_path(char *result, const char *path, const char *file);
uint check_if_file_exist(const char *filename);


typedef struct RealPaths_ *RealPaths;

extern void realpath_print_not_exist(RealPaths self, void (*print_function)(const char *));
void realpath_free(RealPaths self);
RealPaths realpath_init(int argc, const char **argv);
char **realpath_get_dirs(RealPaths self);
char **realpath_get_not_exist(RealPaths self);



void write_string(FILE *out, const char *str);
void write_strings(FILE *out, const size_t, char * const list[]);
void write_long(const uint32_t number, FILE *out);
void write_short(const uint16_t number, FILE *out);

uint16_t read_short_mm(void **p);
uint32_t read_long_mm(void **p);
char read_char_mm(void **p);
const char *read_string_mm(void **p);
void read_strings_mm(void **p, const size_t len, char** const list[]);
  
