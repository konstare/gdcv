#ifndef _INDEX_H
#define _INDEX_H

#include <stdio.h> //FILE
#include <stdint.h> //uint32_t
#include <sys/stat.h> //stat



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

extern void index_write(const struct index_node *node, struct root_ D, FILE *out);


extern struct index_mm *index_mm_open( const char *filename, struct stat *stamp);


extern void index_mm_close(struct index_mm *idx);

extern int decode_articles(char *word,struct article **cards,int *cards_number, struct index_mm *index);

extern struct dictionary * get_dictionary(struct dictionary *Dic, int N, int id);

extern char * word_fetch(struct dictionary * Dic, int start, int size);



extern void look_for_a_word(struct index_mm *index,char *word,int *art, struct article **Art,int is_prefix);


#endif
