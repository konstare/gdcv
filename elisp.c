int plugin_is_GPL_compatible;

#include "emacs-module.h"
#include <string.h>
#include "format.h"
#include "index.h"
#include "utils.h"

#define FREE(p) if (p!=NULL) free(p);
#define SYM(sym) env->intern(env, (sym))

#define STRING(val) env->make_string(env, (val), strlen(val))
#define INTEGER(val) env->make_integer(env,(val))



#define NON_LOCAL_EXIT_CHECK \
  if (env->non_local_exit_check(env) != emacs_funcall_exit_return) {  \
    return SYM("nil");                                 \
  }


static emacs_value
make_list(emacs_env *env, int n, emacs_value elts[]) {
  emacs_value list=env->funcall(env, SYM("list"), n, elts);
  NON_LOCAL_EXIT_CHECK
  return list;
}


static int
extract_string_arg(emacs_env *env, emacs_value arg, char **str) {
  ptrdiff_t size = 0;
  if (!env->copy_string_contents(env, arg, NULL, &size))
    return 1;
  *str = xmalloc(size);
  if (!env->copy_string_contents(env, arg, *str, &size)) {
    if ((*str) != 0) free(*str);
    *str = 0;
    return 1;
  }
  return 0;
}



static void
close_db(void *ptr) {
  struct index_mm *index=(struct index_mm *)ptr;
  if (index) 
    index_mm_close(index);
}


static emacs_value
open_database(emacs_env *env, ptrdiff_t n, emacs_value *args, void *ptr)
{
    (void)ptr;
    (void)n;

    char *index_path;
    
    if (extract_string_arg(env, args[0], &index_path)) {
      return SYM("nil");
    }

    struct index_mm *index;
    index=index_mm_open(index_path);

    if(!index)
      {
	emacs_value signal = env->intern(env, "no index");
	emacs_value message = STRING(index_path);
	free(index_path);
	env->non_local_exit_signal(env, signal, message);
      }

    free(index_path);
    return env->make_user_ptr(env, close_db, (void *)index);
}


static emacs_value
render_emacs(emacs_env *env,struct format_ *format)
{
  char *src=format->string;

  struct 
  {
    TAG type;
    size_t open;   //open and close in utf8
    size_t close;   
    char *property;
    int id;
  } T[format->N];
  
  size_t tags_N=0;

  size_t utf_opened[N_TAG]={0};
  TAG type;

  int opened=0;
  size_t utf_pos=0;
  size_t pos;
  
   for(int i=0; i<format->N;i++)
     {
       pos=format->tag[i].pos;
       type=format->tag[i].type;
       for(size_t j=opened;j<pos;j++)
	 if ((src[j] & 0xC0) != 0x80) 
	   utf_pos++;
       opened=pos;
       
       if(format->tag[i].open==1)
	 {
	   T[tags_N].type=type;
	   T[tags_N].open=utf_pos;
	   T[tags_N].close=utf_pos; //in the case tag is not closed.
	   T[tags_N].property=format->tag[i].properties;
	   T[tags_N].id=i;
	   utf_opened[type]=tags_N;
	   tags_N++;
	 }
       else
	   T[utf_opened[type]].close=utf_pos;
     }

   emacs_value *elts =xmalloc(tags_N*sizeof(emacs_value));

   emacs_value tag_param[4];
   emacs_value string_param[2];
   
   
   for(size_t i=0;i<tags_N;i++)
     {
       tag_param[0]=INTEGER(T[i].type);
       tag_param[1]=INTEGER(T[i].open);
       tag_param[2]=INTEGER(T[i].close);
       tag_param[3]=T[i].property ? STRING(T[i].property) : SYM("nil");
       elts[i]=make_list(env, 4, tag_param);
     }

   string_param[0]=STRING(src);
   string_param[1]=make_list(env, tags_N, elts);
   FREE(elts);
   
   emacs_value result=make_list(env, 2, string_param);
      
   return result;
}


static emacs_value
word_defs(emacs_env *env, ptrdiff_t n, emacs_value *args, void *ptr)
{
    (void)ptr;
    (void)n;

    if (!env->is_not_nil(env, args[0])) {
      return SYM( "nil");
  }

    struct index_mm *index=(struct index_mm *)env->get_user_ptr(env, args[0]);
    NON_LOCAL_EXIT_CHECK;
      
    char *Word;
    if (extract_string_arg(env, args[1], &Word)) {
      return SYM( "nil");
    }


    
    int art=0;
    int rc=0;
    struct article *Art=NULL;
    emacs_value art_param[5];

    if((rc=decode_articles(Word,&Art,&art,index))==0)
      {
	emacs_value *elts =xmalloc(art*sizeof(emacs_value));
	for(int i=0;i<art;i++)
	  {
	    struct dictionary *Dic=get_dictionary(index->Dic  , index->N, Art[i].dic);

	    art_param[0]=STRING(Dic->data[NAME]);
	    art_param[1]=INTEGER(Art[i].dic);
	    art_param[2]=INTEGER(Art[i].start);
	    art_param[3]=INTEGER(Art[i].size);
	    art_param[4]=Dic->data[RESOURCES] ? STRING(Dic->data[RESOURCES]) : SYM( "nil");
	    
	    elts[i]=make_list(env, 5,art_param);
	    free(Art[i].word);
	}
	emacs_value result = make_list(env, art,elts);
	FREE(elts);
	free(Art);
	free(Word);
	return result;    
      }

    free(Word);
    return SYM( "nil");
}


