#define _GNU_SOURCE
#include "utils.h"
#include <string.h> //strdup, memcpy, strcmp, strlen sscanf strtok
#include <stdio.h> //perror printf FILENAME_MAX

void * xcalloc (size_t n, size_t s)
{
  void *val;
  val = calloc (n, s);
  CHECK_PTR(val)
    return val;
}

void * xrealloc (void *ptr, size_t size)
{
  void *value = realloc (ptr, size);
  CHECK_PTR(value)
    return value;
}

void * xmalloc (size_t size)
{
  void *value = malloc (size);
  CHECK_PTR(value)
    return value;
    }

char * xstrdup (const char *s1)
{
  void *value = strdup(s1);
  CHECK_PTR(value)
  return value;
}

void xstrcpy (char **s1,const char *s2)
{
  *s1=xmalloc((strlen(s2)+1)*sizeof(char));
  strcpy(*s1, s2);
}

void xasprintf(char **strp, const char *fmt, char *src)
{
  int rc;
  rc=asprintf(strp, fmt, src);
  if(rc==-1)
    {
      printf("asprintf failed");
      exit(EXIT_FAILURE);
    }
}
