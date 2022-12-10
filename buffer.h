#pragma once
#include <stddef.h>

typedef struct Buffer Buffer;

Buffer *Buffer_init();

void Buffer_free(Buffer *self);

void Buffer_push(Buffer *self, const char ch);

void Buffer_pushchars(Buffer *self, const char* str);

char *Buffer_strstr(Buffer *self, const char *needle);

void Buffer_search_change(Buffer *self, size_t search);

size_t Buffer_getlen(Buffer *self);
void Buffer_setlen(Buffer *self, size_t len);
void Buffer_shrink(Buffer *self, size_t len);
char *Buffer_steal(Buffer *self);

