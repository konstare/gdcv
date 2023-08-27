/*
  The original vector implementation is from
  https://github.com/graphitemaster/cvec

  These vectors can be manipulated in a similar manner to C++ vectors; their elements can be accessed via the [] operator and elements may be added or removed through the use of simple library calls.
  This works because these vectors are stored directly alongside special header in memory, which keeps track of the vector's size and capacity:
  +------------+------------------+
  | Header | Vector data |
  +------------+------------------+
               |
                ` -> Pointer returned to the user.
 */
#pragma once
#include <string.h>
#include <stdlib.h>

#define VECTOR(T) T*

typedef struct {
    size_t size;
} vector_t;

/* Attempts to grow [VECTOR] */
#define VECTOR_TRY_GROW(VECTOR, SIZE)				\
  ((void)vec_grow(((void **)&(VECTOR)), (SIZE), sizeof(*(VECTOR))) )



/* Get the metadata block for [VECTOR] */
#define VECTOR_META(VECTOR) \
    ((vector_t *)(((unsigned char *)(VECTOR)) - sizeof(vector_t)))

/* Resize [VECTOR] to accomodate [SIZE] more elements */
#define VECTOR_RESIZE(VECTOR, SIZE) \
    (VECTOR_TRY_GROW((VECTOR), (SIZE)), VECTOR_META(VECTOR)->size += (SIZE), \
        &(VECTOR)[VECTOR_META(VECTOR)->size - (SIZE)])

  
/* Deletes [VECTOR] and sets it to NULL */
#define VECTOR_FREE(VECTOR) \
    ((void)((VECTOR) ? (vec_delete((void *)(VECTOR)), (VECTOR) = NULL) : 0))

/* Pushes back [VALUE] into [VECTOR] */
#define VECTOR_PUSH(VECTOR, VALUE) \
  (VECTOR_TRY_GROW((VECTOR), 1), (VECTOR)[VECTOR_META(VECTOR)->size++] = (VALUE))

/* Get the size of [VECTOR] */
#define VECTOR_SIZE(VECTOR) \
    ((VECTOR) ? VECTOR_META(VECTOR)->size : 0)

/* Pop an element off the back of [VECTOR] */
#define VECTOR_POP(VECTOR) \
  ((VECTOR)[--(VECTOR_META(VECTOR)->size)])

/* Append to [VECTOR], [COUNT] elements from [POINTER] */
#define VECTOR_APPEND(VECTOR, COUNT, POINTER) \
    ((void)(memcpy(VECTOR_RESIZE((VECTOR), (COUNT)), (POINTER), (COUNT) * sizeof(*(POINTER)))))

void vec_grow(void **vector, size_t more, size_t type_size);
void vec_delete(void *vector);
