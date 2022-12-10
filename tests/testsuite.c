#include "../dictionaries.h"
#include "../utils.h"
#include "test.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../index.h"
#include "../cvec.h"

UNIT_TEST(AppendExtension_1) {
  char basename[255] = "/##AAA/aaa/2";
  char *result = append_extension(basename, ".dict");
  test_assert(!result);
  free(result);
}

UNIT_TEST(AppendExtension_2) {
  char basename[255] = "./Dicts/dictd_example";
  char *result = append_extension(basename, ".index");
  test_assert(!strcmp(result, "./Dicts/dictd_example.index"));
  free(result);
}

UNIT_TEST(AppendExtension_3) {
  char basename[255] = "./Dicts/dictd_example.index";
  char *result = append_extension(basename, ".index");
  test_assert(!strcmp(basename, "./Dicts/dictd_example.index"));
  free(result);
}

UNIT_TEST(RealPath_) {
  int argc = 2;
  const char *argv[] = {"./Dicts/dictd_example.in",
                        "./Dicts/dictd_example.index"};
  RealPaths result = realpath_init(argc, argv);
  char **dirs = realpath_get_dirs(result);
  char **not_exist = realpath_get_not_exist(result);
  test_assert(
      strstr(dirs[0], "/tests/Dicts/dictd_example.index")&&
      !strcmp(not_exist[0], "./Dicts/dictd_example.in"));
  realpath_free(result);
}

UNIT_TEST(DictdCreate_) {
  const char *filename = "./Dicts/dictd_example.index";
  Dictionary *interface = dictd_create(filename);
  test_assert(interface->to_index);
  interface->free(interface);
}

size_t check_word(Dictionary *interface, const char *answer[],
                  size_t size) {
  WordDefinition result = worddefinition_init();
  int assert = 1;
  size_t i = 0;
  interface->open(interface);
  while (interface->next_word(interface, &result) && assert && i < size) {
    assert = !strcmp(result.word, answer[i]) ? 1 : 0;
    worddefinition_free(&result);
    i++;
  }
  if (i != size)
    assert = 0;
  interface->close(interface);
  interface->free(interface);
  return assert;
}

UNIT_TEST(DictdCreateWordList_) {
  const char *filename = "./Dicts/dictd_example.index";
  Dictionary *interface = dictd_create(filename);
  const char *answer[] = {"animal", "aNimal", "anIml", "cat", "empty definition", "plant"};
  size_t assert = check_word(interface, answer, ARRAY_LENGTH(answer));
  test_assert(assert);
}

UNIT_TEST(DictdCreate_Broken) {
  const char *filename = "./Dicts/dictd_broken.index";
  Dictionary *interface = dictd_create(filename);
  test_assert(!interface->to_index);
  interface->free(interface);
}

UNIT_TEST(DictdCreate_BrokenName) {
  const char *filename = "./Dicts/dictd_broken_name.index";
  Dictionary *interface = dictd_create(filename);
  test_assert(!interface->to_index);
  interface->free(interface);
}

UNIT_TEST(StardictCreate_) {
  const char *filename = "./Dicts/stardict_example.idx";
  Dictionary *interface = stardict_create(filename);
  test_assert(interface->to_index);
  interface->free(interface);
}

UNIT_TEST(StardictCreateSyn_) {
  const char *filename = "./Dicts/stardict_example.idx";
  Dictionary *interface = stardict_create(filename);
  const char *answer[] = {"2dimensional",   "apple",          "2dimensionale",
                          "2dimensionalem", "2dimensionalen", "2dimensionaler",
                          "2dimensionales", "apples"};
  size_t assert = check_word(interface, answer, ARRAY_LENGTH(answer));
  test_assert(assert);
}

UNIT_TEST(DslCreate_) {
  const char *filename = "./Dicts/sample.dsl";
  Dictionary *interface = dsl_create(filename);
  test_assert(interface->to_index);
  interface->free(interface);
}

