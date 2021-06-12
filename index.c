#define _GNU_SOURCE /* for FNM_CASEFOLD, FNM_EXTMATCH, strcasestr in fnmatch.h, a GNU extension */
#include "utils.h"
#include <search.h>//bsearch
#include <zlib.h>  //gzFile gzclose gzgets
#include <fts.h> //fts_open fts_read fts_close
#include <fnmatch.h> //fnmatch
#include <netinet/in.h> /* htonl */
#include <fcntl.h> //open

#include <string.h> //strdup, memcpy, strcmp, strlen sscanf strtok
#include <stdio.h> //perror printf FILENAME_MAX

#include "utfproc/utf8proc.h" 

#include "index.h"


#define CHAR_BUFFER_SIZE 2097152

#define INDEX_MAGIC 0x57EE1
#define INDEX_VERSION_MAJOR 0x0003
#define INDEX_VERSION_MINOR 0x0001
#define INDEX_VERSION ((INDEX_VERSION_MAJOR<<16)|INDEX_VERSION_MINOR)

#define INDEX_NODE_PREFIX   0x80000000
#define INDEX_NODE_VALUES  0x40000000
#define INDEX_NODE_CHILDS   0x20000000
#define INDEX_NODE_MASK      0x1FFFFFFF 


#define COMPRESS_SHIFT 5




#define        MIN(a,b) (((a)<(b))?(a):(b))






void index_destroy(struct index_node *node)
{
  for(int i=0;i<node->c_N;i++)
    index_destroy(node->childrens[i].node);
    
  free(node->childrens);
  

  if(node->key)
    free(node->key);
  free(node->prefix);
  free(node);
}




static int comp_ch(const void *a1, const void *a2) {
  const struct childrens *k1 = a1;
  const  struct childrens *k2 = a2;
  return k1->ch-k2->ch;
}

static  int comp_id(const void *a1, const void *a2) {
  const struct dictionary *d1 = a1;
  const struct dictionary *d2 = a2;
  if (d1->id>d2->id)
    return 1;
  if (d1->id==d2->id)
    return 0;
  return -1;
}

static  int strcmp_word(const void *a1, const void *a2) {
  const struct index_value *v1 = a1;
  const struct index_value *v2 = a2;
  return strcmp(v1->word,v2->word);
}

static int index_add_value(struct index_node *node, const char *word, const char *value,const char *key)
{

  if(!node->values)
    node->v_N=0;

  size_t v_N=node->v_N;
  struct index_value *current=lfind(&word, node->values,&v_N, sizeof(struct index_value), strcmp_word);

  if(!node->key)
    node->key=xstrdup(key);
  
  if (!current)
    {
      node->values=xrealloc(node->values, (v_N+1)*sizeof(struct index_value));
      node->values[v_N].word = xstrdup(word);
      node->values[v_N].value = xstrdup(value);
      (node->v_N)++;
      return 0;
    }
  else
    {
      const size_t add = strlen(value);
      const size_t old=strlen(current->value);
      current->value = xrealloc(current->value, ( old+add + 1)*sizeof(char));
      memcpy(current->value+old, value, add + 1);
    }
      
  
  return 1;
}



int index_insert(struct index_node *node, const char *key, const char *value,const char *word)
{
  
	int i = 0; /* index within str */
	char ch;
	while(1) {
		int j; /* index within node->prefix */
	
		/* Ensure node->prefix is a prefix of &str[i].
		   If it is not already, then we must split node. */
		for (j = 0; node->prefix[j]; j++) {
			ch = node->prefix[j];
			
			if (ch != key[i+j]) {
				char *prefix = node->prefix;
				struct index_node *n;
				
				/* New child is copy of node with prefix[j+1..N] */
				n = xcalloc(1,sizeof(struct index_node));
				memcpy(n, node, sizeof(struct index_node));
				n->prefix = xstrdup(&prefix[j+1]);
				
				/* Parent has prefix[0..j], child at prefix[j] */
				memset(node, 0, sizeof(struct index_node));
				prefix[j] = '\0';
				node->prefix = prefix;

				node->childrens=xrealloc(node->childrens, (node->c_N+1)*sizeof(struct childrens));
				node->childrens[node->c_N].ch=ch;
				node->childrens[node->c_N].node=n;
				(node->c_N)++;
				node->key=NULL;
				node->values=NULL;
				break;
			}
		}
		/* j is now length of node->prefix */
		i += j;
	
		ch = key[i];
		if(ch == '\0')
		  return index_add_value(node, word, value,key);

		size_t c_N=node->c_N;
		struct childrens *new=lfind (&ch, node->childrens,&c_N, sizeof(struct childrens), comp_ch);

		
		if (!new)
		  {
		    struct index_node *child;
		    
		    node->childrens=xrealloc(node->childrens, (c_N+1)*sizeof(struct childrens));
		    node->childrens[c_N].ch=ch;
		    node->childrens[c_N].node=xmalloc(sizeof(struct index_node ));

		    child = node->childrens[c_N].node;
		    child->prefix = xstrdup(&key[i+1]);
		    child->childrens=NULL;
		    child->c_N=0;
		    child->key=NULL;

		    child->values=NULL;
		    index_add_value(child,word, value,key);

		    (node->c_N)++;
		    
		    return 0;
		  }


		
		/* Descend into child node and continue */
		node = new->node;
		i++;
	}
}




