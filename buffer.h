#pragma once
#include <stddef.h>

typedef struct Buffer Buffer;

typedef struct BufferState BufferState;

BufferState *bufferstate_save(const Buffer *self);
void bufferstate_restore(Buffer *self, BufferState *state);

Buffer *buffer_init();

void buffer_free(Buffer *self);

void buffer_push(Buffer *self, const char ch);

void buffer_pushchars(Buffer *self, const char* str);

char *buffer_strstr(Buffer *self, const char *needle);

void buffer_search_change(Buffer *self, size_t search);

size_t buffer_getlen(const Buffer *self);
void buffer_shrink(Buffer *self, size_t len);
char *buffer_steal(const Buffer *self);

