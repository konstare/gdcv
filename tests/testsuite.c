#include "../dictionaries.h"
#include "../utils.h"
#include "test.h"
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
  test_assert(
      strstr(result.dirs[0], "/tests/Dicts/dictd_example.index")&&
      !strcmp(result.not_exist[0],
              "./Dicts/dictd_example.in"));
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
  const char *answer[] = {"animal", "cat", "empty definition", "plant"};
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


UNIT_TEST(IndexCreate) {
  Node *root;
  root = index_create();
  test_assert(VECTOR_SIZE(root->childrens) == 0 && VECTOR_SIZE(root->values) == 0 && root->childrens == NULL &&
              !strcmp(root->prefix, ""));
  index_free(root);
}

Node *create_trie(const char *words[], const uint len) {
  Node *root = index_create();
  WordDefinition word = worddefinition_init();
  for (uint i = 0; i < len; i++) {
    word.offset = i;
    word.size = i + len;
    word.dictionary_number = i + 2 * len;
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
  test_assert(VECTOR_SIZE(root->childrens) == 1 && VECTOR_SIZE(root->values) == 0 && !strcmp(root->prefix, ""));
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
  test_assert(VECTOR_SIZE(child1->childrens) == 2 && VECTOR_SIZE(child1->values) == 0 &&
              !strcmp(child1->prefix, "") && child1->childrens[0].ch == 'o' &&
              child1->childrens[1].ch == 'u');
  index_free(root);
}

UNIT_TEST(IndexChildren3) {
  Node *root = create_trie_default();
  Node *child2 = root->childrens[0].node->childrens[0].node;
  test_assert(VECTOR_SIZE(child2->childrens) == 2 && VECTOR_SIZE(child2->values) == 0 &&
              !strcmp(child2->prefix, "m") && child2->childrens[0].ch == 'a' &&
              child2->childrens[1].ch == 'u');
  index_free(root);
}

UNIT_TEST(IndexChildren4) {
  Node *root = create_trie_default();
  Node *child3 = root->childrens[0].node->childrens[0].node->childrens[1].node;
  test_assert(!strcmp(child3->prefix, "lus") &&
              child3->values[0].pos->dictionary_number == 16);
  index_free(root);
}

UNIT_TEST(IndexSameWords) {
  const char *words[] = {"romane", "romane", "romane", "romane", "romane"};
  const uint len = ARRAY_LENGTH(words);
  Node *root = create_trie(words, len);
  Node *child = root->childrens[0].node;
  Value *value = &child->values[0];
  test_assert(VECTOR_SIZE(child->values) == 1 && VECTOR_SIZE(value->pos) == 5 && !strcmp(value->word, ""));
  index_free(root);
}

UNIT_TEST(IndexSameUTFWords) {
  const char *words[] = {"romane", "römane", "romäne", "romanë", "romänë"};
  const uint len = ARRAY_LENGTH(words);
  Node *root = create_trie(words, len);
  Node *child = root->childrens[0].node;
  Value *value = &child->values[4];
  test_assert(VECTOR_SIZE(child->values) == 5 && VECTOR_SIZE(value->pos) == 1 &&
              !strcmp(value->word, "romänë") &&
              !strcmp(child->values[0].word, ""));
  index_free(root);
}

UNIT_TEST(SaveDictionaryandLoad) {
  char buffer[CHAR_BUFFER_SIZE];
  const char *argv[] = {"./Dicts/dictd_example.index", "./Dicts/stardict_example.idx", "./Dicts/sample.dsl"};
  RealPaths realpath = realpath_init(ARRAY_LENGTH(argv), argv);
  DictionaryArray d = dictionary_array_init(realpath);
  FILE *fd =tmpfile();
  dictionary_array_write(&d, fd);
  realpath_free(realpath);
  rewind(fd);
  char *p=buffer;
  char ch;
  while((ch = fgetc(fd)) != EOF) {
    *p=ch;
    p++;
  }
  DictionaryArray d2 = dictionary_array_load(buffer);
  test_assert(!strcmp(d.dictionary[1]->name, d2.dictionary[1]->name)&&
	      !strcmp(d.dictionary[0]->name, d2.dictionary[0]->name) &&
	      !strcmp(d.dictionary[0]->words_file, d2.dictionary[0]->words_file)&&
	      !strcmp(d.dictionary[1]->words_file, d2.dictionary[1]->words_file)&&
	      !strcmp(d.dictionary[2]->words_file, d2.dictionary[2]->words_file)&&
	      !strcmp(d.dictionary[2]->name, d2.dictionary[2]->name));
  fclose(fd);
  dictionary_array_free(d);
  dictionary_array_free(d2);
}

UNIT_TEST(SaveIndexLoad) {
  const char *argv[] = {"./Dicts/dictd_example.index"};
  RealPaths realpath = realpath_init(ARRAY_LENGTH(argv), argv);
  DictionaryArray d = dictionary_array_init(realpath);
  uint dN = 0;
  Node *root = index_create();
  Dictionary *p;
  WordDefinition result = worddefinition_init();
  for (size_t i = 0; i < VECTOR_SIZE(d.dictionary); i++) {
    p = d.dictionary[i];
    if (p->to_index) {
      p->id = dN;
      dictionary_open(p);
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
  index_write(root, &d, "/tmp/");
  index_free(root);
  dictionary_array_free(d);
  realpath_free(realpath);

  DB* db = DB_open("/tmp");

  
  DB_close(db);

}
