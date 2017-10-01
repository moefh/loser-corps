/* spr_data.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "spr_data.h"
#include "parse.h"


#define PARSE_ERROR(state, msg)						\
  do {									\
    printf("%s:%d: %s\n", (state)->filename, (state)->line, msg);	\
    return -1;								\
  } while (0);

#define PARSE_ERROR_STR(state, msg, str)				\
  do {									\
    printf("%s:%d: " msg "\n", (state)->filename, (state)->line, str);	\
    return -1;								\
  } while (0);

#define PARSE_ERROR_INT(state, msg, i)					 \
  do {									 \
    printf("%s:%d: " msg "\n", (state)->filename, (state)->line, i);	 \
    return -1;								 \
  } while (0);



static int prop_func_author(PARSE_FILE_STATE *state, TILESET_FILE *file)
{
  if (! read_parse_token(state, file->author))
    PARSE_ERROR(state, "expected author name");
  return 0;
}

static int prop_func_comment(PARSE_FILE_STATE *state, TILESET_FILE *file)
{
  if (! read_parse_token(state, file->comment))
    PARSE_ERROR(state, "expected comment");
  return 0;
}

static int prop_func_copyright(PARSE_FILE_STATE *state, TILESET_FILE *file)
{
  if (! read_parse_token(state, file->copyright))
    PARSE_ERROR(state, "expected copyright information");
  return 0;
}


static struct PROP_FUNC_TABLE {
  char *prop;
  int (*func)(PARSE_FILE_STATE *, TILESET_FILE *);
} prop_func_table[] = {
  { "author", prop_func_author },
  { "comment", prop_func_comment },
  { "copyright", prop_func_copyright },
  { NULL }
};



static void fill_default_values(TILESET_FILE *file)
{
  *file->name = *file->author = *file->comment = '\0';
}

static int read_tileset_defs(PARSE_FILE_STATE *state, TILESET_FILE *file)
{
  char name[256], token[256];
  int n;

  if (! read_parse_token(state, name)) {
    printf("%s:%d: expected tileset name\n", state->filename, state->line);
    return -1;
  }
  if (! read_parse_token(state, token) || strcmp(token, "{") != 0) {
    printf("%s:%d: expected `{'\n", state->filename, state->line);
    return -1;
  }

  fill_default_values(file);
  strcpy(file->name, name);

  while (read_parse_token(state, token) && strcmp(token, "}") != 0) {
    for (n = 0; prop_func_table[n].prop != NULL; n++)
      if (strcmp(prop_func_table[n].prop, token) == 0) {
	if (prop_func_table[n].func(state, file) < 0)
	  return -1;
	break;
      }

    if (prop_func_table[n].prop == NULL) {  /* Property is not in the table */
      printf("%s:%d: unknown TILESET property: `%s'\n",
	     state->filename, state->line, token);
      return -1;
    }
  }

  return 0;
}

int parse_tileset_info(TILESET_INFO *info, char *filename)
{
  PARSE_FILE_STATE *state;
  char token[256];
  TILESET_FILE *file = NULL;
  int n_files = 0, alloc_files = 0;

  if ((state = open_parse_file(filename)) == NULL) {
    printf("Error reading file `%s'.\n", filename);
    return -1;
  }

  info->n_tilesets = 0;
  info->tileset = NULL;

  while (read_parse_token(state, token)) {
    if (strcmp(token, "TILESET") != 0) {
      printf("%s:%d: expected `TILESET' definition\n",
	     state->filename, state->line);
      close_parse_file(state);
      return -1;
    }

    if (n_files >= alloc_files) {
      TILESET_FILE *p;

      if (alloc_files == 0) {
	alloc_files = 16;
	p = (TILESET_FILE *) malloc(sizeof(TILESET_FILE) * alloc_files);
      } else {
	alloc_files += 16;
	p = (TILESET_FILE *) realloc(file, sizeof(TILESET_FILE)*alloc_files);
      }
      if (p == NULL) {
	printf("%s: out of memory to read definition\n", state->filename);
	free(file);
	close_parse_file(state);
	return -1;
      }
      file = p;
    }

    if (read_tileset_defs(state, file + n_files)) {
      close_parse_file(state);
      free(file);
      return -1;
    }
    n_files++;
  }

  info->tileset = file;
  info->n_tilesets = n_files;
  return 0;
}
