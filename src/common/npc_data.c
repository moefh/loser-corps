/* npc_data.c */


#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "npc_data.h"
#include "parse.h"


typedef struct JACK_WEAPON {
  char jack_name[256];
  int jack_npc;                    /* Jack NPC */
  int weapon_num;                  /* Jack weapon number */
  char weapon_name[256];           /* Weapon NPC name */
} JACK_WEAPON;

typedef struct WEAPON_NAME {
  char name[256];                  /* Weapon NPC name */
  int num;                         /* Weapon NPC number */
} WEAPON_NAME;

typedef struct WEAPON_NPC_INFO {
  int n_jacks, n_weapons;
  JACK_WEAPON jack[MAX_NPCS];
  WEAPON_NAME weapon[MAX_NPCS];
} WEAPON_NPC_INFO;

typedef struct STATE_USER_DATA {
  int mirror_start;
  char *npc_name;
  WEAPON_NPC_INFO *weapons;
} STATE_USER_DATA;


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


/* Return the NPC type, given its name */
static int npc_type_name(char *name)
{
  if (strcmp(name, "JACK") == 0)
    return NPC_TYPE_JACK;
  if (strcmp(name, "WEAPON") == 0)
    return NPC_TYPE_WEAPON;
  if (strcmp(name, "ENEMY") == 0)
    return NPC_TYPE_ENEMY;
  if (strcmp(name, "ITEM") == 0)
    return NPC_TYPE_ITEM;
  return -1;
}

/* Return the drawing mode code from the description */
static int get_draw_mode(char *mode)
{
  if (strcmp(mode, "normal") == 0)
    return NPC_DRAW_NORMAL;
  if (strcmp(mode, "none") == 0)
    return NPC_DRAW_NONE;
  if (strcmp(mode, "alpha25") == 0)
    return NPC_DRAW_ALPHA25;
  if (strcmp(mode, "alpha50") == 0)
    return NPC_DRAW_ALPHA50;
  if (strcmp(mode, "alpha75") == 0)
    return NPC_DRAW_ALPHA75;
  if (strcmp(mode, "lighten") == 0)
    return NPC_DRAW_LIGHTEN;
  if (strcmp(mode, "darken") == 0)
    return NPC_DRAW_DARKEN;
  if (strcmp(mode, "displace") == 0)
    return NPC_DRAW_DISPLACE;
  if (strcmp(mode, "parallax") == 0)
    return NPC_DRAW_PARALLAX;
  return -1;
}

/* Return the floating mode code from the description */
static int get_float_mode(char *mode)
{
  if (strcmp(mode, "fast") == 0)
    return ITEM_FLOAT_FAST;
  if (strcmp(mode, "slow") == 0)
    return ITEM_FLOAT_SLOW;
  if (strcmp(mode, "none") == 0)
    return ITEM_FLOAT_NONE;
  return -1;
}

/* Return the layer position (fg/bg) */
static int get_layer_pos(char *mode)
{
  if (strcmp(mode, "fg") == 0 || strcmp(mode, "foreground") == 0)
    return NPC_LAYER_FG;
  if (strcmp(mode, "bg") == 0 || strcmp(mode, "background") == 0)
    return NPC_LAYER_BG;
  return -1;
}

/* Return the boolean value (0/1 for false/true), or -1 on error */
static int get_bool_val(char *val)
{
  if (strcmp(val, "yes")==0 || strcmp(val, "true")==0 || strcmp(val, "1")==0)
    return 1;
  if (strcmp(val, "no")==0 || strcmp(val, "false")==0 || strcmp(val, "0")==0)
    return 0;
  return -1;
}

/* Read a list of integers enclosed between '{' and '}'. Returns the
 * number of integers read */
static int read_int_list(PARSE_FILE_STATE *state, int *list)
{
  char token[256];
  int n = 0;

  if (! read_parse_token(state, token) || strcmp(token, "{") != 0)
    return -1;

  n = 0;
  while (read_parse_token(state, token) && n < 256) {
    if (strcmp(token, "}") == 0)
      return n;
    list[n++] = atoi(token);
  }
  return -1;    /* Reached end of file without reaching a '}' */
}

/* Add a weapon definition so we can later resolve weapon references
 * for JACKs */
static int add_weapon_def(PARSE_FILE_STATE *state, char *npc_name, int npc_id)
{
  WEAPON_NPC_INFO *weapons;

  weapons = ((STATE_USER_DATA *) state->user_data)->weapons;
  if (weapons->n_weapons >= MAX_NPCS)
    return 1;
  strcpy(weapons->weapon[weapons->n_weapons].name, npc_name);
  weapons->weapon[weapons->n_weapons].num = npc_id;
  weapons->n_weapons++;
  return 0;
}