static int comp_base(const void *a1, const void *a2) {
  const  struct dictionary *d1 = a1;
 const  struct dictionary *d2 = a2;
  return strcasecmp(d1->base, d2->base);
}


extern void root_destroy(struct root_ D)
{
  for(int i=0;i<D.N;i++)
    {
      struct dictionary temp =D.Dic[i];
      free(temp.base);
      free(temp.encoding);
      for(int j=0;j<DATA;j++)
	  free(temp.data[j]);
    }
  free(D.Dic);
}

static char *clean(char *src)
{
  int idx = 0;
  int opened = 0; // false
  int size = strlen(src)+1;
  char *str = xmalloc(size);
  for(int i=0; i<size; i++)
    {
      if(src[i]=='{') opened = 1; 
      else if (src[i] == '}') opened = 0;
      else if (!opened&&src[i]!='\n'&&src[i]!='\r')
	{if((i>1&&src[i-1]=='\\'&&src[i-2]!='\\')||(i==1&&src[0]=='\\')) str[idx-1] =src[i];
	else  str[idx++] = src[i];
	} 
    }
  str[idx] = '\0';
  return str;
}


static void add_word_to_index(char * word, size_t D_start, size_t L_next, int dic_number, struct index_node *index)
{
  utf8proc_uint8_t *converted;
  int len=utf8proc_map((utf8proc_uint8_t *)word, -1, &converted, UTF8PROC_DECOMPOSE|UTF8PROC_NULLTERM|UTF8PROC_IGNORE |UTF8PROC_CASEFOLD |UTF8PROC_STRIPMARK);
  if(len<0)
    return;
  
  char *Def;
  int rc;
  rc=asprintf(&Def, "%d:%ld:%ld:", dic_number, D_start, L_next-1-D_start);
  if(rc==-1)
    {
      printf("asprintf failed");
      exit(EXIT_FAILURE);
    }
  if(len>0) //sometimes after cleaning nothing is left, for example the comment {{EXAMPLE}}\n
    {
      if(strcmp(word, (char *)converted)==0)
	index_insert(index, (char *)converted,Def,"");
      else
	index_insert(index, (char *)converted,Def,word);
    } 
  free(word);
  free(Def);
  free(converted);
}

void index_dictionaries(struct dictionary D, struct index_node *index) {

    printf("Name: %s File: %s  \n",D.data[NAME],D.data[DICTIONARY_FILE]);
    gzFile file = gzopen(D.data[DICTIONARY_FILE], "rb");
    char buf[CHAR_BUFFER_SIZE];
    char *word[100];
    int opened = 0;
    int def=0;
    int FB;
    size_t P = 3;
    size_t D_start = 0;
    gzseek(file,P , SEEK_SET);


    
    while (gzgets(file, buf, CHAR_BUFFER_SIZE)!=NULL){
      FB=buf[0];
      if (FB!='\t'&&FB!=' ')
	{
	  if (def&&opened) {
	    for(int i=0; i<opened;i++)
	      add_word_to_index(word[i], D_start,P ,D.id,index);

	    opened=0;
	    def=0;
	  }
	  
	  if (FB!= '\n' &&FB!= '\r'&&FB!= '#'){
	    if(opened==0)
	      D_start=P;
	    word[opened++] = clean(buf);
	  }
	}
      else def=1;
      P=(size_t )gztell(file);
    }
    
    if (opened)
      for(int i=0; i<opened;i++)
	add_word_to_index(word[i], D_start,P ,D.id,index);

    
    gzclose(file);
    
  
}