UNIT_TEST(DslCreate_1) {
  const char *filename = "./Dicts/sample.dsl";
  Dictionary *interface = dsl_create(filename);
  const char *answer[] = {
      "trivial card",    "sample headword", "sample  card",  "example",
      "sample entry",    "typical card",    "trivial card2", "trivial card5",
      "trivial card4.6", "trivial card4.5", "trivial card4", "trivial card3",
      "trivial card6",   "trivial card7",   "trivial card8"};
  size_t assert = check_word(interface, answer, ARRAY_LENGTH(answer));
  test_assert(assert);
}

void *dsl_clean_word(char *str);

UNIT_TEST(DslClean_1) {
  char test[] = "ad{{ad}a}s{dz}d{q}\n";
  dsl_clean_word(test);
  test_assert(!strcmp(test, "adsd"));
}

UNIT_TEST(DslClean_2) {
  char test[] = "ad\\{{ad}Z\\}s\\\\\\{dzd}{q}\n";
  const char answer[] = "ad{Z}s\\{dzd";
  dsl_clean_word(test);
  test_assert(!strcmp(test, answer));
}

UNIT_TEST(DslClean_3) {
  char test[] = "ad\\{{ad}Z\\}s\\\\\\{dzd}{q}\r\n";
  const char answer[] = "ad{Z}s\\{dzd";
  dsl_clean_word(test);
  test_assert(!strcmp(test, answer));
}

UNIT_TEST(DslClean_4) {
  char test[] = "\\{af\\} afe\n";
  const char answer[] = "{af} afe";
  dsl_clean_word(test);
  test_assert(!strcmp(test, answer));
}


UNIT_TEST(IndexCreate) {
  Node *root;
  root = index_create();
  test_assert(VECTOR_SIZE(root->childrens) == 0 && VECTOR_SIZE(root->forms) == 0 && root->childrens == NULL &&
              !strcmp(root->prefix, ""));
  index_free(root);
}

Node *create_trie(const char *words[], const uint len) {
  Node *root = index_create();
  WordDefinition word = worddefinition_init();
  for (uint i = 0; i < len; i++) {
    word.def.offset = i;
    word.def.size = i + len;
    word.def.id = i + 2 * len;
    word.word = xstrdup(words[i]);
    worddefinition_utf8_cleaning(&word);
    index_insert(root, &word);
    worddefinition_free(&word);
  }
  return root;
}

Node *create_trie_default() {
  const char *words[] = {"romane", "romanus", "romulus",   "rubens",
                         "ruber",  "rubicon", "rubicundus"};
  const uint len = ARRAY_LENGTH(words);
  return create_trie(words, len);
}

UNIT_TEST(IndexRoot) {
  Node *root = create_trie_default();
  test_assert(VECTOR_SIZE(root->childrens) == 1 && VECTOR_SIZE(root->forms) == 0 && !strcmp(root->prefix, ""));
  index_free(root);
}

UNIT_TEST(IndexChildren1) {
  Node *root = create_trie_default();
  test_assert(root->childrens[0].ch == 'r');
  index_free(root);
}
UNIT_TEST(IndexChildren2) {
  Node *root = create_trie_default();
  Node *child1 = root->childrens[0].node;
  test_assert(VECTOR_SIZE(child1->childrens) == 2 && VECTOR_SIZE(child1->forms) == 0 &&
              !strcmp(child1->prefix, "") && child1->childrens[0].ch == 'o' &&
              child1->childrens[1].ch == 'u');
  index_free(root);
}

UNIT_TEST(IndexChildren3) {
  Node *root = create_trie_default();
  Node *child2 = root->childrens[0].node->childrens[0].node;
  test_assert(VECTOR_SIZE(child2->childrens) == 2 && VECTOR_SIZE(child2->forms) == 0 &&
              !strcmp(child2->prefix, "m") && child2->childrens[0].ch == 'a' &&
              child2->childrens[1].ch == 'u');
  index_free(root);
}

