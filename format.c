#define _GNU_SOURCE         ///

#include "format.h"
#include  <search.h>
#include "utils.h"
#include <stdio.h>                ////asprintf
#include <string.h> //strdup, memcpy, strcmp, strlen sscanf strtok
#include <stdio.h> //perror printf FILENAME_MAX

#define CLI_DIC_TITLE "\x1B[1m\x1B[31m"
#define CLI_RESET   "\x1B[0m"
#define CLI_BLACK   "\x1B[30m"      /* Black */
#define CLI_RED     "\x1B[31m"      /* Red */
#define CLI_YELLOW   "\x1B[33m"      /* Green */
#define CLI_GREEN   "\x1B[32m"      /* Green */
#define CLI_BLUE    "\x1B[34m"      /* Blue */
#define CLI_BOLD   "\x1B[1m"      /* Bold  */
#define CLI_FAINT   "\x1B[2m"      /* FAINT  */
#define CLI_UNDERLINE "\x1B[4m" /* Underline */
#define CLI_ITALIC "\x1B[3m" /* Italic */
#define CLI_REF "\x1B[1m\x1B[34m\x1B[4m"
#define CLI_LABELS "\x1B[3m\x1B[32m"



static char *color_name[139] = { "aliceblue","antiquewhite","aqua","aquamarine","azure",
				 "beige","bisque","blanchedalmond","blue","blueviolet",
				 "brown","burlywood","cadetblue","chartreuse","chocolate",
				 "coral","cornflowerblue","cornsilk","crimson","cyan",
				 "darkblue","darkcyan","darkgoldenrod","darkgray","darkgreen",
				 "darkkhaki","darkmagenta","darkolivegreen","darkorange","darkorchid",
				 "darkred","darksalmon","darkseagreen","darkslateblue","darkslategray",
				 "darkturquoise","darkviolet","deeppink","deepskyblue","dimgray",
				 "dodgerblue","firebrick","floralwhite","forestgreen","fuchsia",
				 "gainsboro","ghostwhite","gold","goldenrod","gray",
				 "green","greenyellow","honeydew","hotpink","indianred",
				 "indigo","ivory","khaki","lavender","lavenderblush",
				 "lawngreen","lemonchiffon","lightblue","lightcoral","lightcyan",
				 "lightgoldenrodyellow","lightgreen","lightgrey","lightpink","lightsalmon",
				 "lightseagreen","lightskyblue","lightslategray","lightsteelblue","lightyellow",
				 "lime","limegreen","linen","magenta","maroon",
				 "mediumaquamarine","mediumblue","mediumorchid","mediumpurple","mediumseagreen",
				 "mediumslateblue","mediumspringgreen","mediumturquoise","mediumvioletred","midnightblue",
				 "mintcream","mistyrose","moccasin","navajowhite","navy",
				 "oldlace","olive","olivedrab","orange","orangered",
				 "orchid","palegoldenrod","palegreen","paleturquoise","palevioletred",
				 "papayawhip","peachpuff","peru","pink","plum",
				 "powderblue","purple","red","rosybrown","royalblue",
				 "saddlebrown","salmon","sandybrown","seagreen","seashell",
				 "sienna","silver","skyblue","slateblue","slategray",
				 "snow","springgreen","steelblue","tan","teal",
				 "thistle","tomato","turquoise","violet","wheat",
				 "white","whitesmoke","yellow","yellowgreen"};

