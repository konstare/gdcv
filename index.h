/* We use radix tree to build index file.
 * The implementation  is based on the one of libkmod (interface to kernel module operations )
 * (https://git.kernel.org/pub/scm/utils/kernel/kmod/kmod.git)
 *
 *
 * Few bugs are introduced.
 * First, all dictionary files with *dsl.dz extension are found in the given directories.
 * These files are read with gzread line by line, all words are going through Unicode normalization (utf8proc library is used).
 * The index  contains two files. index_value.db and index_key.
 *
 *
 *
 * Disk format for index_value.db:
 *
 *
 * char *word //null terminated 
 * all words are grouped by their unicode normalisation form, so: dog,DOG, Dög are in one group. 
 * To save space,  if a word is equal to the key then it is skipped.
 *
 *
 * char * dictionary_id:start_offset:end_offset: //null terminated
 * start_offset and end_offset are positions of the word translation in dictionary file.
 * The file is compressed with dictzip.
 *
 *
 *
 *
 * Disk format for index_key.db:
 *
 *  uint32_t magic = INDEX_MAGIC;
 *  uint32_t version = INDEX_VERSION;
 *
 *
 *  List of dictionaries:
 *
 *  uint32_t dictionary_id;
 *  char* basename;//null terminated
 *  char*  filename, abbreviation_file,icon,zip_file_with_resources,name,translation_from,translation_to; //null terminated
 *
 *
 *  uint32_t root_offset;
 *
 *  (node_offset & INDEX_NODE_MASK) specifies the file offset of nodes:
 *
 *       char[] prefix; // nul terminated
 *       
 *       unsigned char N; //number of childrens
 *       char children[N];
 *       uint32_t children[N];
 *
 *       unsigned char v_N; //number of values.
 *        
 *       long start; 
 *       long end; 
 *       start and end position for list of words similar to the key and the list of dictionaries in the file index_value.db
 *
 *
 *  (node_offset & INDEX_NODE_FLAGS) indicates which fields are present.
 *  Empty prefixes are omitted, leaf nodes omit the three child-related fields.
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

/* Format of node offsets within index file */

#ifndef _INDEX_H
#define _INDEX_H

#include <stdio.h> //FILE
#include <stdint.h> //uint32_t
#include <sys/stat.h> //stat

#include "dictzip.h"


struct childrens {
  char ch;		/* path compression */
  struct index_node *node; /* indexed by character */
};

struct index_value{
  char *word;
  char *value;
};

struct index_node
{
  char *prefix;		/* path compression */
  char *key;
  int v_N;  //Number of values,
  struct index_value *values;
  unsigned char c_N; //Number of childrens,
  struct  childrens *childrens; //array of childrens.
}; 



enum DICTIONARY_STRUCTURE {
    DICTIONARY_FILE,
    FILE_ABRV,
    ICON,
    RESOURCES,
    NAME,
    FROM,
    TO,
    DATA
};

struct dictionary {
  char *base;
  int id;
  char *data[7]; //0 -.dls.dz// 1-_abrv.dls//2-.bmp//3-.dls.dz.zip//4-NAME//5-FROM//6-TO
  char *encoding;
  int to_index;
} ;

struct index_mm {
  void *mm;
  uint32_t root_offset;
  size_t size;
  int N;
  struct dictionary *Dic;
  dictData * dz;
};

struct root_
{
  struct dictionary *Dic;
  int N;
  int indexed;
};

struct article {
  char *word;
  char *definition;
  char *full_definition;
  int dic;
  size_t start;
  size_t size;
};

struct string {
  size_t len;
  char *str;
};

struct search_results
{
  int *count;
  struct string buf;
  struct string *res;
  size_t *max_len;
  int max_results;
};



extern struct root_ index_directories (char ** dir_list);

extern struct index_node *index_create(struct root_ D);

extern void index_destroy(struct index_node *node);

extern void root_destroy(struct root_ D);

extern void index_write(const struct index_node *node, struct root_ D, char *index_path);


extern struct index_mm *index_mm_open( char *index_path);


extern void index_mm_close(struct index_mm *idx);

extern int decode_articles(char *word,struct article **cards,int *cards_number, struct index_mm *index);

extern struct dictionary * get_dictionary(struct dictionary *Dic, int N, int id);

extern char * word_fetch(struct dictionary * Dic, int start, int size);



extern void look_for_a_word(struct index_mm *index,char *word,int *art, struct article **Art,int is_prefix);


#endif