UNIT_TEST(IndexChildren4) {
  Node *root = create_trie_default();
  Node *child3 = root->childrens[0].node->childrens[0].node->childrens[1].node;
  test_assert(!strcmp(child3->prefix, "lus") &&  child3->forms[0].pos->id == 16);
  index_free(root);
}

UNIT_TEST(IndexSameWords) {
  const char *words[] = {"romane", "romane", "romane", "romane", "romane"};
  const uint len = ARRAY_LENGTH(words);
  Node *root = create_trie(words, len);
  Node *child = root->childrens[0].node;
  Form *value = &child->forms[0];
  test_assert(VECTOR_SIZE(child->forms) == 1 && VECTOR_SIZE(value->pos) == 5 && !strcmp(value->word, ""));
  index_free(root);
}

UNIT_TEST(IndexSameUTFWords) {
  const char *words[] = {"romane", "römane", "romäne", "romanë", "romänë"};
  const uint len = ARRAY_LENGTH(words);
  Node *root = create_trie(words, len);
  Node *child = root->childrens[0].node;
  Form *value = &child->forms[4];
  test_assert(VECTOR_SIZE(child->forms) == 5 && VECTOR_SIZE(value->pos) == 1 &&
              !strcmp(value->word, "romänë") &&
              !strcmp(child->forms[0].word, ""));
  index_free(root);
}

UNIT_TEST(SaveDictionaryandLoad) {
  char buffer[CHAR_BUFFER_SIZE];
  const char *argv[] = {"./Dicts/dictd_example.index", "./Dicts/stardict_example.idx", "./Dicts/sample.dsl"};
  RealPaths realpath = realpath_init(ARRAY_LENGTH(argv), argv);
  DictionaryArray origin_array = dictionary_array_init(realpath);
  FILE *fd =tmpfile();
  dictionary_array_write(origin_array, fd);
  realpath_free(realpath);
  rewind(fd);
  char *p=buffer;
  char ch;
  while((ch = fgetc(fd)) != EOF) {
    *p=ch;
    p++;
  }
  DictionaryArray result_array = dictionary_array_load(buffer);

  Dictionary **origin = dictionary_array_get(origin_array);
  Dictionary **result= dictionary_array_get(result_array);
  
  test_assert(!strcmp(origin[1]->name, result[1]->name)&&
	      !strcmp(origin[0]->name, result[0]->name) &&
	      !strcmp(origin[0]->words_file, result[0]->words_file)&&
	      !strcmp(origin[1]->words_file, result[1]->words_file)&&
	      !strcmp(origin[2]->words_file, result[2]->words_file)&&
	      !strcmp(origin[2]->name, result[2]->name));
  fclose(fd);
  dictionary_array_free(result_array);
  dictionary_array_free(origin_array);
}

/* WordDefinition *word_get_forms(const DB idx, const char *search_word); */

void create_index(){
  const char *argv[] = {"./Dicts/dictd_example.index", "./Dicts/dictd_example2.index"};
  RealPaths realpath = realpath_init(ARRAY_LENGTH(argv), argv);
  DictionaryArray d = dictionary_array_init(realpath);
  uint dN = 0;
  Node *root = index_create();
  Dictionary *p;
  WordDefinition result = worddefinition_init();
  Dictionary **dictionary=dictionary_array_get(d);
  for (size_t i = 0; i < VECTOR_SIZE(dictionary); i++) {
    p = dictionary[i];
    if (p->to_index) {
      p->id = dN;
      dictionary_open(p);
      while (p->next_word(p, &result)) {
        worddefinition_utf8_cleaning(&result);
        result.def.id = p->id;
        index_insert(root, &result);
        worddefinition_free(&result);
      }
      dN++;
      dictionary_close(p);
    }
  }
  index_write(root, d, "/tmp/");
  index_free(root);
  dictionary_array_free(d);
  realpath_free(realpath);
}

