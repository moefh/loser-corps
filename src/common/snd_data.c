/* snd_data.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "snd_data.h"
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



static int prop_func_file(PARSE_FILE_STATE *state, SOUND_FILE *file)
{
  if (! read_parse_token(state, file->file))
    PARSE_ERROR(state, "expected file name");
  return 0;
}

static int prop_func_author(PARSE_FILE_STATE *state, SOUND_FILE *file)
{
  if (! read_parse_token(state, file->author))
    PARSE_ERROR(state, "expected author name");
  return 0;
}

static int prop_func_comment(PARSE_FILE_STATE *state, SOUND_FILE *file)
{
  if (! read_parse_token(state, file->comment))
    PARSE_ERROR(state, "expected comment");
  return 0;
}

static int prop_func_copyright(PARSE_FILE_STATE *state, SOUND_FILE *file)
{
  if (! read_parse_token(state, file->copyright))
    PARSE_ERROR(state, "expected copyright information");
  return 0;
}


static struct PROP_FUNC_TABLE {
  char *prop;
  int (*func)(PARSE_FILE_STATE *, SOUND_FILE *);
} prop_func_table[] = {
  { "file", prop_func_file },
  { "author", prop_func_author },
  { "comment", prop_func_comment },
  { "copyright", prop_func_copyright },
  { NULL }
};


static int read_sound_def(PARSE_FILE_STATE *state, SOUND_FILE *file)
{
  char token[256], name[256];
  int n;

  if (! read_parse_token(state, name)) {
    printf("%s:%d: expected definition name\n", state->filename, state->line);
    return 1;
  }
  if (! read_parse_token(state, token) || strcmp(token, "{") != 0) {
    printf("%s:%d: expected `{'\n", state->filename, state->line);
    return 1;
  }

  strcpy(file->name, name);

  while (read_parse_token(state, token) && strcmp(token, "}") != 0) {
    for (n = 0; prop_func_table[n].prop != NULL; n++)
      if (strcmp(prop_func_table[n].prop, token) == 0) {
	if (prop_func_table[n].func(state, file) < 0)
	  return -1;
	break;
      }

    if (prop_func_table[n].prop == NULL) {  /* Property is not in the table */
      printf("%s:%d: unknown sound property: `%s'\n",
	     state->filename, state->line, token);
      return -1;
    }
  }

  return 0;
}

int parse_sound_info(SOUND_INFO *info, char *filename)
{
  PARSE_FILE_STATE *state;
  char token[256];

  if ((state = open_parse_file(filename)) == NULL) {
    printf("Error reading file `%s'.\n", filename);
    return -1;
  }

  info->n_samples = 0;
  info->sample = NULL;
  info->n_musics = 0;
  info->music = NULL;

  while (read_parse_token(state, token)) {
    if (strcmp(token, "MIDI") != 0 && strcmp(token, "SAMPLE") != 0) {
      printf("%s:%d: expected `MIDI' or `SAMPLE' definition\n",
	     state->filename, state->line);
      close_parse_file(state);
      return -1;
    }

    if (strcmp(token, "MIDI") == 0) {
      SOUND_FILE *p;

      info->n_musics++;
      if (info->music == NULL)
        p = (SOUND_FILE *) malloc(sizeof(SOUND_FILE) * info->n_musics);
      else
        p = (SOUND_FILE *) realloc(info->music,
				   sizeof(SOUND_FILE) * info->n_musics);
      if (p == NULL) {
	printf("%s: out of memory to read file\n", state->filename);
	if (info->music) free(info->music);
	if (info->sample) free(info->sample);
	close_parse_file(state);
	return -1;
      }
      info->music = p;

      if (read_sound_def(state, info->music + info->n_musics - 1)) {
	if (info->music) free(info->music);
	if (info->sample) free(info->sample);
	close_parse_file(state);
	return -1;
      }
    } else if (strcmp(token, "SAMPLE") == 0) {
      SOUND_FILE *p;

      info->n_samples++;
      if (info->sample == NULL)
        p = (SOUND_FILE *) malloc(sizeof(SOUND_FILE) * info->n_samples);
      else
        p = (SOUND_FILE *) realloc(info->sample,
				   sizeof(SOUND_FILE) * info->n_samples);
      if (p == NULL) {
	printf("%s: out of memory to read file\n", state->filename);
	if (info->music) free(info->music);
	if (info->sample) free(info->sample);
	close_parse_file(state);
	return -1;
      }
      info->sample = p;

      if (read_sound_def(state, info->sample + info->n_samples - 1)) {
	if (info->music) free(info->music);
	if (info->sample) free(info->sample);
	close_parse_file(state);
	return -1;
      }
    }
  }

  /* Fix the ID numbers */
  {
    int i;

    for (i = 0; i < info->n_samples; i++)
      info->sample[i].id = i;
    for (i = 0; i < info->n_musics; i++)
      info->music[i].id = i;
  }

  return 0;
}
