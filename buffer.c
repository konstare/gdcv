#include <string.h>
#include "dictionaries.h"
#include "buffer.h"
#include "utils.h"

struct Buffer{
  char *bytes;
  size_t len;
  size_t search_start;
};

void buffer_search_change(Buffer *self, size_t search){
  self->search_start= search;
}



Buffer *buffer_init(){
  Buffer *result=xcalloc(1, sizeof *result);
  char buffer[CHAR_BUFFER_SIZE];
  result->bytes=buffer;
  return result;
}

void buffer_free(Buffer *self){
  free(self);
}

void buffer_push(Buffer *self, const char ch){
  self->bytes[self->len++]=ch;
  self->bytes[self->len]='\0';
}

void buffer_pushchars(Buffer *self, const char* str){
  strcpy(self->bytes +self->len, str);
  self->len += strlen(str);
}

char *buffer_strstr(Buffer *self, const char *needle){
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

size_t buffer_getlen(const Buffer *self){
  return self->len;
}

void buffer_shrink(Buffer *self, size_t len){
  self->len -=len;
}

char *buffer_steal(const Buffer *self){
  return xstrdup(self->bytes);
}


struct BufferState {
  size_t len;
  size_t search;
};

BufferState *bufferstate_save(const Buffer *self){
  BufferState *result = xmalloc(sizeof *result);
  result->len= self->len;
  result->search = self->search_start;
  return result;
}

void bufferstate_restore(Buffer *self, BufferState *state){
  self->len = state->len;
  self->search_start = state->search;
  free(state);
}
