#include "index.h"
#include "dictionaries.h"
#include "dictzip.h"
#include "utils.h"
#include "cvec.h"
#include <arpa/inet.h> //htonl
#include <linux/limits.h>
#include <netinet/in.h>
#include <search.h> //lsearch
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define INDEX_MAGIC 0x57EE1
#define INDEX_VERSION_MAJOR 0x0004
#define INDEX_VERSION_MINOR 0x0001
#define INDEX_VERSION ((INDEX_VERSION_MAJOR << 16) | INDEX_VERSION_MINOR)

#define INDEX_NODE_PREFIX 0x80000000
#define INDEX_NODE_VALUES 0x40000000
#define INDEX_NODE_CHILDS 0x20000000
#define INDEX_NODE_MASK 0x1FFFFFFF


static void value_free(Value *result) {
  free(result->word);
  VECTOR_FREE(result->pos);
}

static int comp_ch(const void *a1, const void *a2) {
  const struct childrens *k1 = a1;
  const struct childrens *k2 = a2;
  return k1->ch - k2->ch;
}

static int comp_word(const void *a1, const void *a2) {
  const WordDefinition *v1 = a1;
  const Value *v2 = a2;
  return strcmp(v1->word, v2->word);
}

/* #include <assert.h> */
/* struct foo4 { */
/*   char c;      /\* 1 byte *\/ */
/*   short s;     /\* 2 bytes *\/ */
/* }; */
/* static_assert(sizeof(struct foo4) == 3, "Check your assumptions"); */

static int index_add_value(Node *node, const WordDefinition *word) {
  size_t v_N = VECTOR_SIZE(node->values);
  Value *current =
      lfind(word, node->values, &v_N, (sizeof *node->values), comp_word);

  if (!current) {
    Value temp = {.word = xstrdup(word->word), .pos=NULL};
    VECTOR_PUSH(node->values, temp);
    current = &node->values[v_N];
  }
  PositionDictionary temp = {.size = word->size,
			     .offset = word->offset,
			     .dictionary_number = word->dictionary_number};
  VECTOR_PUSH(current->pos, temp);
  return 1;
}

static inline Node *index_null_node() {
  Node *node;
  node = xcalloc(1, sizeof *node);
  return node;
}

extern Node *index_create() {
  Node *node = index_null_node();
  node->prefix = xstrdup(EMPTY_STR);
  return node;
}

extern void index_free(Node *node) {
  free(node->prefix);

  for (size_t i = 0; i < VECTOR_SIZE(node->values); i++)
    value_free(&node->values[i]);
  VECTOR_FREE(node->values);

  for (size_t i = 0; i < VECTOR_SIZE(node->childrens); i++)
    index_free(node->childrens[i].node);

  VECTOR_FREE(node->childrens);
  free(node);
}

extern int index_insert(Node *node, const WordDefinition *word) {
  int i = 0; /* index within str */
  const char *key = word->word_utf_clean;
  char ch;
  while (1) {
    int j; /* index within node->prefix */

    /* Ensure node->prefix is a prefix of &str[i].
       If it is not already, then we must split node. */
    for (j = 0; node->prefix[j]; j++) {
      ch = node->prefix[j];

      if (ch != key[i + j]) {
        char *prefix = node->prefix;
        Node *n = index_null_node();
        /* New child is copy of node with prefix[j+1..N] */
        memcpy(n, node, sizeof *n);
        n->prefix = xstrdup(&prefix[j + 1]);

        /* Parent has prefix[0..j], child at prefix[j] */
        memset(node, 0, sizeof *node);
        prefix[j] = '\0';
        node->prefix = prefix;
	Childrens temp={.ch = ch,.node =n};
	VECTOR_PUSH(node->childrens, temp);
        break;
      }
    }
    /* j is now length of node->prefix */
    i += j;

    ch = key[i];
    if (ch == '\0')
      return index_add_value(node, word);

    size_t cN = VECTOR_SIZE(node->childrens);
    Childrens *new =
        lfind(&ch, node->childrens, &cN, (sizeof *node->childrens), comp_ch);

    if (!new) {
      Childrens temp={.ch = ch,.node =index_null_node()};
      VECTOR_PUSH(node->childrens, temp);
      Node *child = node->childrens[cN].node;
      child->prefix = xstrdup(&key[i + 1]);
      index_add_value(child, word);
      return 0;
    }

    /* Descend into child node and continue */
    node = new->node;
    i++;
  }
}


static void index_write__prefix(const Node *node, FILE *out) {
  write_string(out, node->prefix);
}