/* Add a weapon definition to the jack */
static int add_jack_weapon(PARSE_FILE_STATE *state, char *jack_name,
			   int jack_npc, int weapon_num, char *weapon_name)
{
  WEAPON_NPC_INFO *weapons;

  weapons = ((STATE_USER_DATA *) state->user_data)->weapons;
  if (weapons->n_jacks >= MAX_NPCS)
    return 1;
  strcpy(weapons->jack[weapons->n_jacks].jack_name, jack_name);
  weapons->jack[weapons->n_jacks].jack_npc = jack_npc;
  weapons->jack[weapons->n_jacks].weapon_num = weapon_num;
  strcpy(weapons->jack[weapons->n_jacks].weapon_name, weapon_name);
  weapons->n_jacks++;
  return 0;
}


/*
 * These are the functions to read the NPC properties.
 */

static int prop_func_sprite(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  if (! read_parse_token(state, info->file))
    PARSE_ERROR(state, "expected sprite file name");
  return 0;
}

static int prop_func_lib(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  if (! read_parse_token(state, info->lib_name))
    PARSE_ERROR(state, "expected library name");
  return 0;
}

static int prop_func_clip(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  char token[256];

  if (! read_parse_token(state, token))
    PARSE_ERROR(state, "expected x offset for clipping rectangle");
  info->clip_x = atoi(token);
  if (! read_parse_token(state, token))
    PARSE_ERROR(state, "expected y offset for clipping rectangle");
  info->clip_y = atoi(token);
  if (! read_parse_token(state, token))
    PARSE_ERROR(state, "expected width for clipping rectangle");
  info->clip_w = atoi(token);
  if (! read_parse_token(state, token))
    PARSE_ERROR(state, "expected height for clipping rectangle");
  info->clip_h = atoi(token);
  return 0;
}

static int prop_func_mirror(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  char token[256];
  STATE_USER_DATA *user_data;

  user_data = (STATE_USER_DATA *) state->user_data;
  if (! read_parse_token(state, token))
    PARSE_ERROR(state, "expected frame number for mirror start");
  user_data->mirror_start = atoi(token);
  return 0;
}

static int prop_func_stand(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  int n;
  STATE_USER_DATA *user_data;

  user_data = (STATE_USER_DATA *) state->user_data;
  info->s_stand = 0;
  info->n_stand = read_int_list(state, info->frame);
  if (info->n_stand <= 0)
    PARSE_ERROR(state, "error reading `stand' frame list");
  for (n = info->s_stand; n < info->s_stand + info->n_stand; n++)
    info->frame[n + info->n_stand] = info->frame[n] + user_data->mirror_start;
  info->n_stand *= 2;
  return 0;
}

static int prop_func_jump(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  int n;
  STATE_USER_DATA *user_data;

  user_data = (STATE_USER_DATA *) state->user_data;
  info->s_jump = info->n_stand;
  info->n_jump = read_int_list(state, info->frame + info->s_jump);
  if (info->n_jump <= 0)
    PARSE_ERROR(state, "error reading `jump' frame list");
  for (n = info->s_jump; n < info->s_jump + info->n_jump; n++)
    info->frame[n + info->n_jump] = info->frame[n] + user_data->mirror_start;
  info->n_jump *= 2;
  return 0;
}

static int prop_func_walk(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  int n;
  STATE_USER_DATA *user_data;

  user_data = (STATE_USER_DATA *) state->user_data;
  info->s_walk = info->n_stand + info->n_jump;
  info->n_walk = read_int_list(state, info->frame + info->s_walk);
  if (info->n_walk <= 0)
    PARSE_ERROR(state, "error reading `walk' frame list");
  for (n = info->s_walk; n < info->s_walk + info->n_walk; n++)
    info->frame[n + info->n_walk] = info->frame[n] + user_data->mirror_start;
  info->n_walk *= 2;
  return 0;
}

static int prop_func_weapon(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  char token[256];
  int num_weapon;
  STATE_USER_DATA *user_data;

  user_data = (STATE_USER_DATA *) state->user_data;

  if (! read_parse_token(state, token))
    PARSE_ERROR(state, "error reading weapon definition");
  num_weapon = atoi(token);
  if (num_weapon < 0 || num_weapon >= MAX_WEAPONS)
    PARSE_ERROR_INT(state, "invalid weapon number (%d)", num_weapon);

  if (! read_parse_token(state, token))
    PARSE_ERROR(state, "error reading weapon definition");
  info->weapon_y[num_weapon] = atoi(token);

  if (! read_parse_token(state, token))
    PARSE_ERROR(state, "error reading weapon definition");
  info->weapon_npc[num_weapon] = -1;
  add_jack_weapon(state, user_data->npc_name, i, num_weapon, token);
  return 0;
}

