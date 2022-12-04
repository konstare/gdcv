#include <netinet/in.h>
#include <stdint.h>
#define _GNU_SOURCE
#include "utils.h"
#include "cvec.h"
#include <stdio.h>    //asprintf perror printf FILENAME_MAX
#include <string.h>   //strdup, memcpy, strcmp, strlen sscanf strtok
#include <sys/stat.h> //stat




inline void *xcalloc(size_t n, size_t s) {
  void *val;
  val = calloc(n, s);
  CHECK_PTR(val);
  return val;
}

inline void *xrealloc(void *ptr, size_t size) {
  void *value = realloc(ptr, size);
  CHECK_PTR(value);
  return value;
}

inline void *xmalloc(size_t size) {
  void *value = malloc(size);
  CHECK_PTR(value);
  return value;
}

inline char *xstrdup(const char *s1) {
  void *value = strdup(s1);
  CHECK_PTR(value);
  return value;
}

inline void xstrcpy(char **s1, const char *s2) {
  *s1 = xmalloc((strlen(s2) + 1) * sizeof(char));
  strcpy(*s1, s2);
}

inline void combine_full_path(char *result, const char *path,
                              const char *file) {
  int rc = sprintf(result, "%s/%s", path, file);
  if (rc == -1) {
    printf("sprintf failed");
    exit(EXIT_FAILURE);
  }
}

uint check_if_file_exist(const char *filename) {
  struct stat buffer;
  int exist = stat(filename, &buffer);
  if (exist == 0)
    return 1;
  else
    return 0;
}

void realpath_free(RealPaths self) {
  for (uint i = 0; self.dirs[i]; i++) {
    free(self.dirs[i]);
  };
  for (uint i = 0; i<VECTOR_SIZE(self.not_exist); i++) {
    free(self.not_exist[i]);
  };
  VECTOR_FREE(self.dirs);
  VECTOR_FREE(self.not_exist);
}

RealPaths realpath_init(const int argc, const char **argv) {
  RealPaths result={NULL,NULL};
  char *real_path;
  for (int i = 0; i < argc; i++) {
    if (!(real_path= realpath(argv[i], NULL)))
      VECTOR_PUSH(result.not_exist, xstrdup(argv[i]));
    else
      VECTOR_PUSH(result.dirs, real_path);
  }
  // fts_open demands the array must be terminated by a NULL pointer.
  VECTOR_PUSH(result.dirs,  NULL);
  return result;
}

extern void realpath_print_not_exist(RealPaths self, void (*print_function)(const char *)) {
  for(uint i=0; i<VECTOR_SIZE(self.not_exist);i++){
    print_function(self.not_exist[i]);
  };
}

inline void write_string(FILE *out, const char *str){
  if(str){
    fputs(str, out);
  };
  fputc('\0', out);
}

inline void write_strings(FILE *out, uint len, char *list[]){
  for(uint i=0; i<len;i++){
    write_string(out, list[i]);
  }
}

inline void write_long(const uint32_t number, FILE *out){
  uint32_t number_ = htonl(number);
  fwrite(&number_, sizeof(number_), 1, out);
}

inline void write_short(const uint16_t number, FILE *out){
  uint16_t number_ = htons(number);
  fwrite(&number_, sizeof(number_), 1, out);
}


inline uint32_t read_long_mm(void **p) {
  uint8_t *addr = *(uint8_t **)p;
  uint32_t v;
  /* addr may be unalined to uint32_t */
  v = GET_UNALIGNED((uint32_t *)addr);
  *p = addr + sizeof(v);
  return ntohl(v);
}

inline uint16_t read_short_mm(void **p) {
  uint8_t *addr = *(uint8_t **)p;
  uint16_t v;
  /* addr may be unalined to uint16_t */
  v = GET_UNALIGNED((uint16_t *)addr);
  *p = addr + sizeof(v);
  return ntohs(v);
}


inline uint8_t read_char_mm(void **p) {
  uint8_t *addr = *(uint8_t **)p;
  uint8_t v = *addr;
  *p = addr + sizeof(v);
  return v;
}

inline const char *read_string_mm(void **p) {
  char *addr = *(char **)p;
  size_t len = strlen(addr);
  *p = addr + len + 1;

  if (len == 0)
    return EMPTY_STR;

  return addr;
}

inline void read_strings_mm(void **p, size_t len, char** const list[]) {
  for(size_t i=0; i<len; i++){
    *(list[i])=xstrdup(read_string_mm(p));
  }
}