static void index_write__childrens(const Node *node, FILE *out,
                                   const uint32_t *child_offs) {
  unsigned char cN = VECTOR_SIZE(node->childrens);
  fputc(cN, out);
  for (uint i = 0; i < cN; i++) {
    fputc(node->childrens[i].ch, out);
    write_long(child_offs[i], out);
  }
}


static void index_write__dic_values(const Value *value, FILE *out) {
  write_short(VECTOR_SIZE(value->pos), out);
  for (uint i = 0; i < VECTOR_SIZE(value->pos); i++) {
    write_short(value->pos[i].dictionary_number, out);
    write_long(value->pos[i].offset,out);
    write_long(value->pos[i].size,out);
  }
}

static void index_write__values(const Node *node, FILE *out) {
  unsigned char vN = VECTOR_SIZE(node->values);
  for (uint i = 0; i < vN; i++) {
    const Value *value = &node->values[i];
    write_string(out,value->word);
    index_write__dic_values(value, out);
  }
}


uint32_t index_write__node(const Node *node, FILE *out, FILE *value_out) {
  uint32_t offset;
  unsigned char cN = VECTOR_SIZE(node->childrens);
  unsigned char vN = VECTOR_SIZE(node->values);
  uint32_t child_offs[cN];

  if (!node)
    return 0;

  /* Write children and save their offsets */

  if (node->childrens) {
    qsort(node->childrens, cN, sizeof *node->childrens, comp_ch);

    for (uint i = 0; i < cN; i++) {
      const Node *child = node->childrens[i].node;
      child_offs[i] = index_write__node(child, out, value_out);
    }
  }

  /* Now write this node */
  offset = ftell(out);

  if (node->prefix[0]) {
    index_write__prefix(node, out);
    offset |= INDEX_NODE_PREFIX;
  }

  if (node->childrens) {
    index_write__childrens(node, out, child_offs);
    offset |= INDEX_NODE_CHILDS;
  }

  if (node->values) {
    uint32_t start_v;
    uint32_t end_v;
    // fputc(vN, out);
    for (uint i = 0; i < vN; i++)
      if (!strcmp(node->values[i].word, EMPTY_STR)) {
        Value temp = node->values[i];
        node->values[i] = node->values[0];
        node->values[0] = temp;
        break;
      }

    start_v = ftell(value_out);
    index_write__values(node, value_out);
    end_v = ftell(value_out);

    write_long(start_v, out);
    write_long(end_v, out);

    offset |= INDEX_NODE_VALUES;
  }

  return offset;
}



extern void index_print_head(){
  const char head[]="Saving Index";
  size_t len = strlen(head);
  size_t skip = (80-len)/2;
  size_t i = (80-len)/2;
  printf("\n");
  for (i = 0; i < skip; i ++) putchar('=');
  printf("%s", head);
  for (i = 0; i < skip; i ++) putchar('=');
  printf("\n\n");
}

extern void index_write(Node *root, DictionaryArray *D, const char *index_path){
  char keyDB[PATH_MAX], valueDB[PATH_MAX];
  combine_full_path(keyDB,index_path, "index_key.db");
  combine_full_path(valueDB,index_path, "index_value.db");

  FILE *out=fopen(keyDB, "w");
  FILE *value_out=tmpfile();
  if(!value_out){
    printf("The temporary file can not be created");
    exit(EXIT_FAILURE);
  }
  
  uint32_t initial_offset;//, final_offset;
  uint32_t u;

  write_long(INDEX_MAGIC, out);
  write_long(INDEX_VERSION, out);

  dictionary_array_write(D, value_out);
  write_long(ftell(value_out), out);
  
  /* Next byte is reserved for the offset of the root node */
  initial_offset = ftell(out);
  write_long(0, out);

  /* Dump trie */
  u = index_write__node(root, out,value_out);
	
  /* Update first byte */
  //final_offset = ftell(out);
  fseek(out, initial_offset, SEEK_SET);
  write_long(u, out);
  //fseek(out, final_offset, SEEK_SET);

  rewind(value_out);
  dict_data_zip(value_out, valueDB);

  fclose(value_out);
  fclose(out);
}



#include <fcntl.h>    //open
#include <sys/mman.h> //mmap
#include <sys/stat.h> //stat

struct DB {
  void *mm;
  uint32_t root_offset;
  size_t size;
  dictData *value;
  DictionaryArray array;
};