static emacs_value
word_from_dictionary(emacs_env *env, ptrdiff_t n, emacs_value *args, void *ptr)
{
    (void)ptr;
    (void)n;
    if (!env->is_not_nil(env, args[0])) {
      return SYM( "nil");
  }

    struct index_mm *index=(struct index_mm *)env->get_user_ptr(env, args[0]);
    NON_LOCAL_EXIT_CHECK;
      
    int dic = env->extract_integer(env, args[1]);
    NON_LOCAL_EXIT_CHECK;
    int start = env->extract_integer(env, args[2]);
    NON_LOCAL_EXIT_CHECK;
    int end = env->extract_integer(env, args[3]);
    NON_LOCAL_EXIT_CHECK;

    char *path_unzip;
    if (extract_string_arg(env, args[4], &path_unzip)) {
      return SYM( "nil");
    }


    struct dictionary *Dic=get_dictionary(index->Dic  , index->N, dic);
    
    char *definition = word_fetch(Dic, start, end);

    struct format_ format;
    format.tag=NULL;

    strip_tags(definition,&format);
    emacs_value result=render_emacs(env,&format);

    if(strcmp(path_unzip,"")&&Dic->data[RESOURCES]!=NULL&&strcmp(Dic->data[RESOURCES],""))
      unzip_resources(&format, path_unzip, Dic->data[RESOURCES]);

    
    free_format(&format);
    free(path_unzip);
    free(definition);
    return result;
}


static emacs_value
word_lookup(emacs_env *env, ptrdiff_t n, emacs_value *args, void *ptr)
{
    (void)ptr;
    (void)n;

    if (!env->is_not_nil(env, args[0])) {
      return SYM( "nil");
  }

    struct index_mm *index=(struct index_mm *)env->get_user_ptr(env, args[0]);
    NON_LOCAL_EXIT_CHECK;
    char *Word;
    if (extract_string_arg(env, args[1], &Word)) {
      return SYM( "nil");
    }

    int is_prefix = env->extract_integer(env, args[2]);
    NON_LOCAL_EXIT_CHECK;


    struct article *Art=NULL;
    int art=0;

    look_for_a_word(index,Word,&art,&Art,is_prefix);
    free(Word);
    emacs_value res;
    if(art>0)
      {
	emacs_value *elts=xmalloc(art*sizeof(emacs_value));
	for(int j=0;j<art;j++)
	  {
	    elts[j] = STRING(Art[j].word);
	    free(Art[j].word);
	  }
	free(Art);
	res = make_list(env, art, elts);
	FREE(elts);
      }
    else
      res=SYM( "nil");
    
    return res;
}


static emacs_value
close_database(emacs_env *env, ptrdiff_t n, emacs_value *args, void *ptr)
{
    (void)ptr;
    (void)n;

    if (!env->is_not_nil(env, args[0]))
      return SYM( "nil");


    struct index_mm *index=(struct index_mm *)env->get_user_ptr(env, args[0]);

    NON_LOCAL_EXIT_CHECK;

    if (index ) {
      index_mm_close(index);
      env->set_user_ptr(env, args[0], 0);
    }
    return SYM( "nil");
}



void bind_func(emacs_env *env, const char *name, ptrdiff_t min, ptrdiff_t max, 
               emacs_value (*function) (emacs_env *env, ptrdiff_t nargs, emacs_value args[],void *) EMACS_NOEXCEPT, const char *doc)
{
  emacs_value fset = SYM( "fset");
  emacs_value args[2];

  args[0] = SYM( name);
  args[1] = env->make_function(env, min, max, function, doc, 0);
  env->funcall(env, fset, 2, args);
}


int
emacs_module_init(struct emacs_runtime *ert)
{
    emacs_env *env = ert->get_environment(ert);

    
    
    bind_func(env,"gdcv-database-open",1,1,open_database, "Open database with index and return pointer");
    bind_func(env,"gdcv-database-close",1,1,close_database, "Close database with given pointer");
    bind_func(env,"gdcv-look-word",3,3,word_lookup, "Look for words similar to given word");
    bind_func(env,"gdcv-word-defs",2,2,word_defs, "List of all dictionaries with current word");
    bind_func(env,"gdcv-word-fetch",5,5,word_from_dictionary, "Return article from specific dictionary ");

    emacs_value provide = SYM( "provide");    
    emacs_value gdcv[] = {SYM( "gdcv-elisp")};
    env->funcall(env, provide, 1, gdcv);
    return 0;
}