extern struct root_ index_directories (char **dir_list)
{
  struct root_ D;
  D.Dic=NULL;
  D.N=0;
  D.indexed=0;
  FTS *tree;
  FTSENT *f;

  const char *endings[]={".dsl.dz.files.zip",".dsl.files.zip",".files.zip","_abrv.dsl.dz", "_abrv.dsl",".dsl.dz",".bmp"};

  tree = fts_open(dir_list, FTS_LOGICAL | FTS_NOSTAT, NULL);
  
  CHECK_PTR(tree)
    while ((f = fts_read(tree)))
      {
	if(f->fts_info== FTS_F&&fnmatch("+(*.dz|*.files.zip|*_abrv.dsl|*.bmp)", f->fts_name, FNM_CASEFOLD|FNM_EXTMATCH) == 0)
	  {
	    int ending_N=0;
	    const char *name=f->fts_name;
	    const char *ptr;
	    char base[FILENAME_MAX];
	    enum DICTIONARY_STRUCTURE ext;
	    while((ptr=strcasestr(name, endings[ending_N])) == NULL && ending_N< 6) ending_N++;
	    
	    switch(ending_N)
	      {
	      case 0: case 1: case 2:
		ext=RESOURCES;
		break;
	      case 3: case 4:
		ext=FILE_ABRV;
		break;
	      case 5:
		ext=DICTIONARY_FILE;
		break;
	      case 6: default:
		ext=ICON;
	    }

	    strncpy(base, name, ptr-name);
	    base[ptr-name]='\0';


	    size_t S=D.N;
	    struct dictionary temp, *current;
	    temp.base=base;
	    current=lfind(&temp, D.Dic,&S, sizeof(struct dictionary), comp_base);
	    
	    if (!current)
	      {
		D.Dic=xrealloc(D.Dic, (D.N+1)* sizeof(struct dictionary));
		
		current=&D.Dic[D.N];
		
		xstrcpy(&current->base,base);

		current->id=D.N;
		current->to_index=1;
		current->encoding=NULL;

		for(int i=0;i<DATA;i++)
		  current->data[i]=NULL; 

		D.N++; 
	      }



	    if((current->data[ext])==NULL)
	      xstrcpy(&current->data[ext],f->fts_path);
	    else 
	      printf("These files are similar, and the first one will be indexed\n%s\n%s\n", current->data[ext], f->fts_path);


	    if(ext==DICTIONARY_FILE&&current->data[NAME]==NULL&&current->data[TO]==NULL&&current->data[FROM]==NULL&&current->encoding==NULL)
	      {
	    	gzFile file = gzopen(current->data[DICTIONARY_FILE], "rb");
		

	    	char encoding[30];
	    	unsigned char firstBytes[ 2 ];
	    	if ( gzread( file, firstBytes, sizeof( firstBytes ) ) != sizeof( firstBytes ) )
	    	  {
	    	    // Apparently the file's too short
	    	    gzclose( file );
	    	    printf("The DSL file is malformed");
	    	    continue;
	    	  }
	    	// If the file begins with the dedicated Unicode marker, we just consume
	    	// it. If, on the other hand, it's not, we return the bytes back
	    	if ( firstBytes[ 0 ] == 0xFF && firstBytes[ 1 ] == 0xFE )
	    	  strcpy( encoding, "UTF-16LE" );
	    	else
	    	  if ( firstBytes[ 0 ] == 0xFE && firstBytes[ 1 ] == 0xFF )
	    	    strcpy( encoding,  "UTF-16BE");
	    	  else
	    	    if ( firstBytes[ 0 ] == 0xEF && firstBytes[ 1 ] == 0xBB )
	    	      {
	    		// Looks like Utf8, read one more byte
	    		if ( gzread( file, firstBytes, 1 ) != 1 || firstBytes[ 0 ] != 0xBF )
	    		  {
	    		    // Either the file's too short, or the BOM is weird
	    		    gzclose( file );
	    		    printf("The DSL file is malformed");
	    		    continue;
	    		  }
	    		strcpy( encoding, "UTF-8");
	    	      }
	    	    else
	    	      {
	    		if ( firstBytes[ 0 ] && !firstBytes[ 1 ] )
	    		  strcpy( encoding, "UTF-16LE");
	    		else
	    		  if ( !firstBytes[ 0 ] && firstBytes[ 1 ] )
	    		    strcpy( encoding, "UTF-16BE");
	    		  else
	    		    strcpy( encoding,  "WINDOWS-1251");
	    	      }

		xstrcpy(&current->encoding,encoding);

		if(strcmp(encoding,"UTF-8")!=0)
		  {
		    gzclose( file );
		    continue;
		  }

	    	char buf[CHAR_BUFFER_SIZE];
	    	char *subString;

	    	while (gzgets(file, buf, CHAR_BUFFER_SIZE)!=NULL&&buf[0]== '#')
		  {
		    subString = strtok(buf, "\"");
		    subString=strtok(NULL, "\"");
		    if (strstr(buf, "NAME"))
		      xstrcpy(&current->data[NAME],subString);
		    if (strstr(buf, "INDEX_LANGUAGE"))
		      xstrcpy(&current->data[FROM],subString);
		    if (strstr(buf, "CONTENTS_LANGUAGE"))
		      xstrcpy(&current->data[TO],subString);
		  }
  
	    	gzclose(file);
		
		
	      }
	    
	  }
      }
 
  if (fts_close(tree) < 0)
    exit(EXIT_FAILURE);



  if(!D.N)
    {
      printf("No dictionaries are found");
      exit(EXIT_FAILURE);
    }


  for(int i=0;i<D.N;i++)
    {
      if(D.Dic[i].data[DICTIONARY_FILE]==NULL||(D.Dic[i].encoding&&strcmp(D.Dic[i].encoding,"UTF-8")))
	D.Dic[i].to_index=0;
      
      if(D.Dic[i].to_index)
	D.indexed++;
    }
    

  qsort(D.Dic, D.N, sizeof(struct dictionary), comp_id);
  
  return D;
}









  
/* Recursive post-order traversal

   Pre-order would make for better read-side buffering / readahead / caching.
   (post-order means you go backwards in the file as you descend the tree).
   However, index reading is already fast enough.
   Pre-order is simpler for writing, and depmod is already slow.
 */