static char *ANSI[139] = {"\x1B[38:5:189m","\x1B[38:5:189m","\x1B[38:5:51m" ,"\x1B[38:5:122m","\x1B[38:5:195m",
			  "\x1B[38:5:188m","\x1B[38:5:223m","\x1B[38:5:224m","\x1B[38:5:21m" ,"\x1B[38:5:92m",
			  "\x1B[38:5:124m","\x1B[38:5:180m","\x1B[38:5:73m" ,"\x1B[38:5:118m","\x1B[38:5:172m",
			  "\x1B[38:5:209m","\x1B[38:5:68m" ,"\x1B[38:5:224m","\x1B[38:5:161m","\x1B[38:5:51m",
			  "\x1B[38:5:18m" ,"\x1B[38:5:30m" ,"\x1B[38:5:136m","\x1B[38:5:145m","\x1B[38:5:22m",
			  "\x1B[38:5:144m","\x1B[38:5:90m" ,"\x1B[38:5:64m" ,"\x1B[38:5:208m","\x1B[38:5:128m",
			  "\x1B[38:5:88m" ,"\x1B[38:5:174m","\x1B[38:5:108m","\x1B[38:5:60m" ,"\x1B[38:5:23m",
			  "\x1B[38:5:44m" ,"\x1B[38:5:92m" ,"\x1B[38:5:198m","\x1B[38:5:39m" ,"\x1B[38:5:102m",
			  "\x1B[38:5:33m" ,"\x1B[38:5:124m","\x1B[38:5:224m","\x1B[38:5:28m" ,"\x1B[38:5:201m",
			  "\x1B[38:5:188m","\x1B[38:5:189m","\x1B[38:5:220m","\x1B[38:5:178m","\x1B[38:5:102m",
			  "\x1B[38:5:28m" ,"\x1B[38:5:154m","\x1B[38:5:194m","\x1B[38:5:211m","\x1B[38:5:167m",
			  "\x1B[38:5:54m" ,"\x1B[38:5:230m","\x1B[38:5:186m","\x1B[38:5:188m","\x1B[38:5:224m",
			  "\x1B[38:5:112m","\x1B[38:5:224m","\x1B[38:5:152m","\x1B[38:5:174m","\x1B[38:5:195m",
			  "\x1B[38:5:188m","\x1B[38:5:114m","\x1B[38:5:188m","\x1B[38:5:217m","\x1B[38:5:216m",
			  "\x1B[38:5:37m" ,"\x1B[38:5:116m","\x1B[38:5:103m","\x1B[38:5:146m","\x1B[38:5:230m",
			  "\x1B[38:5:46m" ,"\x1B[38:5:40m" ,"\x1B[38:5:188m","\x1B[38:5:201m","\x1B[38:5:88m",
			  "\x1B[38:5:115m","\x1B[38:5:20m" ,"\x1B[38:5:134m","\x1B[38:5:104m","\x1B[38:5:72m",
			  "\x1B[38:5:104m","\x1B[38:5:43m" ,"\x1B[38:5:80m" ,"\x1B[38:5:126m","\x1B[38:5:18m",
			  "\x1B[38:5:194m","\x1B[38:5:224m","\x1B[38:5:223m","\x1B[38:5:223m","\x1B[38:5:18m",
			  "\x1B[38:5:188m","\x1B[38:5:100m","\x1B[38:5:100m","\x1B[38:5:214m","\x1B[38:5:202m",
			  "\x1B[38:5:176m","\x1B[38:5:187m","\x1B[38:5:114m","\x1B[38:5:152m","\x1B[38:5:174m",
			  "\x1B[38:5:224m","\x1B[38:5:223m","\x1B[38:5:173m","\x1B[38:5:217m","\x1B[38:5:182m",
			  "\x1B[38:5:152m","\x1B[38:5:90m" ,"\x1B[38:5:196m","\x1B[38:5:138m","\x1B[38:5:68m",
			  "\x1B[38:5:94m" ,"\x1B[38:5:174m","\x1B[38:5:179m","\x1B[38:5:29m" ,"\x1B[38:5:224m",
			  "\x1B[38:5:130m","\x1B[38:5:145m","\x1B[38:5:116m","\x1B[38:5:98m" ,"\x1B[38:5:102m",
			  "\x1B[38:5:224m","\x1B[38:5:48m" ,"\x1B[38:5:67m" ,"\x1B[38:5:180m","\x1B[38:5:30m",
			  "\x1B[38:5:182m","\x1B[38:5:203m","\x1B[38:5:80m" ,"\x1B[38:5:176m","\x1B[38:5:187m",
			  "\x1B[38:5:231m","\x1B[38:5:188m","\x1B[38:5:226m","\x1B[38:5:148m" };

