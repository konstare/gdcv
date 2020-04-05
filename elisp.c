#define _GNU_SOURCE
int plugin_is_GPL_compatible;

#include "emacs-module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h> //mkdir, stat
#include <argp.h>
#include <error.h>
#include <search.h>



#include "format.h"
#include "index.h"
#include "utils.h"



#define FREE(p) if (p!=NULL) free(p);
#define SYM(env, sym) (env)->intern((env), sym)


#define NON_LOCAL_EXIT_CHECK(env) \
  if ((env)->non_local_exit_check(env) != emacs_funcall_exit_return) {  \
    return (env)->intern((env), "nil");                                 \
  }


static emacs_value make_list(emacs_env *env, int n, emacs_value elts[]) {
  return env->funcall(env, SYM(env,"list"), n, elts);
}


int extract_string_arg(emacs_env *env, emacs_value arg, char **str) {
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

 


static void close_db(void *ptr) {
  struct index_mm *index=(struct index_mm *)ptr;
  if (index) 
    index_mm_close(index);
}


static emacs_value
open_database(emacs_env *env, ptrdiff_t n, emacs_value *args, void *ptr)
{
    (void)ptr;
    (void)n;

    char *FileDB = 0;
    if (extract_string_arg(env, args[0], &FileDB)) {
      return SYM(env, "nil");
    }
    
    struct stat stat_buffer;
    struct index_mm *index;
    index=index_mm_open(FileDB,&stat_buffer);

    if(!index)
      {
	emacs_value signal = env->intern(env, "no index");
	emacs_value message = env->make_string(env, FileDB, strlen(FileDB));
	env->non_local_exit_signal(env, signal, message);
	return SYM(env, "nil");
      }

    return env->make_user_ptr(env, close_db, (void *)index);
}


emacs_value
render_emacs(emacs_env *env,struct format_ *format)
{
  char *src=format->string;
  size_t len=strlen(src);


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
   for(size_t i=0;i<tags_N;i++)
	 elts[i]=make_list(env, 4, (emacs_value[])
			 {env->make_integer(env,T[i].type),
			    env->make_integer(env,T[i].open),
			    env->make_integer(env,T[i].close),
			    T[i].property ? env->make_string(env, T[i].property, strlen(T[i].property)) : SYM(env, "nil")});
   
   emacs_value result=make_list(env, 2, (emacs_value[])
				{env->make_string(env,src,len),
				 make_list(env, tags_N, elts)
				  });
   
   FREE(elts);
   
   return result;
}


static emacs_value
word_defs(emacs_env *env, ptrdiff_t n, emacs_value *args, void *ptr)
{
    (void)ptr;
    (void)n;

    if (!env->is_not_nil(env, args[0])) {
      return SYM(env, "nil");
  }

    struct index_mm *index=(struct index_mm *)env->get_user_ptr(env, args[0]);

    char *Word;
    if (extract_string_arg(env, args[1], &Word)) {
      return SYM(env, "nil");
    }


    
    int art=0;
    int rc=0;
    struct article *Art=NULL;
    char *name;
    char *resources;
    if((rc=decode_articles(Word,&Art,&art,index))==0)
      {
	emacs_value *elts =xmalloc(art*sizeof(emacs_value));
	for(int i=0;i<art;i++)
	  {
	    struct dictionary *Dic=get_dictionary(index->Dic  , index->N, Art[i].dic);

	    name=Dic->data[NAME];
	    resources=Dic->data[RESOURCES];
	    elts[i]=make_list(env, 5,
			      (emacs_value[]){env->make_string(env,name,strlen(name)),
						env->make_integer(env,Art[i].dic),
						env->make_integer(env,Art[i].start),
						env->make_integer(env,Art[i].size),
						resources ? env->make_string(env,resources,strlen(resources)) : SYM(env, "nil")	});
	}
	emacs_value result = make_list(env, art,elts);
	free(elts);
	free(Art); 
	return result;    
      }
    return SYM(env, "nil");
}


static emacs_value
word_from_dictionary(emacs_env *env, ptrdiff_t n, emacs_value *args, void *ptr)
{
    (void)ptr;
    (void)n;
    if (!env->is_not_nil(env, args[0])) {
      return SYM(env, "nil");
  }

    struct index_mm *index=(struct index_mm *)env->get_user_ptr(env, args[0]);

    int dic = env->extract_integer(env, args[1]);
    NON_LOCAL_EXIT_CHECK(env);
    int start = env->extract_integer(env, args[2]);
    NON_LOCAL_EXIT_CHECK(env);
    int end = env->extract_integer(env, args[3]);
    NON_LOCAL_EXIT_CHECK(env);


    struct dictionary *Dic=get_dictionary(index->Dic  , index->N, dic);
    
    char *definition = word_fetch(Dic, start, end);

    struct format_ format;
    format.tag=NULL;

    strip_tags(definition,&format);
    emacs_value result=render_emacs(env,&format);
    for(int i=0;i<format.N;i++)
      if(format.tag[i].properties)
	  free(format.tag[i].properties);

    free(format.tag);
    free(format.string);
    free(definition);
    return result;
}




static emacs_value
word_lookup(emacs_env *env, ptrdiff_t n, emacs_value *args, void *ptr)
{
    (void)ptr;
    (void)n;

    if (!env->is_not_nil(env, args[0])) {
      return SYM(env, "nil");
  }

    struct index_mm *index=(struct index_mm *)env->get_user_ptr(env, args[0]);

    char *Word;
    if (extract_string_arg(env, args[1], &Word)) {
      return SYM(env, "nil");
    }

    int is_prefix = env->extract_integer(env, args[2]);
    NON_LOCAL_EXIT_CHECK(env);


    struct article *Art=NULL;
    int art=0;

    look_for_a_word(index,Word,&art,&Art,is_prefix);
    emacs_value res;
    if(art>0){
    emacs_value *elts=xmalloc(art*sizeof(emacs_value));
    for(int j=0;j<art;j++)
      {
	elts[j] = env->make_string(env, Art[j].word,strlen(Art[j].word) );
	free(Art[j].word);
      }
    free(Art);
    res = make_list(env, art, elts);
    FREE(elts);
    }
    else
      res=SYM(env, "nil");
    
    return res;
}


static emacs_value
close_database(emacs_env *env, ptrdiff_t n, emacs_value *args, void *ptr)
{
    (void)ptr;
    (void)n;

    if (!env->is_not_nil(env, args[0]))
      return SYM(env, "nil");


    struct index_mm *index=(struct index_mm *)env->get_user_ptr(env, args[0]);

    NON_LOCAL_EXIT_CHECK(env);

    if (index ) {
      index_mm_close(index);
      env->set_user_ptr(env, args[0], 0);
    }
    return SYM(env, "nil");
}



void bind_func(emacs_env *env, const char *name, ptrdiff_t min, ptrdiff_t max, 
               emacs_value (*function) (emacs_env *env, ptrdiff_t nargs, emacs_value args[],void *) EMACS_NOEXCEPT, const char *doc)
{
  emacs_value fset = SYM(env, "fset");
  emacs_value args[2];

  args[0] = SYM(env, name);
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
    bind_func(env,"gdcv-word-defs",2,2,word_defs, "List of all dictionaries");
    bind_func(env,"gdcv-word-fetch",4,4,word_from_dictionary, "Return article from specific dictionary ");

    

    emacs_value provide = SYM(env, "provide");    
    emacs_value gdcv = SYM(env, "gdcv-elisp");
    env->funcall(env, provide, 1, &gdcv);
    return 0;
}