#define SetBit(A,k)  ( A |= (1 << ((unsigned char)k/32)) )
#define TestBit(A,k)    ( A & (1 << ((unsigned char)k/32)) )


static unsigned char index_add_grand_childrens(struct index_node *node)
{
  unsigned char bits_n=0;
  unsigned char bits_t=0;
  
  struct index_node *child;

  if (!node)
      return bits_n;

  char ch;
  for(int j=0;(ch=node->prefix[j]);j++)
    SetBit(bits_n,ch);
  
  if (node->childrens)
    {
      for (int i = 0; i < node->c_N; i++)
	{
	  ch=node->childrens[i].ch;
	  SetBit(bits_n,ch);
	  child = node->childrens[i].node;
	  bits_t=index_add_grand_childrens(child);
	  node->childrens[i].gc=bits_t;
	  bits_n|=bits_t;
	}
    }

  return bits_n;
}

static uint32_t index_write__node(const struct index_node *node, FILE *out, FILE * value_out)
{
  uint32_t offset;
  uint32_t *child_offs = NULL;
  unsigned char c_N=node->c_N;
  unsigned char v_N=node->v_N;

  int i=0;

  if (!node)
    return 0;
  
  /* Write children and save their offsets */
		
  
  if (node->childrens)
    {
      qsort(node->childrens, c_N, sizeof(struct childrens), comp_ch);

      const struct index_node *child;
      child_offs = xmalloc(c_N * sizeof(uint32_t));
      
      for (i = 0; i < c_N; i++)
	{
	  child = node->childrens[i].node;
	  child_offs[i] = htonl(index_write__node(child, out,value_out));
	}
    }
      
		
	/* Now write this node */
	offset = ftell(out);

	if (node->prefix[0]) {
	  fputs(node->prefix, out);
	  fputc('\0', out);
	  offset |= INDEX_NODE_PREFIX;
	}
		
	if (node->childrens)
	  {
	    fputc(c_N, out);

	    for(i=0; i<c_N;i++)
	      {
		fputc(node->childrens[i].ch, out);
		fputc(node->childrens[i].gc, out);
		fwrite(&child_offs[i], sizeof(uint32_t), 1, out);
	      }

	    free(child_offs);

	    

	  offset |= INDEX_NODE_CHILDS;
	}
	
	if (node->values) {

	  fputc(v_N, out);
	  
	  for( i=0; i <v_N;i++)
	    if(!strcmp(node->values[i].word,""))
	      {
		struct index_value temp=node->values[i];
		node->values[i]=node->values[0];
		node->values[0]=temp;
		break;
	      }
	  
	  uint32_t start_v;
	  uint32_t end_v;

	  start_v=htonl(ftell(value_out));
	  
	  for(i=0;i<v_N;i++){
	    fputs(node->values[i].word, value_out);
	    fputc('\0', value_out);
	  }

	  for(i=0;i<v_N;i++){
	    fputs(node->values[i].value, value_out);
	    fputc('\0', value_out);
	  }

	  end_v=htonl(ftell(value_out));

	  fwrite((const void*)&start_v, sizeof(uint32_t), 1, out);
	  fwrite((const void*)&end_v, sizeof(uint32_t), 1, out);


	  
	  for(int i=0;i<node->v_N;i++)
	    {
	      free(node->values[i].word);
	      free(node->values[i].value);
	    }
	  free(node->values);


	  offset |= INDEX_NODE_VALUES;
	}
	
	return offset;
}