static int prop_func_draw(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  char token[256];

  if (! read_parse_token(state, token))
    PARSE_ERROR(state, "error reading draw mode");
  info->draw_mode = get_draw_mode(token);
  if (info->draw_mode < 0)
    PARSE_ERROR_STR(state, "unknown draw mode: `%s'", token);
  return 0;
}

static int prop_func_layer(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  char token[256];

  if (! read_parse_token(state, token))
    PARSE_ERROR(state, "error reading layer position");
  info->layer_pos = get_layer_pos(token);
  if (info->layer_pos < 0)
    PARSE_ERROR_STR(state, "unknown layer position: `%s'", token);
  return 0;
}

static int prop_func_float(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  char token[256];

  if (! read_parse_token(state, token))
    PARSE_ERROR(state, "error reading floating mode");
  info->float_mode = get_float_mode(token);
  if (info->float_mode < 0)
    PARSE_ERROR_STR(state, "unknown floating mode: `%s'", token);
  return 0;
}

static int prop_func_speed(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  char token[256];

  if (! read_parse_token(state, token))
    PARSE_ERROR(state, "error reading speed");
  info->speed = atoi(token);
  return 0;
}

static int prop_func_shootframe(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  char token[256];

  if (! read_parse_token(state, token))
    PARSE_ERROR(state, "error reading shoot_frame");
  info->shoot_frame = atoi(token);
  return 0;
}

static int prop_func_accel(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  char token[256];

  if (! read_parse_token(state, token))
    PARSE_ERROR(state, "error reading accel");
  info->accel = atoi(token);
  return 0;
}


static int prop_func_block(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  char token[256];

  if (! read_parse_token(state, token))
    PARSE_ERROR(state, "error reading block setting");
  info->block = get_bool_val(token);
  if (info->block < 0) {
    PARSE_ERROR_STR(state, "invalid setting to `block': \"%s\"\n", token);
    return 1;
  }
  return 0;
 
}

static int prop_func_showhit(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  char token[256];

  if (! read_parse_token(state, token))
    PARSE_ERROR(state, "error reading show_hit setting");
  info->show_hit = get_bool_val(token);
  if (info->show_hit < 0) {
    PARSE_ERROR_STR(state, "invalid setting to `show_hit': \"%s\"\n", token);
    return 1;
  }
  return 0;
}

static int prop_func_secret(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  int n_secret, secret[256], j;

  n_secret = read_int_list(state, secret);
  if (n_secret <= 0)
    PARSE_ERROR(state, "error reading `secret' list");
  for (j = 0; j < n_secret; j++)
    info->secret[j] = secret[j];
  info->secret[n_secret] = 255;
  return 0;
}

/* The table of NPC properties and the functions to read them */
static struct PROP_FUNC_TABLE {
  char *prop;
  int (*func)(PARSE_FILE_STATE *, NPC_INFO *, int);
} prop_func_table[] = {
  { "sprite", prop_func_sprite },
  { "lib", prop_func_lib },
  { "clip", prop_func_clip },
  { "speed", prop_func_speed },
  { "accel", prop_func_accel },
  { "draw", prop_func_draw },
  { "layer", prop_func_layer },
  { "float", prop_func_float },
  { "clip", prop_func_clip },
  { "mirror", prop_func_mirror },
  { "stand", prop_func_stand },
  { "jump", prop_func_jump },
  { "walk", prop_func_walk },
  { "weapon", prop_func_weapon },
  { "show_hit", prop_func_showhit },
  { "block", prop_func_block },
  { "shoot_frame", prop_func_shootframe },
  { "secret", prop_func_secret },
  { NULL, NULL },
};


/* Fill default values for an NPC info structure */
static void set_info_defaults(NPC_INFO *info)
{
  int n;

  info->lib_name[0] = '\0';
  info->speed = 15;
  info->accel = 0;
  info->show_hit = 1;
  info->block = 0;
  info->draw_mode = NPC_DRAW_NORMAL;
  info->float_mode = ITEM_FLOAT_FAST;
  info->layer_pos = NPC_LAYER_FG;
  info->secret[0] = 255;
  info->n_stand = info->s_stand = 0;
  info->n_jump = info->s_jump = 0;
  info->n_walk = info->s_walk = 0;
  info->shoot_frame = 0;
  for (n = 0; n < MAX_WEAPONS; n++) {
    info->weapon_npc[n] = -1;
    info->weapon_y[n] = 0;
  }
}

