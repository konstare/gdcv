#include <stdlib.h> //realpath
#include <sys/stat.h> //mkdir
#include <argp.h> //argp
#include <error.h> //argp
#include <string.h> // strcmp
#include <stdio.h> //perror printf FILENAME_MAX

#include "utils.h"
#include "index.h"
#include "format.h"

const char *argp_program_version = "gdcv 0.1";
const char *argp_program_bug_address =  "<somewhere>";

static char doc[] = "Command Line version of GoldenDict"
  "\noptions:"
  "\vTo look up a word \"cat\": gdcv cat.\n"
  "To search words with substring \"cat\": gdcv *cat\n"
  "To index directory: gdcv -i /usr/share/dict";

/* A description of the arguments we accept. */
static char args_doc[] = "WORD or -i PATH";

static struct argp_option options[] = {
				       {"index",   'i', 0,  0, "Index dictionary files in the chosen directory" ,0},
				       {"color",   'c', "THEME",  0, "The color theme of the output: default, decolorized, full" ,0},
				       {"unzip",   'u', "DIRECTORY",  0, "Unzip resource files" ,0},
				       { 0 }
};

struct arguments_
{
  char *word;
  char *unzip;
  int to_index;
  char **index_path;
  int color;
};


static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments_ *arguments = state->input;
  switch (key)
    {
    case 'u':
      arguments->unzip = arg;
      break;
    case 'i':
      arguments->to_index = 1;
      break;
    case 'c':
      if(strcmp(arg, "default")==0)
	arguments->color = 0;
      else if (strcmp(arg, "full")==0)
	arguments->color = 1;
      else if (strcmp(arg, "nocolor")==0)
	arguments->color = 2;
      else
	{
	  printf("Expected argument for option -c is one of {default, full, nocolor}.");
	  arguments->color = 0;
	}
      break;

      
    case ARGP_KEY_NO_ARGS:
      argp_usage (state);
      break;
      
    case ARGP_KEY_ARG:
      if(arguments->to_index)
	{
	  arguments->index_path = &state->argv[state->next-1];
	  state->next = state->argc;
	}
      else
	arguments->word=arg;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc , 0, 0, 0 };






void render_cli(struct dictionary D,struct format_ *format,int color,char *unzip);


int main(int argc, char *argv[])  
{
  struct arguments_ arguments={"","",0,NULL,0};

  argp_parse (&argp, argc, argv, 0, 0, &arguments);

  char *index_path=NULL;
  
  if (getenv("XDG_CONFIG_HOME") != NULL) 
    xasprintf(&index_path, "%s/gdcv", getenv("XDG_CONFIG_HOME"));
  else if ( getenv("HOME") != NULL )
    xasprintf(&index_path, "%s/.config/gdcv", getenv("HOME"));
  else
    {
      printf("The variables HOME and XDG_CONFIG_HOME are not defined");
      exit(EXIT_FAILURE);
    }


 
  mkdir(index_path, 0755); 
   


  if(arguments.to_index)
    {
      int N=0;
      char **paths=NULL;

      for(int i=0;arguments.index_path[i];i++)
	{
	  char *temp_path=realpath(arguments.index_path[i], NULL);

	  if(!temp_path)
	    printf("The directory %s does not exist\n",arguments.index_path[i]);
	  else
	    {
	      paths= xrealloc(paths, (N+1)*sizeof(char *));
	      paths[N]=temp_path;
	      N++;
	    }
	  
	}

      if(paths==NULL)
	  exit(EXIT_SUCCESS);

      struct root_ D = index_directories (paths);

      for(int i=0;i<N;i++)
	free(paths[i]);
      free(paths);

      
      show_dictionaries(D);

      if(!D.indexed)
	{
	  printf("Nothing to index");
	  exit(EXIT_FAILURE);
	}


      printf ( "\nPress [Enter] to continue . . ." );
      fflush ( stdout ); getchar();

      
      struct index_node *index;
      index = index_create(D);

      index_write(index, D, index_path);
      index_destroy(index);
      root_destroy(D);
    }
  else{
    struct index_mm *index;
    index=index_mm_open(index_path);
    if(!index)
      {
	printf("The index files do not exist or cannot be read\nPlease use gdcv -i 'Path to dictionaries in GoldenDict format'");
	exit(EXIT_FAILURE);  
      }

    if(arguments.word)
      {if(arguments.word[0]!='*')
	{
	int art=0;
	struct article *Art=NULL;
	int rc;
	if((rc=decode_articles(arguments.word,&Art,&art,index))==0)
	  {
	    for(int i=0;i<art;i++)
	      {
		struct dictionary *Dic=get_dictionary(index->Dic  , index->N, Art[i].dic);
		Art[i].full_definition= word_fetch(Dic, Art[i].start, Art[i].size);

		 		

		struct format_ format; 
		format.tag=NULL; 
		strip_tags(Art[i].full_definition,&format);
		render_cli(*Dic, &format,arguments.color,arguments.unzip);

		free(Art[i].full_definition);
		free(Art[i].word);
		
		free_format(&format);

	      }



	    free(Art); 
	  }
	else
	  printf("The word %s is not found", arguments.word);
      }

	
	if(arguments.word[0]=='*')
	  {
	    int art=0;
	    struct article *Art=NULL;
	    look_for_a_word(index,&arguments.word[1],&art, &Art,0);
	    if(art>0)
	      printf("Similar words\n");
	      
	    for (int i=0; i<art;i++)
	      printf("%s\n",Art[i].word);

	    for(int i=0; i<art;i++)
		free(Art[i].word);
	    free(Art);
	      	    
	  }
      }

    
    index_mm_close(index);
  }
    
  


  free(index_path);
  exit(EXIT_SUCCESS);
}