void dictionaries_write(struct dictionary D,FILE *out) {
  if (D.to_index) {
    uint32_t u;
    u = htonl(D.id);
    fwrite(&u, sizeof(u), 1, out);

    fputs(D.base, out);
    fputc('\0', out);

    
    for(int j=0;j<DATA;j++)
      {
	if(D.data[j])
	  fputs(D.data[j], out);

	fputc('\0', out);
	
      }
  }
}

extern void index_write(struct index_node *node, struct root_ D, char *index_path)
{

  printf("\n\t");
  for (int i = 0; i < 50; i ++) putchar('=');
  printf("Saving Index");
  for (int i = 0; i < 58; i ++) putchar('=');
  printf("\n\n");

  char *keyDB, *valueDB;
  xasprintf(&keyDB, "%s/index_key.db", index_path);
  xasprintf(&valueDB, "%s/index_value.db", index_path);

  FILE *out=fopen(keyDB, "w");
  FILE *value_out=tmpfile();
  
	uint32_t initial_offset, final_offset;
	uint32_t u;
	
	u = htonl(INDEX_MAGIC);
	fwrite(&u, sizeof(u), 1, out);
	u = htonl(INDEX_VERSION);
	fwrite(&u, sizeof(u), 1, out);

	u = htonl(D.indexed);
	fwrite(&u, sizeof(u), 1, out);


	
	for(int i=0;i<D.N;i++)
	  dictionaries_write(D.Dic[i],out);
	
	/* Next word is reserved for the offset of the root node */
	initial_offset = ftell(out);
	u = 0;
	fwrite(&u, sizeof(uint32_t), 1, out);


	//reformat nodes;
	index_add_grand_childrens(node);

	
	/* Dump trie */
	u = htonl(index_write__node(node, out,value_out));
	
	/* Update first word */
	final_offset = ftell(out);
	fseek(out, initial_offset, SEEK_SET);
	fwrite(&u, sizeof(uint32_t), 1, out);
	fseek(out, final_offset, SEEK_SET);

	rewind(value_out);
	dict_data_zip( value_out, valueDB);

	fclose(value_out);
	fclose(out);
	free(keyDB);
	free(valueDB);
	
}


extern struct index_node *index_create(struct root_ D)
{
	struct index_node *node;
	node = xcalloc(1,sizeof(struct index_node));
	node->prefix = xstrdup("");
	node->childrens=NULL;
	node->values=NULL;
	node->c_N=0;
	node->v_N=0;
	  
	printf("\n\t");
	for (int i = 0; i < 50; i ++) putchar('=');
	printf("Indexation");
	for (int i = 0; i < 58; i ++) putchar('=');
	printf("\n\n");

	for(int i=0;i<D.N;i++)
	  index_dictionaries(D.Dic[i],node);
	
	return node;
}







#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static const char _idx_empty_str[] = "";


struct children_mm {
  char ch;		/* path compression */
  unsigned char gc;
  uint32_t offset; /* indexed by character */
};

struct index_mm_node {
  struct index_mm *idx;
  const char *prefix;
  const int prefix_len;
  unsigned char v_N;
  unsigned char c_N;
  struct children_mm  *children;
  uint32_t start_v;
  uint32_t end_v;
};


#define get_unaligned(ptr)					\
  __extension__ ({						\
      struct __attribute__((packed)) {				\
	typeof(*(ptr)) __v;					\
      } *__p = (typeof(__p)) (ptr);				\
      __p->__v;							\
})

static inline uint32_t read_long_mm(void **p)
{
	uint8_t *addr = *(uint8_t **)p;
	uint32_t v;

	/* addr may be unalined to uint32_t */
	v = get_unaligned((uint32_t *) addr);

	*p = addr + sizeof(uint32_t);
	return ntohl(v);
}

static inline uint8_t read_char_mm(void **p)
{
	uint8_t *addr = *(uint8_t **)p;
	uint8_t v = *addr;
	*p = addr + sizeof(uint8_t);
	return v;
}


static inline char *read_chars_mm(void **p )
{
	char *addr = *(char **)p;
	size_t len =  strlen(addr);
	*p = addr + len + 1;

	if(len==0)
	  return (char *)_idx_empty_str;
	
	return addr;
}



static void index_mm_free_node(struct index_mm_node *node)
{
  free(node);
}


static struct index_mm_node *index_mm_read_node(struct index_mm *idx, uint32_t offset) {
  void *p = idx->mm;
  struct index_mm_node *node;
  const  char *prefix;
  
  struct children_mm kids[UCHAR_MAX];
  unsigned char  c_N=0;
  unsigned char  v_N=0;
  uint32_t start_v=0;
  uint32_t end_v=0;
  int i;



