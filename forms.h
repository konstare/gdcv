#pragma once
#include "dictionaries.h"

typedef struct {
  char *word;
  Definitions *pos; /*vector of  different dictionaries with the same form of the word*/
} Form;

typedef struct M_Form {
  uint32_t start;
  uint32_t end;
} M_Form;

extern void form_free(Form *self);
int form_push(Form **self, const WordDefinition *word);
M_Form forms_write(Form *forms, FILE *out);

Definitions *forms_get_definitions(dictData *data, const char *key, const char *search_word, const M_Form m_form);
char **forms_get_words(dictData *data, const char *key, const M_Form m_form);
