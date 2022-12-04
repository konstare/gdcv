#include "dictionaries.h"
#include <stdint.h>


typedef  struct {
  uint16_t dictionary_number;
  uint32_t offset;
  uint32_t size;
} PositionDictionary;

typedef struct {
  char *word;
  PositionDictionary *pos; /*vector of  different dictionaries with the same form of a word*/
} Value;

typedef struct {
  char *prefix;                /* path compression */
  struct childrens *childrens; // vector of childrens.
  Value *values; /*vector of  different forms of a key*/
} Node;

typedef struct childrens {
  char ch;
  union{
    Node *node;  /* index in memory */
    uint32_t offset; /* index is read from file */
  };
} Childrens;

uint32_t index_write__node(const Node *node, FILE *out, FILE *value_out);
extern int index_insert(Node *node, const WordDefinition *word);
extern Node *index_create();
extern void index_free(Node *node);
extern void index_print_head();
extern void index_write(Node *root, DictionaryArray *D, const char *index_path);
typedef struct DB DB;
extern DB *DB_open(const char *path);
extern void DB_close(DB *idx);