  if ((offset & INDEX_NODE_MASK) == 0)
    return NULL;


  p = (char *)p + (offset & INDEX_NODE_MASK);

  
  if (offset & INDEX_NODE_PREFIX) {
    prefix = read_chars_mm(&p);
  } else
    prefix =  _idx_empty_str;

  if (offset & INDEX_NODE_CHILDS) {
    c_N = read_char_mm(&p);
    for(i=0;i<c_N;i++)
      {
	kids[i].ch = read_char_mm(&p);
	kids[i].gc = read_char_mm(&p);
	kids[i].offset = read_long_mm(&p);
      }
  
  }
  
  if (offset & INDEX_NODE_VALUES) {
    v_N=read_char_mm(&p);
    start_v = read_long_mm(&p);
    end_v = read_long_mm(&p);
  }


	    
  node = xmalloc(sizeof(struct index_mm_node)+c_N*sizeof(struct children_mm));

  node->idx = idx;
  node->prefix = prefix;
  
  node->c_N=c_N;
  node->v_N=v_N;

  node->start_v=start_v;
  node->end_v=end_v;
  
  node->children=(struct children_mm *)((char *)node +  sizeof(struct index_mm_node));

  if(c_N)
      memcpy(node->children, kids, c_N*sizeof(struct children_mm));
  else
    node->children=NULL;
    
  return node;
}



extern struct index_mm *index_mm_open(char *index_path)
{
	int fd;
	struct stat st;
	struct index_mm *idx;
	struct {
	  uint32_t magic;
	  uint32_t version;
	  uint32_t root_offset;
	  uint32_t dictionary_number;
	  struct dictionary * Dic;
	} hdr;
	void *p;


	char *filename,*value_file;
	char *indx=realpath(index_path, NULL);
	xasprintf(&filename, "%s/index_key.db",indx );
	xasprintf(&value_file, "%s/index_value.db", indx);
	free(indx);
	
	idx = xmalloc(sizeof(struct index_mm));

	
	idx->dz = dict_data_open(value_file, 0 );
	if(!idx->dz)
	  goto fail_open;
	
	if ((fd = open(filename, O_RDONLY|O_CLOEXEC)) < 0) {
		goto fail_open;
	}

	if (fstat(fd, &st) < 0)
		goto fail_nommap;
	if ((size_t) st.st_size < sizeof(hdr))
		goto fail_nommap;

	if ((idx->mm = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0))
							== MAP_FAILED) {
	  printf("mmap failed");
	  goto fail_nommap;
	}

	p = idx->mm;
	hdr.magic = read_long_mm(&p);
	hdr.version = read_long_mm(&p);

	hdr.dictionary_number=read_long_mm(&p);

	hdr.Dic=NULL;
	hdr.Dic=xmalloc(hdr.dictionary_number*sizeof(struct dictionary));
	for(int j=0;j<(int)hdr.dictionary_number;j++)
	  {
	    struct dictionary * current = &hdr.Dic[j];
	    current->id=read_long_mm(&p);
	    current->base= read_chars_mm(&p);
	    current->to_index=1;
	    current->encoding=NULL;
	    
	     for(enum DICTIONARY_STRUCTURE ext=0; ext<DATA;ext++) 
	     	current->data[ext]= read_chars_mm(&p);
	  }


	hdr.root_offset = read_long_mm(&p);

	if (hdr.magic != INDEX_MAGIC) {
	  printf("magick check failed");
	
		goto fail;
	}

	if (hdr.version >> 16 != INDEX_VERSION_MAJOR) {
	  printf("version check failed");

		goto fail;
	}

	idx->root_offset = hdr.root_offset;
	idx->size = st.st_size;
	idx->Dic = hdr.Dic;
	idx->N=hdr.dictionary_number;
	
	close(fd);

	free(filename);
	free(value_file);

	return idx;

fail:
	munmap(idx->mm, st.st_size);
fail_nommap:
	close(fd);
fail_open:
	free(idx);

	free(filename);
	free(value_file);
	return NULL;
}


extern void index_mm_close(struct index_mm *idx)
{
  dict_data_close( idx->dz );
  free(idx->Dic);
  munmap(idx->mm, idx->size);
  free(idx);
}
static struct index_mm_node *index_mm_readroot(struct index_mm *idx)
{
	return index_mm_read_node(idx, idx->root_offset);
}
static struct index_mm_node *index_mm_readchild(const struct index_mm_node *parent,
									int ch)
{
  struct children_mm *current =bsearch(&ch, parent->children,parent->c_N, sizeof(struct children_mm), comp_ch);
  if(current)
    return index_mm_read_node(parent->idx, current->offset);
  
  return NULL;
}