Definitions *index_mm_get_definitions(const DB idx, const char *search_word);

UNIT_TEST(IndexLoad) {
  create_index();
  DB db = database_open("/tmp");

  Definitions *result = index_mm_get_definitions(db, "animal");
  Definitions *result2 = index_mm_get_definitions(db, "xempt");
  Definitions *result3 = index_mm_get_definitions(db, "empt");
  Definitions *result4 = index_mm_get_definitions(db, "aNimal");

  bool assert =true;

  assert &= VECTOR_SIZE(result)==2;
  assert &=result[0].size == 40;
  assert &=result[0].offset == 712;

  assert &= VECTOR_SIZE(result2)==0;

  assert &= VECTOR_SIZE(result3)==0;

  assert &= VECTOR_SIZE(result4)==2;
  //  assert &=result4[0].size == 40;
  // assert &=result4[0].offset == 712;
  
  test_assert(assert);
  
  VECTOR_FREE(result);
  VECTOR_FREE(result2);
  VECTOR_FREE(result3);
  VECTOR_FREE(result4);
  
  database_close(db);
}

char ** index_mm_search_all_forms(const DB idx);

static bool check_index_search(char **result, const char *answer[], size_t len){
  bool assert =true;
  assert &= VECTOR_SIZE(result)== len;
  if (assert)
    for(uint i=0;i<VECTOR_SIZE(result); i++){
      //    printf("%s, %s\n", result[i], answer[i]);
      assert &=!strcmp(result[i], answer[i]);
    }

  for(uint i=0;i<VECTOR_SIZE(result); i++){
    free(result[i]);
  }
  VECTOR_FREE(result);
  return assert;
}

UNIT_TEST(IndexSearchAll) {
  create_index();
  DB db = database_open("/tmp");
  bool assert =true;

  char **result = index_mm_search_all_forms(db);
  const char *answer[]={"animal", "aNimal", "anIml", "cat", "empty definition", "plant"};
  assert &=check_index_search(result, answer, ARRAY_LENGTH(answer));
  
  test_assert(assert);
  database_close(db);
}

UNIT_TEST(IndexSearchPrefix) {
  create_index();
  DB db = database_open("/tmp");
  bool assert =true;
  
  char **result = index_mm_search_prefix_forms(db, "an");
  const char *answer[]={"animal", "aNimal", "anIml"};
  assert &=check_index_search(result, answer, ARRAY_LENGTH(answer));

  result = index_mm_search_prefix_forms(db, "empty");
  const char *answer2[]={"empty definition"};
  assert &=check_index_search(result, answer2, ARRAY_LENGTH(answer2));

  test_assert(assert);
  database_close(db);
}


UNIT_TEST(IndexSearchSubstring) {
  create_index();
  DB db = database_open("/tmp");
  bool assert =true;

  char **result = index_mm_search_substring_forms(db, "nim");
  const char *answer[]={"animal", "aNimal", "anIml"};
  assert &=check_index_search(result, answer, ARRAY_LENGTH(answer));


  result = index_mm_search_substring_forms(db, "an");
  const char *answer2[]={"animal", "aNimal", "anIml",  "plant"};
  assert &=check_index_search(result, answer2, ARRAY_LENGTH(answer2));
  
  test_assert(assert);
  
  database_close(db);
}


char **key_parse(const char *str);

bool check_keys(char **string, const char *answer[], size_t len){
  bool assert = true;
  assert &=VECTOR_SIZE(string)==len;
  for(size_t i=0; i<VECTOR_SIZE(string); i++){
    assert &= !strcmp(string[i], answer[i]);
    VECTOR_FREE(string[i]);
  }
  VECTOR_FREE(string);
  return assert;
}

