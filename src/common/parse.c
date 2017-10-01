/* parse.c */

#include <stdio.h>
#include <malloc.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"


/* Open a file to be parsed.  Returns NULL on error. */
PARSE_FILE_STATE *open_parse_file(char *filename)
{
  PARSE_FILE_STATE *state;

  state = (PARSE_FILE_STATE *) malloc(sizeof(PARSE_FILE_STATE));
  if (state == NULL) {
    printf("Out of memory; can't read file `%s'\n", filename);
    return NULL;
  }

  state->f = fopen(filename, "r");
  if (state->f == NULL) {
    free(state);
    return NULL;
  }
  strcpy(state->filename, filename);
  state->pos = state->buffer;
  *state->pos = '\0';
  state->end_of_file = 0;
  state->line = 0;

  /* User-specific data */
  state->user_data = NULL;

  return state;
}


/* Read the next token from a file being parsed.  Returns 1 on
 * success, or 0 on error or if the end of the file was reached. */
int read_parse_token(PARSE_FILE_STATE *state, char *token)
{
  char *pos;

  while (42) {
    for (pos = state->pos; isspace(*pos); pos++)
      ;
    if (*pos == '#')
      *pos = '\0';
    state->pos = pos;

    if (*state->pos == '\0') {     /* Keep reading if rest of line empty */
      state->pos = state->buffer;
      state->buffer[0] = '\0';
      state->line++;
      if (fgets(state->buffer, 1024, state->f) == NULL) {
	*token = '\0';
	state->end_of_file = 1;
	return 0;
      }
    } else {                       /* Copy a token and return it */
      if (*pos == '\"') {
	pos++;      /* Skip the opening `"' */
	while (*pos != '\"') {
	  if (*pos == '\0') {
	    printf("%s:%d: parse error: expected closing `\"'\n",
		   state->filename, state->line);
	    state->end_of_file = 1;
	    return 0;
	  }
	  *token++ = *pos++;
	}
	pos++;      /* Skip the closing `"' */
      } else if (isalnum(*pos))
	while (*pos != '\0' && ! isspace(*pos))
	  *token++ = *pos++;
      else if (strchr("!@$%^&*()_+-=~`';:,.<>/?[]{}", *pos))
	*token++ = *pos++;
      *token = '\0';
      state->pos = pos;
      return 1;
    }
  } 
}


/* Close a file being parsed. */
void close_parse_file(PARSE_FILE_STATE *state)
{
  fclose(state->f);
  free(state);
}
