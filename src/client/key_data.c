/* key_data.c */

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "key_data.h"
#include "parse.h"
#include "key_name.h"
#include "game.h"


#if 0
static int get_key_name(int code, char *name)
{
  int i;

  if (code < 0) {
    *name = '\0';
    return 1;
  }

  for (i = 0; key_name_table[i].name != NULL; i++)
    if (key_name_table[i].code == code) {
      while (key_name_table[i].code == code)
	i++;
      strcpy(name, key_name_table[i-1].name);
      return 0;
    }
  *name = '\0';
  return 1;
}
#endif

static int get_key_code(char *name)
{
  int i;

  for (i = 0; key_name_table[i].name != NULL; i++)
    if (strcmp(key_name_table[i].name, name) == 0)
      return key_name_table[i].code;
  return -1;
}


#define PARSE_ERROR(state, msg)						\
  do {									\
    printf("%s:%d: %s\n", (state)->filename, (state)->line, msg);	\
    return -1;								\
  } while (0)

#define PARSE_ERROR_STR(state, msg, str)				\
  do {									\
    printf("%s:%d: " msg "\n", (state)->filename, (state)->line, str);	\
    return -1;								\
  } while (0)

#define PARSE_ERROR_INT(state, msg, i)					\
  do {									\
    printf("%s:%d: " msg "\n", (state)->filename, (state)->line, i);	\
    return -1;								\
  } while (0)


static int config_keyboard(PARSE_FILE_STATE *state, KEY_CONFIG *info)
{
  char action[256], key_name[256];
  int key_code;

  while (read_parse_token(state, action) && strcmp(action, "}") != 0) {
    if (! read_parse_token(state, key_name))
      PARSE_ERROR(state, "expected: key name");
    key_code = get_key_code(key_name);
    if (key_code < 0)
      PARSE_ERROR_STR(state, "`%s' is not a valid key", key_name);

#define CHECK_KEY(name, var) (strcmp(action, name) == 0) var = key_code
    if CHECK_KEY("left", info->left);
    else if CHECK_KEY("right", info->right);
    else if CHECK_KEY("jump", info->jump);
    else if CHECK_KEY("down", info->down);
    else if CHECK_KEY("shoot", info->shoot);
    else if CHECK_KEY("use", info->use);
    else if CHECK_KEY("show_map", info->show_map);
    else if CHECK_KEY("show_fps", info->show_fps);
    else if CHECK_KEY("show_parallax", info->show_parallax);
    else if CHECK_KEY("show_credits", info->show_credits);
    else if CHECK_KEY("pause_credits", info->pause_credits);
    else if CHECK_KEY("screen_follow", info->screen_follow);
    else if CHECK_KEY("screen_up", info->screen_up);
    else if CHECK_KEY("screen_down", info->screen_down);
    else if CHECK_KEY("screen_left", info->screen_left);
    else if CHECK_KEY("screen_right", info->screen_right);
#undef CHECK_KEY
    else
      PARSE_ERROR_STR(state, "`%s' is not a valid action", action);
  }
  return 0;
}

int parse_key_info(KEY_CONFIG *info, char *filename)
{
  PARSE_FILE_STATE *state;
  char token[256], old_token[256] = "";
  int done_ok = 0, ret = 0;

  /* Default keys */
  info->left = SCANCODE_CURSORBLOCKLEFT;
  info->right = SCANCODE_CURSORBLOCKRIGHT;
  info->jump= SCANCODE_CURSORBLOCKUP;
  info->down = SCANCODE_CURSORBLOCKDOWN;
  info->shoot = SCANCODE_X;
  info->use = SCANCODE_TAB;
  info->show_map = SCANCODE_M;
  info->show_fps = SCANCODE_SLASH;
  info->show_parallax = SCANCODE_P;
  info->show_credits = SCANCODE_PERIOD;
  info->pause_credits = SCANCODE_SPACE;
  info->screen_follow = SCANCODE_F;
  info->screen_up = SCANCODE_I;
  info->screen_down = SCANCODE_K;
  info->screen_left = SCANCODE_J;
  info->screen_right = SCANCODE_L;

  if ((state = open_parse_file(filename)) == NULL)
    return -1;

  while (! done_ok && read_parse_token(state, token))
    if (strcmp(token, "{") == 0 && strcmp(old_token, "KEYBOARD") == 0) {
      ret = config_keyboard(state, info);
      done_ok = 1;
    } else
      strcpy(old_token, token);
  if (! done_ok) {
    printf("%s:%d: expected: KEYBOARD section\n",state->filename,state->line);
    ret = -1;
  }

  close_parse_file(state);
  return ret;
}