/* Parse a NPC definition. The format is described in the file
 * `docs/npc_defs' in the game distribution. */
static int read_npc_def(PARSE_FILE_STATE *state, NPC_INFO *info, int i)
{
  STATE_USER_DATA *user_data;
  char str_type[256], name[256], token[1024];
  int n;

  user_data = (STATE_USER_DATA *) state->user_data;
  user_data->npc_name = name;

  if (read_parse_token(state, str_type) == 0)
    return 0;    /* End of file */

  if (read_parse_token(state, name) == 0 ||
      read_parse_token(state, token) == 0 ||
      token[0] != '{' || token[1] != '\0') {
    printf("%s:%d: expected start of NPC definition\n",
	   state->filename, state->line);
    return -1;
  }

  info->type = npc_type_name(str_type);
  if (info->type < 0) {
    printf("%s:%d: unknown NPC type `%s'\n",
	   state->filename, state->line, str_type);
    return -1;
  }

  if (info->type == NPC_TYPE_WEAPON || info->type == NPC_TYPE_ENEMY)
    add_weapon_def(state, name, i);

  strcpy(info->npc_name, name);
  set_info_defaults(info);

  /* Parse the properties */
  while (read_parse_token(state, token) && strcmp(token, "}") != 0) {
    for (n = 0; prop_func_table[n].prop != NULL; n++)
      if (strcmp(prop_func_table[n].prop, token) == 0) {
	if (prop_func_table[n].func(state, info, i) < 0)
	  return -1;
	break;
      }

    if (prop_func_table[n].prop == NULL) {  /* Property is not in the table */
      printf("%s:%d: unknown NPC property: `%s'\n",
	     state->filename, state->line, token);
      return -1;
    }
  }

  return 1;
}


/* Resolve jack references to weapons */
static int resolve_weapon_references(PARSE_FILE_STATE *state,
				     NPC_INFO *npc_info,
				     WEAPON_NPC_INFO *weapons)
{
  int i, j;
  int found;

  for (i = 0; i < weapons->n_jacks; i++) {
    found = 0;
    for (j = 0; j < weapons->n_weapons; j++)
      if (strcmp(weapons->jack[i].weapon_name,
		 weapons->weapon[j].name) == 0) {
	int jack_npc = weapons->jack[i].jack_npc;
	int weapon_num = weapons->jack[i].weapon_num;
	int weapon_npc = weapons->weapon[j].num;

	npc_info[jack_npc].weapon_npc[weapon_num] = weapon_npc;
	found = 1;
	break;
      }
    if (! found) {
      int jack_npc = weapons->jack[i].jack_npc;
      int weapon_num = weapons->jack[i].weapon_num;
      int weapon_npc = weapons->weapon[0].num;
	
      npc_info[jack_npc].weapon_npc[weapon_num] = weapon_npc;
      printf("%s: can't find weapon `%s' in \"weapon %d\" of jack `%s'\n",
	     state->filename, weapons->jack[i].weapon_name,
	     weapons->jack[i].weapon_num, weapons->jack[i].jack_name);
      if (weapons->n_weapons <= 0) {
	printf("%s: no weapons defined, can't continue\n", state->filename);
	return -1;
      }	  
    }
  }

  return 0;
}


/* Parse the NPC info file (data/npcs.dat) */
int parse_npc_info(NPC_INFO *game_npc_info, char *filename)
{
  STATE_USER_DATA user_data;
  PARSE_FILE_STATE *state;
  WEAPON_NPC_INFO *weapons;
  int num_npcs, ret;

  /* Allocate the user data */
  weapons = (WEAPON_NPC_INFO *) malloc(sizeof(WEAPON_NPC_INFO));
  if (weapons == NULL) {
    printf("Out of memory to read file `%s'\n", filename);
    return -1;
  }
  weapons->n_weapons = weapons->n_jacks = 0;
  user_data.weapons = weapons;
  user_data.mirror_start = 0;

  if ((state = open_parse_file(filename)) == NULL) {
    printf("Error reading file `%s'.\n", filename);
    free(weapons);
    return -1;
  }
  state->user_data = (void *) &user_data;

  /* Parse the file */
  num_npcs = 0;
  while ((ret = read_npc_def(state, game_npc_info + num_npcs, num_npcs)) == 1)
    num_npcs++;

  if (ret != 0) {
    close_parse_file(state);
    free(weapons);
    return -1;
  }

  if (resolve_weapon_references(state, game_npc_info, weapons)) {
    close_parse_file(state);
    free(weapons);
    return -1;
  }

  close_parse_file(state);
  free(weapons);
  return num_npcs;
}