extern DB *DB_open(const char *path) {
  int fd;
  struct stat st;
  DB *idx;
  struct {
    uint32_t magic;
    uint32_t version;
    uint32_t root_offset;
    uint32_t dictionary_offset;
  } hdr;
  void *p;

  idx = xmalloc(sizeof *idx);

  char keyDB[PATH_MAX], valueDB[PATH_MAX];
  char *index_path = realpath(path, NULL);
  combine_full_path(keyDB,index_path, "index_key.db");
  combine_full_path(valueDB,index_path, "index_value.db");
  free(index_path);

  idx->value = dict_data_open(valueDB, 0 );
  if(!idx->value)
    goto fail_open;

  if ((fd = open(keyDB, O_RDONLY | O_CLOEXEC)) < 0) {
    goto fail_open;
  }

  if (fstat(fd, &st) < 0)
    goto fail_nommap;
  if ((size_t)st.st_size < sizeof(hdr))
    goto fail_nommap;

  if ((idx->mm = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) ==
      MAP_FAILED) {
    printf("mmap failed");
    goto fail_nommap;
  }

  p = idx->mm;
  hdr.magic = read_long_mm(&p);
  hdr.version = read_long_mm(&p);


  if (hdr.magic != INDEX_MAGIC) {
    printf("magick check failed");
    goto fail;
  }

  if (hdr.version >> 16 != INDEX_VERSION_MAJOR) {
    printf("version check failed");
    goto fail;
  }

  hdr.dictionary_offset = read_long_mm(&p);
  hdr.root_offset = read_long_mm(&p);

  char *dictionary = dict_data_read_(idx->value, 0, hdr.dictionary_offset, NULL, NULL);
  idx->array = dictionary_array_load((void *)dictionary);
  free(dictionary);
  
  idx->root_offset = hdr.root_offset;
  idx->size = st.st_size;

  close(fd);

  return idx;

fail:
  munmap(idx->mm, st.st_size);
fail_nommap:
  close(fd);
fail_open:
  free(idx);

  return NULL;
}

extern void DB_close(DB *idx)
{
  dict_data_close(idx->value);
  munmap(idx->mm, idx->size);
  dictionary_array_free(idx->array);
  free(idx);
}

typedef struct M_Node {
  const char *prefix;
  Childrens *children;
  uint32_t start_v;
  uint32_t end_v;
} M_Node;

static M_Node *index_mm_read_node(const DB *idx, uint32_t offset) {
  void *p = idx->mm;
  M_Node *node = xmalloc(sizeof *node);
  Childrens *children = NULL;
  const char *prefix;

  unsigned char cN = 0;
  uint32_t start_v = 0;
  uint32_t end_v = 0;
  int i;

  if ((offset & INDEX_NODE_MASK) == 0)
    return NULL;

  p = (char *)p + (offset & INDEX_NODE_MASK);

  if (offset & INDEX_NODE_PREFIX) {
    prefix = read_string_mm(&p);
  } else
    prefix = EMPTY_STR;

  if (offset & INDEX_NODE_CHILDS) {
    cN = read_char_mm(&p);
    children = VECTOR_RESIZE(children, cN);
    for (i = 0; i < cN; i++) {
      children[i] =
          (Childrens){.ch = read_char_mm(&p), .offset = read_long_mm(&p)};
    }
  }

  if (offset & INDEX_NODE_VALUES) {
    start_v = read_long_mm(&p);
    end_v = read_long_mm(&p);
  }

  node->prefix = prefix;
  node->start_v = start_v;
  node->end_v = end_v;
  node->children = children;

  return node;
}

static M_Node * index_mm_readchild(const DB *idx, const M_Node *parent, int ch) {
  unsigned char cN = VECTOR_SIZE(parent->children);
  Childrens *current = bsearch(&ch, parent->children, cN, sizeof(Childrens), comp_ch);
  if (current)
    return index_mm_read_node(idx, current->offset);

  return NULL;
}

static struct M_Node *index_mm_readroot(const DB *idx) {
  return index_mm_read_node(idx, idx->root_offset);
}

static void index_mm_free_node(M_Node *node)
{
  VECTOR_FREE(node->children);
  free(node);
}

inline static is_value(const M_Node *node){
  return (0!=node->start_v && 0!=node->end_v);
}

typedef struct M_Value{
  uint32_t start;
  uint32_t end;
} M_Value;

