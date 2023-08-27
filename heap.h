#pragma once
#include <stddef.h>
#include <stdbool.h>

#define HEAP(T) T*

typedef struct {
  size_t size;
  size_t capacity;
  size_t size_type;
  bool (*function)(void *a, void *b);
} heap_t;


void heap_fix(void *heap);
void heap_grow(void **heap, size_t more, size_t type_size);
void heap_pop(void *heap);

/*Show the root of the [HEAP]*/
#define HEAP_ROOT(HEAP) (&(HEAP)[0])

/*Remove the root of the [HEAP]*/
#define HEAP_POP(HEAP) (void)(heap_pop(HEAP))

/*Insert [Value] into the [HEAP]*/
#define HEAP_INSERT(HEAP, VALUE)\
(void)((HEAP_TRY_GROW((HEAP), 1), (HEAP)[HEAP_META(HEAP)->size++] = (VALUE), heap_fix(HEAP)))



/* Attempts to grow [HEAP] by [MORE]*/
#define HEAP_TRY_GROW(HEAP, MORE)                                                                                               \
((( ! (HEAP)  || HEAP_META(HEAP)->size + (MORE) >= HEAP_META(HEAP)->capacity))     \
                   ? (void)heap_grow(((void **)&(HEAP)), (MORE), sizeof(*(HEAP))) : (void)0)


/* Init [HEAP] with comparison function [FUNCTION] between elements */
#define HEAP_INIT(HEAP, FUNCTION)\
        (void)((HEAP_TRY_GROW((HEAP), 1), HEAP_META(HEAP)->function = (FUNCTION)))


/* Get the metadata block for [HEAP] */
#define HEAP_META(HEAP) \
((heap_t *)(((unsigned char *)(HEAP)) - sizeof(heap_t)))

/* Deletes [HEAP] and sets it to NULL */
#define HEAP_FREE(HEAP) \
    ((void)((HEAP) ? (heap_delete((void *)(HEAP)), (HEAP) = NULL) : 0))

/* Get the size of [HEAP] */
#define HEAP_SIZE(HEAP) \
    ((HEAP) ? HEAP_META(HEAP)->size : 0)

void heap_delete(void *heap);