struct string {
  size_t len;
  char *str;
};

struct result_list__node
{
  struct string word;
  struct result_list__node * next;
};

struct result_list
{
  struct result_list__node * head;
  const int max;  //how much results are important to consider
  int cnt; //how much results are caught
  size_t max_len; //the maximum length of words in min(max,cnt)
};

struct result_list__node * combine_lists(struct result_list *dest_, struct result_list *src_)
{
  struct result_list__node **dest=&dest_->head;
  struct result_list__node **src=&src_->head;

  dest_->cnt+=src_->cnt;
  while(*dest!=NULL&&(*dest)->word.len<(*src)->word.len)
    dest=&(*dest)->next;

  struct result_list__node *temp=*dest;
  *dest=*src;

  while(*dest!=NULL)
    dest=&(*dest)->next;

  *dest=temp;

  dest=&dest_->head;
  int i=0;
  while(*dest!=NULL&&i<dest_->max)
    {
      dest_->max_len=(*dest)->word.len;
      dest=&(*dest)->next;
      i++;
    }

  
  return temp;
}


struct result_list__node * insert_sorted(struct result_list *list, struct string str)
{
  struct result_list__node **link=&list->head;
  while(*link!=NULL&&(*link)->word.len<str.len)
    link=&(*link)->next;

  struct result_list__node *new=xmalloc(sizeof(struct result_list__node));
  new->word.str=xstrdup(str.str);
  new->word.len=str.len;
  new->next=*link;
  *link=new;
  return new;
}



static struct result_list fetch_value_list(struct index_mm_node *node, const char *key)
{
  struct result_list values={NULL,0,0,0};
  size_t key_len=strlen(key); //all forms of the word can have different length, bu assign the length  of th general form
  if(!node->v_N)
    return values;

  char *list=dict_data_read_(node->idx->dz,node->start_v ,node->end_v-node->start_v-1 , NULL,NULL );
  values.cnt=node->v_N;
  values.max_len=key_len;

  char *ptr=list;
  for(int j=0;j<values.cnt;j++,ptr++)
    {
      insert_sorted(&values,(struct string){key_len, *ptr ? ptr: (char *)key });
      ptr+=strlen(ptr);
    } 
  
  free(list);
  return values;
}



struct unziped {
  char N;
  char *value;
};


static struct unziped index_mm_search_node(struct index_mm_node *node, const char *key, int i)
{
	struct index_mm_node *child;
	struct unziped values={0,NULL};
	
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
		  values.N=node->v_N;
		  if(node->v_N)
		    values.value=dict_data_read_(node->idx->dz,node->start_v ,node->end_v-node->start_v-1 , NULL,NULL );
		  index_mm_free_node(node);
		  return values;
		}
		child = index_mm_readchild(node, key[i]);
		index_mm_free_node(node);
		node = child;
		i++;
	}

	return values;
}








extern int decode_articles(char *key,struct article **cards,int *cards_number, struct index_mm *index)
{

  utf8proc_uint8_t *key_decompose;
  int len=utf8proc_map((utf8proc_uint8_t *)key, -1, &key_decompose,
		       UTF8PROC_DECOMPOSE|UTF8PROC_NULLTERM|UTF8PROC_IGNORE |UTF8PROC_CASEFOLD |UTF8PROC_STRIPMARK);
  if(len<0)
      return 1;



  struct index_mm_node *root;
  root = index_mm_readroot(index);
  
  struct unziped values=index_mm_search_node(root, (const char *)key_decompose, 0);

  if (!values.N)
    {
      free(key_decompose);
      return 1;
    }

  int j=0;
  char *ptr=values.value;
  char *word=ptr;
  char *word0=ptr;
  
  for(j=0;j<values.N;j++,ptr++)
  	ptr+=strlen(ptr);
  char *def=ptr;
  char *def0=ptr;
  for(j=0;j<values.N&&strcmp(word,key)&&strcmp((char*)key_decompose,key);j++,word++,def++)
    {
      word+=strlen(word);
      def+=strlen(def);
    }
  
    
  if(j==values.N)
  {
    def=def0;
    word=word0;
  };


  int C=0;
  size_t par[3]={0};
  struct article * current;
  char *S=def;
  char *E;
  while(S!=NULL&&*S!='\0') {
    for(int j=0;j<3;j++)
      {
	par[j]= strtol(S, &E, 10);
	S=E+1;
      }
    *cards=xrealloc(*cards, (C+1)*sizeof(struct article));
    current = (*cards+C);
    current->dic=par[0];
    current->full_definition = NULL;
    current->definition=NULL;
    current->word=xstrdup(word);
    current->start=par[1];
    current->size=par[2];
    C++;
  }
  *cards_number=C;

  free(values.value);
  free(key_decompose);

  return 0;
}

