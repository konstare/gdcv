//https://github.com/BigfootACA/simple-init/blob/77e797560acf5dbb3e79c09a3dea90232bc81f00/libs/libkmod/libkmod-index.c#L891
#include "index.h"
#include "cvec.h"
#include "heap.h"
#include "dictionaries.h"
#include "dictzip.h"
#include "buffer.h"
#include "utils.h"
#include <arpa/inet.h> //htonl
#include <linux/limits.h>
#include <netinet/in.h>
#include <search.h> //lsearch
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

#define INDEX_MAGIC 0x57EE1
#define INDEX_VERSION_MAJOR 0x0004
#define INDEX_VERSION_MINOR 0x0001
#define INDEX_VERSION ((INDEX_VERSION_MAJOR << 16) | INDEX_VERSION_MINOR)

#define INDEX_NODE_PREFIX 0x80000000
#define INDEX_NODE_FORMS 0x40000000
#define INDEX_NODE_CHILDS 0x20000000
#define INDEX_NODE_MASK 0x1FFFFFFF


static int comp_ch(const void *a1, const void *a2) {
  const Childrens *k1 = a1;
  const Childrens *k2 = a2;
  return k1->ch - k2->ch;
  //https://gcc.gnu.org/bugzilla/show_bug.cgi?id=106247
}

/* #include <assert.h> */
/* struct foo4 { */
/*   char c;      /\* 1 byte *\/ */
/*   short s;     /\* 2 bytes *\/ */
/* }; */
/* static_assert(sizeof(struct foo4) == 3, "Check your assumptions"); */

static inline Node *index_null_node() {
  Node *node = xcalloc(1, sizeof *node);
  return node;
}

extern Node *index_create() {
  Node *node = index_null_node();
  node->prefix = xstrdup(EMPTY_STR);
  return node;
}

extern void index_free(Node *node) {
  free(node->prefix);

  form_free(node->forms);

  for (size_t i = 0; i < VECTOR_SIZE(node->childrens); i++)
    index_free(node->childrens[i].node);

  VECTOR_FREE(node->childrens);
  free(node);
}