UNIT_TEST(KeyParse) {
  bool assert = true;
  char **key;
  const char *answer[]={"c", "arm", ""};
  key = key_parse("c*arm*");
  assert &=check_keys(key, answer, ARRAY_LENGTH(answer));

  key = key_parse("c*arm");
  const char *answer2[]={"c", "arm"};
  assert &=check_keys(key, answer2, ARRAY_LENGTH(answer2));

  key = key_parse("*");
  const char *answer3[]={"", ""};
  assert &=check_keys(key, answer3, ARRAY_LENGTH(answer3));

  key = key_parse("csd\\*");
  const char *answer4[]={"csd*"};
  assert &=check_keys(key, answer4, ARRAY_LENGTH(answer4));

  key = key_parse("\\*");
  const char *answer5[]={"*"};
  assert &=check_keys(key, answer5, ARRAY_LENGTH(answer5));

  key = key_parse("\\**");
  const char *answer6[]={"*",""};
  assert &=check_keys(key, answer6, ARRAY_LENGTH(answer6));

  const char *answer7[]={"c", "arm", ""};
  key = key_parse("c*a\\rm*");
  assert &=check_keys(key, answer7, ARRAY_LENGTH(answer7));

  const char *answer8[]={"", "nim", ""};
  key = key_parse("*nim*");
  assert &=check_keys(key, answer8, ARRAY_LENGTH(answer8));

  
  test_assert(assert);
}

UNIT_TEST(IndexSearchKey) {
  create_index();
  DB db = database_open("/tmp");
  bool assert =true;
  char **result;
  /* result = index_mm_search_forms(db, "*nim*"); */
  /*  const char *answer[]={"animal", "aNimal", "anIml"}; */
  /* assert &=check_index_search(result, answer, ARRAY_LENGTH(answer)); */


  result = index_mm_search_forms(db, "an*");
  const char *answer2[]={"animal", "aNimal", "anIml"};
  assert &=check_index_search(result, answer2, ARRAY_LENGTH(answer2));

 result = index_mm_search_forms(db, "*nt");
 const char *answer3[]={"plant"};
 assert &=check_index_search(result, answer3, ARRAY_LENGTH(answer3));

 result = index_mm_search_forms(db, "*a*t");
 const char *answer4[]={"cat","plant"};
 assert &=check_index_search(result, answer4, ARRAY_LENGTH(answer4));

 result = index_mm_search_forms(db, "*");
 const char *answer5[]={"animal", "aNimal", "anIml", "cat", "empty definition", "plant"};
 assert &=check_index_search(result, answer5, ARRAY_LENGTH(answer5));

 result = index_mm_search_forms(db, "aaaaa*aaaaaa");
 const char *answer6[]={"ISO C forbids zero-size array"};
 assert &=check_index_search(result, answer6, 0);

 result = index_mm_search_forms(db, "*aaaaaa*");
 const char *answer7[]={"ISO C forbids zero-size array"};
 assert &=check_index_search(result, answer7, 0);

 result = index_mm_search_forms(db, "*aaaaaa");
 const char *answer8[]={"ISO C forbids zero-size array"};
 assert &=check_index_search(result, answer8, 0);

 result = index_mm_search_forms(db, "aaaaaa*");
 const char *answer9[]={"ISO C forbids zero-size array"};
 assert &=check_index_search(result, answer9, 0);

  
  test_assert(assert);
  
  database_close(db);
}

/* UNIT_TEST(HugeIndexSearchKey) { */
/*   create_index(); */
/*   DB db = database_open("Huge_Index"); */
/*   bool assert =true; */
/*   char **result; */

/*   result = index_mm_search_forms(db, "a*z"); */
/*   printf("%ld",VECTOR_SIZE(result)); */
/*   for(size_t i=0; i<VECTOR_SIZE(result); i++){ */
/*     printf("%s\n", result[i]); */
/*     free(result[i]); */
/*   } */
/*   assert &=VECTOR_SIZE(result) == 46; */
/*   test_assert(assert); */

/*   VECTOR_FREE(result); */
/*   database_close(db); */
/* } */
