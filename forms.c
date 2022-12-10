#include "cvec.h"
#include "dictionaries.h"
#include "forms.h"
#include "dictzip.h"
#include "utils.h"
#include <search.h> //lsearch
#include <stdint.h>
#include <stdio.h>

extern void form_free(Form *self) {
  for (size_t i = 0; i < VECTOR_SIZE(self); i++)
    {
      free(self[i].word);
      VECTOR_FREE(self[i].pos);
    }
  VECTOR_FREE(self);
}

static int comp_word(const void *a1, const void *a2) {
  const WordDefinition *v1 = a1;
  const Form *v2 = a2;
  return strcmp(v1->word, v2->word);
}

int form_push(Form **self, const WordDefinition *word) {
  
  
  size_t v_N = VECTOR_SIZE(*self);
  Form *current =
      lfind(word, *self, &v_N, (sizeof **self), comp_word);

  if (!current) {
    Form temp = {.word = xstrdup(word->word), .pos = NULL};
    VECTOR_PUSH(*self, temp);
    current = &(*self)[v_N];
  }
  VECTOR_PUSH(current->pos, word->def);
  return 1;
}

static void forms_write_definitions(const Definitions *dv, FILE *out) {
  uint16_t vN = (uint16_t)VECTOR_SIZE(dv);
  write_short(vN, out);
  for (uint16_t i = 0; i < vN; i++) {
    write_short(dv[i].id, out);
    write_long(dv[i].offset, out);
    write_long(dv[i].size, out);
  }
}

M_Form forms_write(Form *forms, FILE *out) {
  M_Form form_m={0};
  long int pos = ftell(out);
  if (pos<0)
    goto fail;
  form_m.start = (uint32_t)pos;
  uint16_t vN = (uint16_t)VECTOR_SIZE(forms);
  write_short(vN, out);
  for (uint16_t i = 0; i < vN; i++) {
    const Form form = forms[i];
    write_string(out, form.word);
    forms_write_definitions(form.pos, out);
  }
  pos = ftell(out);
  if (pos<0)
    goto fail;
  form_m.end = (uint32_t)pos;
fail:
  return form_m;
}

/* Form *forms_read(dictData *data, const char *key, const M_Form m_form) { */
/*   Form *result = NULL; */
/*   char  *const list = dict_data_read_(data, m_form.start, m_form.end - m_form.start, NULL, NULL); */
/*   void *p =list; */
/*   uint16_t vN = read_short_mm(&p); */
/*   for (uint16_t i = 0; i < vN; i++) { */
/*     Form cur = {NULL, NULL}; */
/*     const char *word = read_string_mm(&p); */
/*     cur.word = xstrdup(!strcmp(word, EMPTY_STR) ? key : word); */
/*     uint16_t dN = read_short_mm(&p); */
/*     Definitions dv; */
/*     for (uint16_t j = 0; j < dN; j++) { */
/*       dv.id = read_short_mm(&p); */
/*       dv.offset = read_long_mm(&p); */
/*       dv.size = read_long_mm(&p); */
/*       VECTOR_PUSH(cur.pos, dv); */
/*     } */
/*     VECTOR_PUSH(result, cur); */
/*   } */
/*   free(list); */
/*   return result; */
/* } */

#define FIELD_SIZE(struct_type, field) (sizeof( ((struct_type*)NULL)->field ))

Definitions *forms_get_definitions(dictData *data, const char *key, const char *search_word, const M_Form m_form) {
  const char *word_compare = !strcmp(key, search_word) ? EMPTY_STR : search_word;
  Definitions *result=NULL;
  char  *list = dict_data_read_(data, m_form.start, m_form.end - m_form.start, NULL, NULL);
  void *p = list;
  uint16_t vN = read_short_mm(&p);
  for (uint16_t i = 0; i < vN; i++) {
    const char *word = read_string_mm(&p);
    uint16_t dN = read_short_mm(&p);
    size_t skip= dN*(FIELD_SIZE(Definitions, id)+FIELD_SIZE(Definitions, offset)+FIELD_SIZE(Definitions, size));
    if(!strcmp(word, word_compare)){
      result = VECTOR_RESIZE(result, dN);
      for (uint16_t j = 0; j < dN; j++) {
	result[j].id = read_short_mm(&p);
	result[j].offset = read_long_mm(&p);
	result[j].size = read_long_mm(&p);
      }
      break;
    }
    else{
      p =(char *)p + skip;
    }
  }
  free(list);
  return result;
}

char **forms_get_words(dictData *data, const char *key, const M_Form m_form) {
  char **result=NULL;
  char  *list = dict_data_read_(data, m_form.start, m_form.end - m_form.start, NULL, NULL);
  void *p = list;
  uint16_t vN = read_short_mm(&p);
  result = VECTOR_RESIZE(result, vN);
  for (uint16_t i = 0; i < vN; i++) {
    const char *word = read_string_mm(&p);
    result[i] = xstrdup(!strcmp(word, EMPTY_STR) ? key : word);
    uint16_t dN = read_short_mm(&p);
    size_t skip= dN*(FIELD_SIZE(Definitions, id)+FIELD_SIZE(Definitions, offset)+FIELD_SIZE(Definitions, size));
    p =(char *)p + skip;
    }
  free(list);
  return result;
}


/* void formsIterator_free(FormsIterator *iterator){ */
/*   free(iterator->key); */
/*   free(iterator->list); */
/* } */



/* const char *formsIterator_next(FormsIterator *iterator) { */
/*   if(!iterator->vN) */
/*     { */
/*       iterator->dN=0; */
/*       return NULL; */
/*     } */
/*   size_t skip= iterator->dN*(FIELD_SIZE(Definitions, id)+FIELD_SIZE(Definitions, offset)+FIELD_SIZE(Definitions, size)); */
/*   iterator->p =(char *)iterator->p + skip; */
/*   const char *word = read_string_mm(&iterator->p); */
/*   const char *result = !strcmp(word, EMPTY_STR) ? iterator->key : word; */
/*   iterator->dN = read_short_mm(&iterator->p); */
/*   (iterator->vN)--; */
/*   return result; */
/* } */

/* Definitions *formsIterator_getDefinitions(FormsIterator *iterator) { */
/*   Definitions *result=NULL; */
/*   if(iterator->dN){ */
/*     result=VECTOR_RESIZE(result, iterator->dN); */
/*     for (uint16_t j = 0; j < iterator->dN; j++) { */
/*       result[j].id = read_short_mm(&iterator->p); */
/*       result[j].offset = read_long_mm(&iterator->p); */
/*       result[j].size = read_long_mm(&iterator->p); */
/*     } */
/*   } */
/*   iterator->dN = 0; */
/*   return result; */
/* } */