extern int index_insert(Node *node, const WordDefinition *word) {
  size_t i = 0; /* index within str */
  const char *key = word->word_utf_clean;
  char ch;
  while (1) {
    size_t j; /* index within node->prefix */

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
	*node = (Node){0};
        prefix[j] = '\0';
        node->prefix = prefix;
        Childrens temp = {.ch = ch, .node = n};
        VECTOR_PUSH(node->childrens, temp);
        break;
      }
    }
    /* j is now length of node->prefix */
    i += j;

    ch = key[i];
    if (ch == '\0')
      return form_push(&node->forms, word);

    size_t cN = VECTOR_SIZE(node->childrens);
    Childrens *new =
        lfind(&ch, node->childrens, &cN, (sizeof *node->childrens), comp_ch);

    if (!new) {
      Childrens temp = {.ch = ch, .node = index_null_node()};
      VECTOR_PUSH(node->childrens, temp);
      Node *child = node->childrens[cN].node;
      child->prefix = xstrdup(&key[i + 1]);
      form_push(&child->forms, word);
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

/*
  Writes node to the *out and the different forms of a word in the *form_out.
  The result is the position of the node in the file *out. With modified bits if the node has childrens, word forms, or prefix.
 */
static uint32_t index_write__node(const Node *node, FILE *out, FILE *form_out) {
  uint32_t offset;
  unsigned char cN = (unsigned char)VECTOR_SIZE(node->childrens);
  unsigned char vN = (unsigned char)VECTOR_SIZE(node->forms);
  uint32_t child_offs[cN];
  long int pos;
  if (!node)
    return 0;

  /* Write children and save their offsets */

  
  if (node->childrens) {
    qsort(node->childrens, cN, sizeof *node->childrens, comp_ch);
    

    for (uint i = 0; i < cN; i++) {
      const Node *child = node->childrens[i].node;
      child_offs[i] = index_write__node(child, out, form_out);
    }
  }

  /* Now write this node */
  pos = ftell(out);
  if(pos<0)
    return 0;
  offset = (uint32_t)pos;

  if (node->prefix[0]) {
    index_write__prefix(node, out);
    offset |= INDEX_NODE_PREFIX;
  }

  if (node->childrens) {
    index_write__childrens(node, out, child_offs);
    offset |= INDEX_NODE_CHILDS;
  }

  if (node->forms) {
    for (uint i = 0; i < vN; i++)
      if (!strcmp(node->forms[i].word, EMPTY_STR)) {
        Form temp = node->forms[i];
        node->forms[i] = node->forms[0];
        node->forms[0] = temp;
        break;
      }

    M_Form form_m = forms_write(node->forms, form_out);

    write_long(form_m.start, out);
    write_long(form_m.end, out);

    offset |= INDEX_NODE_FORMS;
  }

  return offset;
}

extern void index_print_head() {
  const char head[] = "Saving Index";
  size_t len = strlen(head);
  size_t skip = (80 - len) / 2;
  size_t i = (80 - len) / 2;
  printf("\n");
  for (i = 0; i < skip; i++)
    putchar('=');
  printf("%s", head);
  for (i = 0; i < skip; i++)
    putchar('=');
  printf("\n\n");
}

extern void index_write(Node *root, DictionaryArray D, const char *index_path) {
  char keyDB[PATH_MAX], formDB[PATH_MAX];
  combine_full_path(keyDB, index_path, "index_key.db");
  combine_full_path(formDB, index_path, "index_forms.db");

  FILE *out = fopen(keyDB, "w");
  FILE *form_out = tmpfile();
  if (!form_out) {
    printf("The temporary file can not be created");
    exit(EXIT_FAILURE);
  }

  uint32_t initial_offset; //, final_offset;
  uint32_t u;

  write_long(INDEX_MAGIC, out);
  write_long(INDEX_VERSION, out);

  dictionary_array_write(D, form_out);
  write_long(ftell(form_out), out);

  /* Next byte is reserved for the offset of the root node */
  initial_offset = ftell(out);
  write_long(0, out);

  /* Dump trie */
  u = index_write__node(root, out, form_out);

  /* Update first byte */
  // final_offset = ftell(out);
  fseek(out, initial_offset, SEEK_SET);
  write_long(u, out);
  // fseek(out, final_offset, SEEK_SET);

  rewind(form_out);
  dict_data_zip(form_out, formDB);

  fclose(form_out);
  fclose(out);
}

#include <fcntl.h>    //open
#include <sys/mman.h> //mmap
#include <sys/stat.h> //stat

struct DB_ {
  void *mm;
  uint32_t root_offset;
  size_t size;
  dictData *value;
  DictionaryArray array;
};

extern DB database_open(const char *path) {
  int fd;
  struct stat st;
  DB idx = xmalloc(sizeof *idx);
  struct {
    uint32_t magic;
    uint32_t version;
    uint32_t root_offset;
    uint32_t dictionary_offset;
  } hdr;
  void *p;

  char keyDB[PATH_MAX], formDB[PATH_MAX];
  char *index_path = realpath(path, NULL);
  combine_full_path(keyDB, index_path, "index_key.db");
  combine_full_path(formDB, index_path, "index_forms.db");
  free(index_path);

  idx->value = dict_data_open(formDB, 0);
  if (!idx->value)
    goto fail_open;

  if ((fd = open(keyDB, O_RDONLY | O_CLOEXEC)) < 0) {
    goto fail_open;
  }

  if (fstat(fd, &st) < 0)
    goto fail_nommap;
  const size_t file_size = (size_t)st.st_size;
  if ( file_size < sizeof(hdr))
    goto fail_nommap;

  if ((idx->mm = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0)) ==
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

  char *dictionary =
      dict_data_read_(idx->value, 0, hdr.dictionary_offset, NULL, NULL);
  idx->array = dictionary_array_load((void *)dictionary);
  free(dictionary);

  idx->root_offset = hdr.root_offset;
  idx->size = file_size;

  close(fd);

  return idx;

fail:
  munmap(idx->mm, file_size);
fail_nommap:
  close(fd);
fail_open:
  free(idx);

  return NULL;
}

extern void database_close(DB idx) {
  dict_data_close(idx->value);
  munmap(idx->mm, idx->size);
  dictionary_array_free(idx->array);
  free(idx);
}

typedef struct M_Node {
  const char *prefix;
  Childrens *children;
  M_Form form;
} M_Node;

static M_Node *index_mm_read_node(const DB idx, uint32_t offset) {
  void *p = idx->mm;
  M_Node *node = xmalloc(sizeof *node);
  Childrens *children = NULL;
  const char *prefix;

  unsigned char cN = 0;
  M_Form form={0};

  if ((offset & INDEX_NODE_MASK) == 0)
    {
      free(node);
      return NULL;
    }

  p = (char *)p + (offset & INDEX_NODE_MASK);

  if (offset & INDEX_NODE_PREFIX) {
    prefix = read_string_mm(&p);
  } else
    prefix = EMPTY_STR;

  if (offset & INDEX_NODE_CHILDS) {
    cN = (unsigned char)read_char_mm(&p);
    children = VECTOR_RESIZE(children, cN);
    for (unsigned char i = 0; i < cN; i++) {
      children[i] =
          (Childrens){.ch = read_char_mm(&p), .offset = read_long_mm(&p)};
    }
  }

  if (offset & INDEX_NODE_FORMS) {
    form.start = read_long_mm(&p);
    form.end = read_long_mm(&p);
  }

  node->prefix = prefix;
  node->form=form;
  node->children = children;

  return node;
}

static M_Node *index_mm_readchild(const DB idx, const M_Node *parent, const char ch) {
  unsigned char cN = (unsigned char)VECTOR_SIZE(parent->children);
  const Childrens find ={.ch = ch};
  Childrens *current =
    bsearch(&find, parent->children, cN, sizeof *parent->children, comp_ch);
  if (current)
    return index_mm_read_node(idx, current->offset);

  return NULL;
}

static struct M_Node *index_mm_readroot(const DB idx) {
  return index_mm_read_node(idx, idx->root_offset);
}

static void index_mm_free_node(M_Node *node) {
  VECTOR_FREE(node->children);
  free(node);
}

typedef struct{
  size_t len;
  char *word;
  M_Form m_form;
} Search_Result;


void search_result_insert(Search_Result **result, const Buffer *buf, const M_Form m_form)
{
  Search_Result sr ={.word=buffer_steal(buf), .len=buffer_getlen(buf), .m_form=m_form};
  VECTOR_PUSH(*result, sr);
}

char **search_result_convert(const DB idx, Search_Result *sr){
  char **result=NULL;
  for(size_t i=0; i<VECTOR_SIZE(sr);i++){
    char **forms = forms_get_words(idx->value, sr[i].word, sr[i].m_form);
    if(forms)
      VECTOR_APPEND(result, VECTOR_SIZE(forms), forms);
    free(sr[i].word);
    VECTOR_FREE(forms);
  }
  VECTOR_FREE(sr);
  return result;
}

inline static bool is_form(const M_Node *node) {
  return (0 != node->form.start && 0 != node->form.end);
}

extern Definitions *index_mm_get_definitions(const DB idx, const char *search_word){
  M_Node *node = index_mm_readroot(idx);
  char *key = word_utf8_clean(search_word);
  Definitions *result=NULL;
  size_t i=0;
   while (node) {
     for (const char *ch=node->prefix; *ch ; ch++) { 
       if (key[i++] !=*ch)
	 {
 	 index_mm_free_node(node);
	 goto exit;
	 } 
     }
     if (key[i]=='\0' )
       {
	 if(is_form(node))
	   result = forms_get_definitions(idx->value, key, search_word, node->form);
	 index_mm_free_node(node);
	 goto exit;
       }
     M_Node *child = index_mm_readchild(idx, node, key[i]);
     index_mm_free_node(node);
     node = child; 
     i++; 
   }

 exit:
   free(key);
   return result;
}

static void index_mm_search_all(const DB idx, const M_Node *node, Buffer *buf, Search_Result **result){
  buffer_pushchars(buf, node->prefix);
  if (is_form(node)) {
    search_result_insert(result, buf, node->form);
  }
  for (unsigned char k = 0; k < VECTOR_SIZE(node->children); k++){
    M_Node *child = index_mm_read_node(idx, node->children[k].offset);
    BufferState *state = bufferstate_save(buf);
    buffer_push(buf, node->children[k].ch);
    index_mm_search_all(idx, child, buf, result);
    index_mm_free_node(child);
    bufferstate_restore(buf, state);
  }
}

char **index_mm_search_all_forms(const DB idx){
  Buffer *buf = buffer_init();
  M_Node *node = index_mm_readroot(idx);
  Search_Result *sr =NULL;
  index_mm_search_all(idx,node,buf, &sr);
  char **result=search_result_convert(idx, sr);
  index_mm_free_node(node);
  buffer_free(buf);
  return result;
}

static Search_Result *index_mm_search_prefix(const DB idx, const char *key, M_Node *node, Buffer *buf){
  Search_Result *result=NULL;
  size_t i=0;
  while (node) {
    size_t key_len_include = i;
     for (const char *ch=node->prefix; key[i]!='\0' && *ch ; ch++) { 
       if (key[i++] !=*ch)
 	 {
	   index_mm_free_node(node);
	   goto exit;
       } 
     }
     if (key[i]=='\0' )
       {
	 buffer_pushchars(buf, key);
	 buffer_shrink(buf, strlen(key) - key_len_include); //prefix will be included on the next step (see index_mm_search_all)
	 index_mm_search_all(idx, node, buf, &result);
	 index_mm_free_node(node);
	 goto exit;
       }
     M_Node *child = index_mm_readchild(idx, node, key[i]);
     index_mm_free_node(node);
     node = child; 
     i++; 
   }

 exit:
   return result;
}

extern char ** index_mm_search_prefix_forms(const DB idx, const char *word){
  Buffer *buf = buffer_init();
  M_Node *node = index_mm_readroot(idx);
  char *key = word_utf8_clean(word);
  Search_Result *sr= index_mm_search_prefix(idx, key, node, buf);
  char **result=search_result_convert(idx, sr);
  buffer_free(buf);
  free(key);
  return  result;
}

static void index_mm_search_substring(const DB idx, const char *key, const M_Node *node, Buffer *buf, Search_Result **result){
  buffer_pushchars(buf, node->prefix);
  if(buffer_strstr(buf, key))
    {
      buffer_shrink(buf, strlen(node->prefix));  //prefix will be included on the next step (see index_mm_search_all)
      index_mm_search_all(idx, node, buf, result);
      return;
    }
  for (unsigned char k = 0; k < VECTOR_SIZE(node->children); k++){
    M_Node *child = index_mm_read_node(idx, node->children[k].offset);
    BufferState *state = bufferstate_save(buf);
    buffer_push(buf, node->children[k].ch);
    if(buffer_strstr(buf, key))
      index_mm_search_all(idx, child, buf, result);
    else{
      index_mm_search_substring(idx, key, child, buf,  result);
    }
    index_mm_free_node(child);
    bufferstate_restore(buf, state);
  }
  
  return;
}

extern char ** index_mm_search_substring_forms(const DB idx, const char *word){
  Buffer *buf = buffer_init();
  M_Node *node = index_mm_readroot(idx);
  char *key =  word_utf8_clean(word);
  Search_Result *sr = NULL;
  index_mm_search_substring(idx, key, node, buf , &sr);
  char **result=search_result_convert(idx, sr);
  buffer_free(buf);
  index_mm_free_node(node);
  free(key);
  return result;
}

char **key_parse(const char *str){
  char **result =NULL;
  char *string=NULL;
  for (const char *s = str; *s; s++) {
    if(*s=='*')
      {
	VECTOR_PUSH(string, '\0');
	VECTOR_PUSH(result, string);
	string = NULL;
      }
    else{
      if(*s=='\\') s++;
      VECTOR_PUSH(string, *s);
    }
  }
  VECTOR_PUSH(string, '\0');
  VECTOR_PUSH(result, string);
  return result;
}

typedef struct{
  const DB idx;   //where to search
  char **keys;  //what to search
  size_t key_idx; //current subkey which is searched
  M_Node *node;  //where we now
  Search_Result **result; //what is found
  Buffer *buf; //what is the compound string now
} Search_State;

static void index_mm_search__substring(Search_State args);

static inline bool is_last_subkey(Search_State args){
  return VECTOR_SIZE(args.keys) == args.key_idx + 1;
}

static inline void search_result_add_if(Search_State args, bool condition, size_t shrink) {
  if (is_last_subkey(args)) {
    if (is_form(args.node) && condition) // the node  has forms and the condition
      search_result_insert(args.result, args.buf, args.node->form);
  } else {
    buffer_shrink(args.buf, shrink);
    args.key_idx++;
    index_mm_search__substring(args);
  }
}

static void index_mm_search__substring(Search_State args) {
  const char *subkey = args.keys[args.key_idx];
  size_t subkey_len = VECTOR_SIZE(subkey)-1;
  if (subkey_len == 0) {
    index_mm_search_all(args.idx, args.node, args.buf, args.result);
    return;
  }
  buffer_pushchars(args.buf, args.node->prefix);
  char *s;
  if ((s = buffer_strstr(args.buf, subkey))) {
    search_result_add_if(args,
			 strlen(s) == subkey_len, // the key closes the node.
			 strlen(args.node->prefix)); // prefix will be included on the next step 
    return;
  }
  M_Node *parent= args.node;
  for (unsigned char k = 0; k < VECTOR_SIZE(parent->children); k++) {
    M_Node *child = index_mm_read_node(args.idx, parent->children[k].offset);
    args.node=child;
    BufferState *state = bufferstate_save(args.buf);
    buffer_push(args.buf, parent->children[k].ch);
    if ((s = buffer_strstr(args.buf, subkey))) {
      search_result_add_if(args,
			   strlen(child->prefix) == 0, // the prefix is empty.
                           0);
    } else {
      index_mm_search__substring(args);
    }
    bufferstate_restore(args.buf, state);
    index_mm_free_node(child);
  }

  return;
}

static void index_mm_search__prefix(Search_State args,  const char *prefix) {
  size_t i = 0;
  for (; *prefix != '\0' && args.node->prefix[i]; i++) {
    if (*prefix++ != args.node->prefix[i])
      return;
  }
  if (*prefix == '\0') {
    const char *prefix2 = args.keys[0];
    buffer_pushchars(args.buf, prefix2);
    buffer_search_change(args.buf, VECTOR_SIZE(prefix2)-1);
    search_result_add_if(args,
			 !args.node->prefix[i], // node->prefix ends with the key
                          i);
    return;
  }
  M_Node *child;
  if ((child = index_mm_readchild(args.idx, args.node, *prefix))) {
    args.node=child;
    index_mm_search__prefix(args, prefix + 1);
    index_mm_free_node(child);
  }
  return;
}

extern char **index_mm_search_forms(const DB idx, const char *word) {
  Buffer *buf = buffer_init();
  M_Node *node = index_mm_readroot(idx);
  char *utf_clean = word_utf8_clean(word);
  char **key = key_parse(utf_clean);
  free(utf_clean);
  Search_Result *search_result = NULL;
  Search_State args = {.key_idx = 0,
                      .keys = key,
                      .idx = idx,
                      .node = node,
                      .result = &search_result,
		      .buf = buf
  };
  index_mm_search__prefix(args, key[0]);
  
  char **result = search_result_convert(idx, search_result);

  index_mm_free_node(node);
  buffer_free(buf);
  for (size_t i = 0; i < VECTOR_SIZE(key); i++)
    VECTOR_FREE(key[i]);
  VECTOR_FREE(key);

  return result;
}