extern struct dictionary * get_dictionary(struct dictionary *Dic, int N, int id)
{
  struct dictionary current;
  current.id=id;
  return bsearch(&current, Dic, N, sizeof(struct dictionary),comp_id);

}

extern char * word_fetch(struct dictionary * Dic, int start, int size)
  {
  dictData * dz;
  dz = dict_data_open((const char *)Dic->data[DICTIONARY_FILE], 0 );
  char *definition=dict_data_read_(dz,start ,size , NULL,NULL );
  dict_data_close( dz );
  return definition;
  }



static void index_mm_search_all(struct string *buf,struct index_mm_node *node, const struct string key, size_t i,struct result_list *result) 
{
  
  const int len=strlen(node->prefix);
  strcpy(buf->str +buf->len,node->prefix);
  buf->len+=len;

  char ch;
  for(int j=0;i<key.len&&(ch=node->prefix[j]);j++)
    {
	  if(key.str[i]==ch)
	    i++;
	  else
	    i=0;
    }

  
  if(node->v_N&&(result->max_len>buf->len||result->cnt<result->max) &&   i==key.len)
    {
      struct result_list new=fetch_value_list(node, buf->str);
      combine_lists(result, &new);
    }

  if((result->cnt<result->max)|| ( result->max_len>buf->len))
    {
      struct index_mm_node *child;
      struct string child_buf;
      int tempi;
      size_t j;
      
      for(int k=0;   k<node->c_N;k++)
	{
	  ch=node->children[k].ch;
	  if(i<key.len)
	    tempi=(key.str[i]==ch) ? i+1 : 0;
	  else
	    tempi=i;

	  for(j=tempi;j<key.len&&TestBit(node->children[k].gc,key.str[j]);j++);
	  
	  if(j==key.len)	    {
	      child = index_mm_read_node(node->idx, node->children[k].offset); // redefine node from parent to child
	      child_buf=*buf;
	      child_buf.str[child_buf.len++]=ch;
	      index_mm_search_all(&child_buf,child,key,tempi,result);
	      }
	}
    }
  
  index_mm_free_node(node);
}



static void index_mm_search_prefix(struct index_mm_node *node, struct string *buf,const struct string key_f,struct result_list *result)
{
  
  struct index_mm_node *child;
  char *key=key_f.str;
  

  char ch;
  int i=0;
  while(node) {
    int j=0;
    for (j = 0; key[i+j]!='\0'&&node->prefix[j]; j++) {
      ch = node->prefix[j];

      if (ch != key[i+j]) {
	index_mm_free_node(node);
	return ;
      }
    }
    
    i += j;
    if (key[i] == '\0') {
      buf->len=i-j;   ////prefix will be included on the next step (see index_mm_search_all)
      strcpy(buf->str,key);
      index_mm_search_all(buf,node, key_f,i,result);

      return ;
    }
    child = index_mm_readchild(node, key[i]);
    index_mm_free_node(node);
    node = child;
    i++;
  }

  return ;
}






extern void look_for_a_word(struct index_mm *idx,char *word,int *art, struct article **Art, int is_prefix)
{
  int i=0;
  utf8proc_uint8_t *converted;
  int len=utf8proc_map((utf8proc_uint8_t *)word, -1, &converted,
		       UTF8PROC_DECOMPOSE|UTF8PROC_NULLTERM|UTF8PROC_IGNORE |UTF8PROC_CASEFOLD |UTF8PROC_STRIPMARK);
  if(len<0)
    return ;

  char *key=(char *)converted;
  
  struct string buf;
  
  struct index_mm_node *node=index_mm_readroot(idx);

  buf.str=xmalloc(CHAR_BUFFER_SIZE*sizeof(char));
  buf.len=0;


  struct result_list result={NULL,20,0,SIZE_MAX};
  if(is_prefix)
    index_mm_search_prefix(node, &buf, (struct string){strlen(key),key},&result);
  else
    index_mm_search_all(&buf,node,(struct string){strlen(key),key},0,&result); 


  struct result_list__node *current=result.head;
  struct result_list__node *temp;

  *art=MIN(result.cnt,result.max);
  *Art=xmalloc(*art*sizeof(struct article));

  for(i=0;i<*art  ;i++)
    {
      (*Art+i)->definition=NULL;
      (*Art+i)->full_definition=NULL;
      (*Art+i)->word=current->word.str;
      temp=current->next;
      free(current);
      current=temp;
    }

    while(current!=NULL)
    {
      temp=current->next;
      free(current->word.str);
      free(current);
      current=temp;
    }

  free(buf.str);
  free(converted);
   
}
