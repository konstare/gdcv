#include <string.h>
#include "dictionaries.h"
#include "buffer.h"

struct Buffer{
  char *bytes;
  size_t len;
  size_t search_start;
};

void Buffer_search_change(Buffer *self, size_t search){
  self->search_start= search;
}



Buffer *Buffer_init(){
  Buffer *result=xcalloc(1, sizeof *result);
  char buffer[CHAR_BUFFER_SIZE];
  result->bytes=buffer;
  return result;
}

void Buffer_free(Buffer *self){
  free(self);
}

void Buffer_push(Buffer *self, const char ch){
  self->bytes[self->len++]=ch;
  self->bytes[self->len]='\0';
}

void Buffer_pushchars(Buffer *self, const char* str){
  strcpy(self->bytes +self->len, str);
  self->len += strlen(str);
}

char *Buffer_strstr(Buffer *self, const char *needle){
  size_t len = strlen(needle);
  char *result=strstr(self->bytes + self->search_start, needle);
  if(result){
    self->search_start = (result- self->bytes) + len;
  }
  else{
    self->search_start = self->len > len ? self->len - len : 0;
  }
  return result;
}

size_t Buffer_getlen(Buffer *self){
  return self->len;
}

void Buffer_setlen(Buffer *self, size_t len){
  self->len=len;
}

void Buffer_shrink(Buffer *self, size_t len){
  self->len -=len;
}

char *Buffer_steal(Buffer *self){
  return xstrdup(self->bytes);
}

