#include <stddef.h> //size_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "heap.h"
#include "utils.h"

/*to get index of the parent of node at index i*/
static inline size_t heap_parent(size_t i) { return (i-1)/2; }

/* to get index of left child of node at index i*/
static inline size_t left(size_t i) { return (2*i + 1); }

/* to get index of right child of node at index i*/
static inline size_t right(size_t i) { return (2*i + 2); }

/*to get element with index from the array heap*/
static inline void *element(void *heap, size_t index){
  size_t size_type = HEAP_META(heap)->size_type;
  return (unsigned char*)heap + index*size_type;
}

/* swap two elements with indexes i and j in the heap array.*/
static inline void heap_swap(void *heap, size_t i, size_t j){
  size_t size_type = HEAP_META(heap)->size_type;
  unsigned char *p = element(heap, i );
  unsigned char *q = element(heap, j );
  unsigned char tmp;
  for(size_t k=0; k< size_type; k++){
    tmp = p[k];
    p[k]=q[k];
    q[k]=tmp;
  }
}

/*Restore the heap property in the array after i-element.*/
static void heap_heapify(void *heap, size_t i){
  heap_t *meta = HEAP_META(heap);
  bool (*function)(void *a, void *b) = meta->function;
  size_t l = left(i);
  size_t r = right(i);
  size_t current = i;
  if (l < meta->size && function(element(heap, l), element(heap, i )))
    current = l;
  if (r < meta->size && function(element(heap, r), element(heap, current)))
    current = r;
  if (current != i)
    {
      heap_swap(heap, i, current );
      heap_heapify(heap, current);
    }
}

void heap_delete(void *heap) {
  free(HEAP_META(heap));
}
void heap_grow(void **heap, size_t more, size_t type_size) {
  heap_t *meta;
  size_t count = 0;
  void *data = NULL;

  if (*heap) {
    meta = HEAP_META(*heap);
    count = 2 * meta->capacity + more;
    data = xrealloc(meta, type_size*count+ sizeof *meta);   
  } else {
    count = more + 1;
    data = xmalloc(type_size * count + sizeof *meta);
    ((heap_t *)data)->size = 0;
    ((heap_t *)data)->size_type = type_size;
  }

  meta = (heap_t *)data;
  meta->capacity = count;
  *heap = meta + 1;
}

/*Remove the maximum element of the heap. */
void heap_pop(void *heap){
  heap_t *meta = HEAP_META(heap);
  if (meta->size < 2  )
    {
      meta->size =0;
      return ;
    }
  // forget the root value
  memcpy(element(heap, 0), element(heap, --meta->size), meta->size_type);
  heap_heapify(heap, 0);
  return ;
}

/* Restore the heap property after insertion of a key in the back*/
void heap_fix(void *heap){
  heap_t *meta= HEAP_META(heap);
  bool (*function)(void *a, void *b) = meta->function;
  size_t i = meta->size -1;
  while (i != 0 && function(element(heap, i), element(heap, heap_parent(i))))
    {
      heap_swap(heap, i, heap_parent(i));
      i = heap_parent(i);
    }
}

