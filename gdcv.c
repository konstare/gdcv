#include "dictionaries.h"
#include "index.h"
#include "utils.h"
#include "cvec.h"
#include <argp.h>   //argp
#include <libgen.h> //basename,dirname;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *argp_program_version = "gdcv 0.8";
const char *argp_program_bug_address = "https://github.com/konstare/gdcv/issues";

/* static char doc[] = "Command Line version of GoldenDict" */
/*   "\noptions:" */
/*   "\vTo look up a word \"cat\": gdcv cat.\n" */
/*   "To search words with substring \"cat\": gdcv *cat\n" */
/*   "To index directory: gdcv -i /usr/share/dict"; */

/* /\* A description of the arguments we accept. *\/ */
/* static char args_doc[] = "WORD or -i PATH"; */

void print_function_no_dir(const char *str){
  printf("The directory or file %s does not exist\n", str);
}

int main(int argc, const char *argv[]) {
  /* RealPaths dirs = realpath_init(argc, argv); */
  /* realpath_print_not_exist(dirs, print_function_no_dir); */
  /* DictionaryArray D = dictionary_array_init(dirs); */

  /* Dictionary *p; */
  /* Dictionary **array= dictionary_array_get(D); */
  /* for (size_t i = 0; i < VECTOR_SIZE(array); i++) { */
  /*   p = array[i]; */
  /*   printf("%s\t\t%s\t\t%d\n", p->name, basename(p->words_file), p->to_index); */
  /* } */
  /* uint dN = 0; */
  /* Node *root = index_create(); */
  /* WordDefinition result = worddefinition_init(); */
  /* for (size_t i = 0; i < VECTOR_SIZE(array); i++) { */
  /*   p = array[i]; */
  /*   if (p->to_index) { */
  /*     p->id = dN; */
  /*     dictionary_open(p); */
  /*     printf("%s\n", p->name); */
  /*     while (p->next_word(p, &result)) { */
  /*       worddefinition_utf8_cleaning(&result); */
  /*       result.def.id = p->id; */
  /*       index_insert(root, &result); */
  /*       worddefinition_free(&result); */
  /*     } */
  /*     dN++; */
  /*     dictionary_close(p); */
  /*   } */
  /* } */

  /* printf("WRITE\nWRITE\nWRITE\n"); */
  /* index_write(root, D, "/tmp/GD"); */
  /* index_free(root); */
  /* dictionary_array_free(D); */
  /* realpath_free(dirs); */

  /*   //6756787 */
  /*   //6756342 */
  //work as a librarian
  DB idx = database_open("../index_db");
  char **result2 = index_mm_search_forms(idx, "*b*");
  for(size_t i=0; i<VECTOR_SIZE(result2); i++){
    printf("%s\n", result2[i]);
    free(result2[i]);
  }
  VECTOR_FREE(result2);
database_close(idx);
}
