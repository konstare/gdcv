/* The core of the program is the index of the file based on the Patricia trie.
 * The index implementation is modified version of the Linux kernel's
 *  libkmod-index.c file -  module index file implementation
 *
 * Integers are stored as 32 bit unsigned in "network" order, i.e. MSB first.
 * All files start with a magic number.
 *
 * Magic spells "STEEL". 
 *
 * We use a version string to keep track of changes to the binary format
 */

/* Disk format:
 *
 *  uint32_t magic = INDEX_MAGIC;
 *  uint32_t version = INDEX_VERSION;
 *  uint32_t root_offset;
 *
 *  (node_offset & INDEX_NODE_MASK) specifies the file offset of nodes:
 *
 *       char[] prefix; // nul terminated
 *
 *       char first;
 *       char last;
 *       uint32_t children[last - first + 1];
 *
 *       uint32_t value_count;
 *       struct {
 *           uint32_t priority;
 *           char[] value; // nul terminated
 *       } values[value_count];
 *
 *  (node_offset & INDEX_NODE_FLAGS) indicates which fields are present.
 *  Empty prefixes are omitted, leaf nodes omit the three child-related fields.
 *
 *  This could be optimised further by adding a sparse child format
 *  (indicated using a new flag).
 *
 *
 * Implementation is based on a radix tree, or "trie".
 * Each arc from parent to child is labelled with a character.
 * Each path from the root represents a string.
 *
 * == Example strings ==
 *
 * ask
 * ate
 * on
 * once
 * one
 *
 * == Key ==
 *  + Normal node
 *  * Marked node, representing a key and it's values.
 *
 * +
 * |-a-+-s-+-k-*
 * |   |
 * |   `-t-+-e-*
 * |
 * `-o-+-n-*-c-+-e-*
 *         |
 *         `-e-*
 *
 * Naive implementations tend to be very space inefficient; child pointers
 * are stored in arrays indexed by character, but most child pointers are null.
 *
 * Our implementation uses a scheme described by Wikipedia as a Patrica trie,
 *
 *     "easiest to understand as a space-optimized trie where
 *      each node with only one child is merged with its child"
 *
 * +
 * |-a-+-sk-*
 * |   |
 * |   `-te-*
 * |
 * `-on-*-ce-*
 *      |
 *      `-e-*
 *
 * We still use arrays of child pointers indexed by a single character;
 * the remaining characters of the label are stored as a "prefix" in the child.
 *
 * The paper describing the original Patrica trie works on individiual bits -
 * each node has a maximum of two children, which increases space efficiency.
 * However for this application it is simpler to use the ASCII character set.
 * Since the index file is read-only, it can be compressed by omitting null
 * child pointers at the start and end of arrays.
 */
#pragma once

#include <stdint.h>
#include "forms.h"
#include "dictionaries.h"


/*
  Node of a tree contains
  prefix - path compression
  childrens - VECTOR of Childrens
  forms - VECTOR of  different forms of a word
 */
typedef struct {
  char *prefix;                /* path compression */
  struct childrens *childrens;  /* vector of childrens.*/
  Form *forms;             /*vector of  different forms of a word*/
} Node;

/*
  Children node start with a char ch.
  The structure is use in both cases - reading index from memory and from file.
  In the first case the pointer Node *node is used,
  in the second case - uint32_t offset in the file. 
 */
typedef struct childrens {
  char ch;
  union{
    Node *node;  /* index in memory */
    uint32_t offset; /* index is read from file */
  };
} Childrens;


/*
  Insert word into index. node is the pointer to the root. word contains utf_cleaned key and definition.  
*/
extern int index_insert(Node *node, const WordDefinition *word);
extern Node *index_create();
extern void index_free(Node *node);
extern void index_print_head();
extern void index_write(Node *root, DictionaryArray D, const char *index_path);


typedef struct DB_ *DB;
extern DB database_open(const char *path);
extern void database_close(DB idx);

extern Definitions *index_mm_get_definitions(const DB idx, const char *search_word);

extern char ** index_mm_search_prefix_forms(const DB idx, const char *word);
extern char ** index_mm_search_substring_forms(const DB idx, const char *word);
extern char ** index_mm_search_forms(const DB idx, const char *word);