extern void render_cli(struct dictionary D,struct format_ *format,int color,char *unzip)
{
  printf("\n%s%s%s\n", (color<2) ? CLI_DIC_TITLE : "", D.data[NAME], (color<2) ? CLI_RESET : "");
 ENTRY *found_item;
 ENTRY item;

 if(color<2)
   {
     if(hcreate(170) == 0) {
       printf("Error on hcreate\n");
       exit(EXIT_FAILURE);
     }
     
     for(int i=0;i<139;i++)
       {
	 item.key=color_name[i];
	 item.data=ANSI[i];
	 hsearch(item, ENTER);
       }
   }

  char *src=format->string;

  char* CLITagNames[N_TAG] =  {
			    CLI_BOLD,
			    CLI_ITALIC,
			    CLI_UNDERLINE,
			    CLI_FAINT,
			    "",   //[m0] [m1] [m2]...
			    "",
			    CLI_FAINT,
			    CLI_FAINT,
			    CLI_REF, //url
			    CLI_FAINT,
			    CLI_LABELS,
			    CLI_BOLD,
			    "", //[lang] [lang ]
			    CLI_REF ,//[ref] [ref ]
			    "",
			    "",
			    CLI_GREEN,   // [c] [c gray]   //The order is important. com will be find before c!
			    CLI_REF
  } ;

  if(color==2)
    for(int i=0;i<N_TAG;i++)
      CLITagNames[i]="";
 

  char *system_call;
  char list_of_media_files[FILENAME_MAX];
    
  TAG opened[N_TAG]={0};
  size_t skip=0;
  int len=0;
  char *temp_color;
  TAG type;
  int open;
  char *prop;
  temp_color="";

  strcpy(list_of_media_files, "");

  for(int i=0; i<format->N;i++)
    {
      type=format->tag[i].type;
      open=format->tag[i].open;
      prop=format->tag[i].properties;

      if(skip<=format->tag[i].pos)
	{
	  len=format->tag[i].pos-skip;
	  printf("%.*s", len, src + skip);
	  skip=format->tag[i].pos;
	  
	  opened[type]=open;

	  //	  if(type==MARGIN&&open)
	  //        printf("%*s", prop[0]-48, "");



	  if(open==0)
	    {
	      printf("%s",CLI_RESET);
	      for(int j=0;j<N_TAG;j++)
		if(opened[j]==1)
		  {
		    if(j==COLOR)
		      printf("%s",temp_color);
		  else
		    printf("%s",CLITagNames[j] );
		  }
	    }
	  else
	    {
	      if(type==COLOR&&color<2)
		{
		  temp_color=CLI_GREEN;
		  if(prop&&color==1)
		    {
		      item.key = prop;
		      found_item =  hsearch(item, FIND);
		      if(found_item != NULL)
			temp_color=(char *)found_item->data;
		    }
		  printf("%s",temp_color);
		}
	      else
		printf("%s",CLITagNames[type] ) ;

	      if(type==MEDIA&&D.data[RESOURCES]!=NULL&&strcmp(unzip,""))
		{
		  strcat(list_of_media_files,prop);
		  strcat(list_of_media_files," ");
		}
	    }
	}
    }


  if (list_of_media_files[0] != '\0')
    {
      int rc=0;
      rc=asprintf(&system_call, "unzip -qqju '%s' %s -d %s", D.data[RESOURCES],list_of_media_files,unzip);
      if(rc<0)
	{
	  printf("asprintf failed");
	  exit(EXIT_FAILURE);
	}
      rc=system(system_call);
      if(rc<0)
	{
	  printf("system call failed");
	  exit(EXIT_FAILURE);
	}

      free(system_call);
    }

  if(color<2)
    hdestroy();
}




static const char *table_format="%50.50s %10s %10s %10s %20s %10s ";

void show_dictionaries_tree(struct dictionary D) {
  enum DICTIONARY_STRUCTURE ext=NAME;

  if(D.to_index==0)
    {
      ext=0;
      while(ext<RESOURCES&&D.data[ext]==NULL) ext++;
    }
    
  printf(table_format ,D.data[ext], D.data[ICON]? "Y" : "N",D.data[FILE_ABRV]? "Y" : "N" ,D.data[RESOURCES]? "Y" : "N",D.encoding,D.to_index ? "Y":"N");
  printf("\n");
  
}

extern void show_dictionaries(struct root_ D)
{
  printf(table_format, "Dictionary Name|", "ICON|", "ABRV|", "RESOURCES|", "ENCODING|", "INDEXING|\n\t" );
  for (int i = 0; i < 116; i ++) putchar('=');
  printf("\n");
  for(int i=0;i<D.N;i++)
    show_dictionaries_tree(D.Dic[i]);

}



