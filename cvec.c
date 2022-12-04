#include "cvec.h"
#include "utils.h"

void vec_grow(void **vector, size_t more, size_t type_size) {
  vector_t *meta;
  size_t count = 0;
  void *data = NULL;

  if (*vector) {
    meta = VECTOR_META(*vector);
    count = meta->size + more;
    data = realloc(meta, type_size * count + sizeof *meta);
  } else {
    count = more + 1;
    data = malloc(type_size * count + sizeof *meta);
    ((vector_t *)data)->size = 0;
  }

  meta = (vector_t *)data;
  *vector = meta + 1;
}

/* void vec_grow(void **vector,  size_t type_size) { */
/*   vector_t *meta; */
/*   size_t count = 0; */
/*   void *data = NULL; */
  
/*   if (*vector) { */
/*     meta = VECTOR_META(*vector); */
/*     count = meta->size + 1; */
/*     data = xrealloc(meta, type_size * count + sizeof *meta); */
/*   } else { */
/*     count = 1 ; */
/*     data = xmalloc(type_size * count + sizeof *meta); */
/*     ((vector_t *)data)->size=0; */
/*   } */
  
/*   meta = (vector_t *)data; */
/*   *vector = meta + 1; */
/* } */

/* void vec_malloc(void **vector,  size_t allocate, size_t type_size) { */
/*   vector_t *meta; */
/*   void *data = NULL; */
/*   data = xmalloc(type_size * allocate + sizeof *meta); */
/*   ((vector_t *)data)->size=  allocate ; */
  
/*   meta = (vector_t *)data; */
/*   *vector = meta + 1; */
/* } */


void vec_delete(void *vector) {
  free(VECTOR_META(vector));
}
