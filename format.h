#include "index.h"

typedef enum {
	      BOLD,
	      ITALIC,
	      UNDERLINE,
	      FULL_MODE,
	      MARGIN,
	      TRANSLATION,
	      EXAMPLE,
	      COMMENT,
	      URL,
	      NO_INDEX,
	      LABEL,
	      STRESS,
	      LANG,
	      REF,
	      SUB,
	      SUP,
	      COLOR,
	      MEDIA,
	      N_TAG
} TAG;

struct format_
{
  struct tag
  {
    TAG type;
    size_t pos;
    int open;
    char *properties;
  } *tag;
  int N;
  char *string;
};

extern void render_cli(struct dictionary D,struct format_ *format,int color,char *unzip);

extern void show_dictionaries(struct root_ D);

extern void strip_tags(char *src,struct format_ *format);