static M_Value index_mm_search_node(const DB *idx, M_Node *node, const char *key, int i)
{
	M_Node *child;
	M_Value values={0,0};
	
	char ch;
	int j;

	while(node) {
		for (j = 0; node->prefix[j]; j++) {
			ch = node->prefix[j];

			if (ch != key[i+j]) {
				index_mm_free_node(node);
				return values;
			}
		}
		i += j;
		if (key[i] == '\0') {
		  index_mm_free_node(node);
		  values.start=node->start_v;
		  values.end=node->end_v;
		  return values;
		}
		child = index_mm_readchild(idx, node, key[i]);
		index_mm_free_node(node);
		node = child;
		i++;
	}
	return values;
}


typedef struct ResultList{
  const char *key; //key which is searched //it is VECTOR
  size_t i; //current index of the key
  char *buf; //part of a key which is found //it is VECTOR
  M_Value *value; //VECTOR of values
} ResultList;


Value *value_get(const DB *idx, const char *key, M_Value m_value){
  Value *result =NULL;
  char *list=dict_data_read_(idx->value, m_value.start ,m_value.end-m_value.start , NULL,NULL);
  unsigned char vN =read_char_mm((void **)&list);
  for(uint i=0; i< vN; i++){
    Value cur={NULL,NULL};
    const char *word= read_string_mm((void **)&list);
    cur.word = xstrdup( !strcmp(word, EMPTY_STR) ? key : word);
    unsigned char dN =read_short_mm((void **)&list);
    cur.pos = xmalloc(dN*(sizeof *cur.pos));
    for(uint j=0; j< dN; j++){
      cur.pos[j].dictionary_number=read_short_mm((void **)&list); 
      cur.pos[j].offset=read_long_mm((void **)&list);
      cur.pos[j].size=read_long_mm((void **)&list);
    }
    VECTOR_PUSH(result, cur);
  }
  free(list);
  return result;
}

static void index_mm_search_all(const DB *idx,M_Node *node, ResultList *result)
{
  const int len=strlen(node->prefix);
  VECTOR_APPEND(result->buf, len+1, node->prefix);

  char ch;
  for(int j=0;result->i<VECTOR_SIZE(result->key)&&(ch=node->prefix[j]); j++)
    {
      if(result->key[result->i]==ch)
	result->i++;
      else
	result->i=0;
    }
  if(is_value(node)&& result->i==VECTOR_SIZE(result->key))
    {
      M_Value new ={.start = node->start_v, .end = node->end_v};
      VECTOR_PUSH(result->value, new);
    }
  for(uint k=0;   k<VECTOR_SIZE(node->children);k++)
    {
      ResultList c_result = {.key = result->key, .i=0, .buf=NULL, .value=NULL};
      VECTOR_APPEND(c_result.buf, VECTOR_SIZE(result->buf), result->buf);
      
      ch=node->children[k].ch;
      if(result->i<VECTOR_SIZE(result->key))
	c_result.i=(result->key[result->i]==ch) ? result->i+1 : 0;
      else
	c_result.i=result->i;
      
      M_Node *child = index_mm_read_node(idx, node->children[k].offset); // redefine node from parent to child
      VECTOR_PUSH(c_result.buf, ch);
      index_mm_search_all(idx, child, &c_result);
      VECTOR_APPEND(result->value, VECTOR_SIZE(c_result.value), c_result.value);
      VECTOR_FREE(c_result.buf);
      VECTOR_FREE(c_result.value);
    }
  index_mm_free_node(node);
}

static void index_mm_search_prefix(const DB *idx,M_Node *node, ResultList *result)
{
  M_Node *child;
  char ch;
  int i=0;
  while(node) {
    int j=0;
    for (j = 0; result->key[i+j]!='\0'&&node->prefix[j]; j++) {
      ch = node->prefix[j];

      if (ch != result->key[i+j]) {
	index_mm_free_node(node);
	return ;
      }
    }
    
    i += j;
    if (result->key[i] == '\0') {
      VECTOR_APPEND(result->buf, VECTOR_SIZE(result->key), result->key);////prefix will be included on the next step (see index_mm_search_all)
      result->i=i;
      index_mm_search_all(idx, node, result);
      return ;
    }
    child = index_mm_readchild(idx, node, result->key[i]);
    index_mm_free_node(node);
    node = child;
    i++;
  }
  return ;
}


ResultList word_search(const DB *idx, WordDefinition *word, int is_prefix)
{
  worddefinition_utf8_cleaning(word);
  M_Node *node =index_mm_readroot(idx);

  ResultList result={.key = word->word_utf_clean, .i=0, .buf=NULL, .value=NULL};
  if(is_prefix)
    index_mm_search_prefix(idx, node, &result);
  else
    index_mm_search_all(idx, node, &result);
  index_mm_free_node(node);
  return result;
}
