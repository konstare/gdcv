#include <stdlib.h> //malloc, realloc, free, size_t, NULL, exit, EXIT_FAILURE , EXIT_SUCCESS, system


#define CHECK_PTR(ptr)			  \
  if (ptr == NULL) {			  \
    perror("Out of memory");		  \
    exit(EXIT_FAILURE); \
};


void * xcalloc (size_t n, size_t s);
void * xrealloc (void *ptr, size_t size);
void * xmalloc (size_t size);
char * xstrdup (const char *s1);
void xstrcpy (char **s1,const char *s2);
void xasprintf(char **strp, const char *fmt, char *src);
