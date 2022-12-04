#include "dictionaries.h"
#include "index.h"
#include "utils.h"
#include "cvec.h"
#include <argp.h>   //argp
#include <libgen.h> //basename,dirname;
#include <stdio.h>
#include <stdlib.h>

const char *argp_program_version = "gdcv 0.5";
const char *argp_program_bug_address = "reichcv@gmail.com";

/* static char doc[] = "Command Line version of GoldenDict" */
/*   "\noptions:" */
/*   "\vTo look up a word \"cat\": gdcv cat.\n" */
/*   "To search words with substring \"cat\": gdcv *cat\n" */
/*   "To index directory: gdcv -i /usr/share/dict"; */

/* /\* A description of the arguments we accept. *\/ */
/* static char args_doc[] = "WORD or -i PATH"; */

void print_function(const char *str){
  printf("The directory or file %s does not exist\n", str);
}

int main(int argc, const char *argv[]) {
  RealPaths dirs = realpath_init(argc, argv);
  realpath_print_not_exist(dirs, print_function);
  DictionaryArray D = dictionary_array_init(dirs);

  Dictionary *p;
  for (size_t i = 0; i < VECTOR_SIZE(D.dictionary); i++) {
    p = D.dictionary[i];
    printf("%s\t\t%s\t\t%d\n", p->name, basename(p->words_file), p->to_index);
  }
  uint dN = 0;
  Node *root = index_create();
  WordDefinition result = worddefinition_init();
  for (size_t i = 0; i < VECTOR_SIZE(D.dictionary); i++) {
    p = D.dictionary[i];
    if (p->to_index) {
      p->id = dN;
      dictionary_open(p);
      printf("%s\n", p->name);
      while (p->next_word(p, &result)) {
        worddefinition_utf8_cleaning(&result);
        result.dictionary_number = p->id;
        index_insert(root, &result);
        worddefinition_free(&result);
      }
      dN++;
      dictionary_close(p);
    }
  }

  printf("WRITE\nWRITE\nWRITE\n");
  index_write(root, &D, "/tmp/");
  index_free(root);
  dictionary_array_free(D);
  realpath_free(dirs);
  
  DB *result2 = DB_open("/tmp/");
  DB_close(result2);
}