static void create_tag(struct format_ **format, int N,size_t pos, TAG type, int open)
{
  (*format)->tag=xrealloc((*format)->tag, (N+1) * sizeof(struct tag));
  (*format)->tag[N].properties=NULL;
  (*format)->tag[N].pos=pos;
  (*format)->tag[N].type=type;
  (*format)->tag[N].open=open;
}


static const char* GDTagNames[] = {
			    "b",
			    "i",
			    "u",
			    "*",
			    "m",   //[m0] [m1] [m2]...
			    "trn",
			    "ex",
			    "com]",
			    "url",
			    "!trs",
			    "p",
			    "'",
			    "lang", //[lang] [lang ]
			    "ref",//[ref] [ref ]
			    "sub]",
			    "sup]",
			    "c",   // [c] [c gray]   //The order is important. com will be find before c!
			    "s]"
};

extern void strip_tags(char *src,struct format_ *format)
{

 int len=strlen(src)+1;
 
 format->string=xmalloc(len*sizeof(char));
 
 char *orig=src;
 char *temp=src;

 int idx=0;
 
 int open=0;
 int i=0;
 int N=0;
 int word=0;
 
  for( ; *orig != '\0' ;orig++)
    switch(*orig)
      {
      case '\t':break;
      case '\n':
	if (word==0) word=idx;
	format->string[idx++]=*orig;
	break;
      case '}':
      case '{':
	if(*(orig+1)=='{')
	  while(*(++orig)!= '\0'&&*(orig-1)!='}'&&*(orig++)!='}');
	break;
      case  '[':
	open=1;
	orig++;
	if (*orig== '/')
	  {
	    open=0;
	    orig++;
	  }
	i=0;

	while(i<N_TAG&&strstr(orig, GDTagNames[i]) != orig)
	  i++;
	if(i<N_TAG)
	  {
	    create_tag(&format,N,idx, i, open);
	    
	    if(i==MARGIN&&open)
	      {
		orig++;
		format->tag[N].properties=xmalloc(2 * sizeof(char));
		format->tag[N].properties[0]='0';
		format->tag[N].properties[1]='\0';
		if(*orig>48&&*orig<58)
		  {
		    format->tag[N].properties[0]=*orig;
		    len+=*orig-48;
		    format->string=xrealloc(format->string, len);
		    for(int j=0;j<*orig-48;j++)
		      format->string[idx++]=' ';
		  }
		else
		  orig--;
	      }

	    if(i==COLOR&&open)
	      {
		orig++;
		if(*orig==' ')
		  {
		    temp=++orig;
		    while(*(orig+1) != '\0'&&*(orig+1)!=']')
		      orig++;
		    format->tag[N].properties=xmalloc((orig-temp+2) * sizeof(char));
		    memcpy(format->tag[N].properties, temp, (orig-temp)+2);
		    format->tag[N].properties[(orig-temp)+1]='\0';
		  }
		else
		  {
		    N++;
		    break;
		  }
	      }

	    
	    if(i==MEDIA&&open)
	      {
		orig++;
		temp=++orig;
		while(*(orig+1) != '\0'&&*(orig+1)!='[')
		    format->string[idx++]=*(orig++);

		format->string[idx++]=*orig;
		
		format->tag[N].properties=xmalloc((orig-temp+2) * sizeof(char));
		memcpy(format->tag[N].properties, temp, (orig-temp)+2);
		format->tag[N].properties[(orig-temp)+1]='\0';
		N++;
		break;
	      }

	    
	    N++;
	  }
	while(*orig != '\0'&&*(++orig)!=']');
	break;
      case '<':
	if(*(orig+1)=='<')
	  {
	    orig++;
	    create_tag(&format,N,idx, REF, 1);
	    N++;
	  }
	else
	    format->string[idx++]=*orig;
	break;

      case '>':
	if(*(orig+1)=='>')
	  {
	    orig++;
	    create_tag(&format,N,idx, REF, 0);
	    N++;
	  }
	else
	    format->string[idx++]=*orig;
	break;
      case '\\':
	orig++;
	format->string[idx++]=*orig;
	break;
      case '~':
	len+=word;
	format->string=xrealloc(format->string, len);
	temp=src;
	for(int j=0;j<word;j++)
	  format->string[idx++]=*(temp++);
	break;
      default:
	format->string[idx++]=*orig;
	break;
      }

  format->string[idx++]= '\0';
  format->N=N;
  
  
  return ;
}
