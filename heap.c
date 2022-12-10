#include <stddef.h> //size_t
#include <stdlib.h>
#include "forms.h"
#include "utils.h"

typedef struct {
  size_t capacity;
  size_t heap_size;
} heap_t;

/* Get the metadata block for [HEAP] */
#define HEAP_META(HEAP) \
    ((heap_t *)(((unsigned char *)(HEAP)) - sizeof(heap_t)))

void heap_delete(void *heap) {
  free(HEAP_META(heap));
}

/* Deletes [VECTOR] and sets it to NULL */
#define HEAP_FREE(HEAP) \
    ((void)((HEAP) ? (heap_delete((void *)(HEAP)), (HEAP) = NULL) : 0))

void heap_grow(void **heap, size_t more, size_t type_size) {
    heap_t *meta = HEAP_META(*heap);
    size_t count = 0;
    void *data = NULL;

    if (*heap) {
        count = 2 * meta->capacity + more;
        data = xrealloc(meta, type_size * count + sizeof *meta);
    } else {
        count = more + 1;
        data = xmalloc(type_size * count + sizeof *meta);
        ((heap_t *)data)->heap_size = 0;
    }

    meta = (heap_t *)data;
    meta->capacity = count;
    *heap = meta + 1;
}

/* Attempts to grow [VECTOR] */
#define HEAP_TRY_GROW(HEAP, SIZE)				\
  ((void)heap_grow(((void **)&(HEAP)), (SIZE), sizeof(*(HEAP))) )

void heap_swapping(void * a, void * b, size_t type_size)
{
    unsigned char *p = a, *q = b, tmp;
    for (size_t i = 0; i < type_size; i++)
    {
        tmp = p[i];
        p[i] = q[i];
        q[i] = tmp;
    }
}

/* Swap elements of a  [HEAP] */
#define HEAP_SWAP(HEAP, I, J)				\
((void)heap_swaping((HEAP)[(I)], (HEAP)[(J)], sizeof(*(HEAP))) )




typedef struct{
  size_t len;
  char *word;
  M_Form m_form;
} Search_Result;

typedef struct{
  size_t capacity;
  size_t heap_size;
  Search_Result *array;
} Heap;

void heap_swap(Heap *self, size_t i, size_t j)
{
    Search_Result temp = self->array[i];
    self->array[i]= self->array[j];
    self->array[j] = temp;
}

static inline size_t heap_parent(size_t i) { return (i-1)/2; }

// to get index of left child of node at index i
static inline size_t left(size_t i) { return (2*i + 1); }
 
// to get index of right child of node at index i
static inline size_t right(size_t i) { return (2*i + 2); }

Heap *heap_init(size_t capacity)
{
  Heap *result;
  result=xmalloc(sizeof *result + capacity*(sizeof *result->array));
  result->heap_size=0;
  result->capacity=capacity;
  return result;
}


Search_Result *heap_get_root(Heap *self) {return &self->array[0];}


void heap_heapify(Heap *self, size_t i, int (*compare)(void *a, void *b))
{
    size_t l = left(i);
    size_t r = right(i);
    size_t current = i;
    if (l < self->heap_size && compare(&self->array[l], &self->array[i]))
        current = l;
    if (r < self->heap_size && compare(&self->array[r], &self->array[i]))
        current = r;
    if (current != i)
    {
      heap_swap(self, i, current);
      heap_heapify(self, current, compare);
    }
}
 

// Inserts a new key 'k'
void heap_insert_key(Heap *self, Search_Result key, int (*compare)(void *a, void *b))
{
    if (self->heap_size == self->capacity)
      {
	self->capacity *=2;
	self->array = xrealloc(self->array, self->capacity* (sizeof *self->array));
    }
 
    // First insert the new key at the end
    self->heap_size++;
    size_t i = self->heap_size - 1;
    self->array[i] = key;
 
    // Fix the min heap property if it is violated
    while (i != 0 && !compare(&self->array[heap_parent(i)], &self->array[i]))
      {
	heap_swap(self, i, heap_parent(i));
	i = heap_parent(i);
    }
}

// Method to remove minimum element (or root) from min heap
Search_Result heap_extract_root(Heap *self, int (*compare)(void *a, void *b))
{
    if (self->heap_size <= 0)
      return (Search_Result){0};
    if (self->heap_size == 1)
    {
        self->heap_size--;
        return self->array[0];
    }
 
    // Store the minimum value, and remove it from heap
    Search_Result root = self->array[0];
    self->array[0] = self->array[self->heap_size-1];
    self->heap_size--;
    heap_heapify(self, 0, compare);
 
    return root;
}
