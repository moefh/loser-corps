/* read.c
 *
 * Copyright (C) 1998,1999 Ricardo R. Massaro
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>

#include "game.h"

#define COLOR_CORRECTION_FILE   "loser.col"

/* Map colors */
#define MAP8_BORDER_COLOR   MAKECOLOR8(255,255,255)
#define MAP8_WALL_COLOR     MAKECOLOR8(127,127,127)
#define MAP8_BACK_COLOR     MAKECOLOR8(0,0,0);
#define MAP16_BORDER_COLOR  MAKECOLOR16(196,196,196)
#define MAP16_WALL_COLOR    MAKECOLOR16(128,128,128)
#define MAP16_BACK_COLOR    MAKECOLOR16(0,0,0)
#define MAP32_BORDER_COLOR  MAKECOLOR32(196,196,196)
#define MAP32_WALL_COLOR    MAKECOLOR32(128,128,128)
#define MAP32_BACK_COLOR    MAKECOLOR32(0,0,0)


static NPC_INFO game_npc_info[MAX_NPCS];
static TILESET_INFO game_tileset_info;
NPC_INFO_NUMBER npc_number;


/* Returns the file name with the correct directory for an image file
 * The returned value is a pointer to a static buffer that will be
 * overriten in future calls from this function. */
char *img_file_name(char *file)
{
  static char buf[256];
  char *p;

  snprintf(buf, sizeof(buf), "%s%s%s", DATA_DIR, IMAGE_DIR, file);
  for (p = buf; *p != '\0'; p++)
    if (*p == '|')
      *p = DIRECTORY_SEP;
  return buf;
}

/* Read the NPC information for all NPCs and sprites for non-jack
 * NPCs (jack sprites are read on demand) */
static int read_npc_files(void)
{
  int i, j;
  char filename[272];
  XBITMAP *bmp[256];

  /* Read the info */
  snprintf(filename, sizeof(filename), "%snpcs.dat", DATA_DIR);
  if ((npc_number.num = parse_npc_info(game_npc_info, filename)) <= 0)
    return 1;
  npc_info = game_npc_info;

  snprintf(filename, sizeof(filename), "%stilesets.dat", DATA_DIR);
  if (parse_tileset_info(&game_tileset_info, filename))
    return 1;
  tileset_info = &game_tileset_info;

  /* Fill the information about NPC numbers */
  npc_number.num_jacks = 0;
  npc_number.jack_start = 0;
  for (i = 0; i < npc_number.num && npc_info[i].type == NPC_TYPE_JACK; i++)
    npc_number.num_jacks++;
  npc_number.jack_end = i - 1;
  npc_number.weapon_start = i;
  for (; i < npc_number.num && npc_info[i].type == NPC_TYPE_WEAPON; i++)
    ;
  npc_number.weapon_end = i - 1;
  npc_number.enemy_start = i;
  for (; i < npc_number.num && npc_info[i].type == NPC_TYPE_ENEMY; i++)
    ;
  npc_number.enemy_end = i - 1;
  npc_number.item_start = i;
  for (; i < npc_number.num && npc_info[i].type == NPC_TYPE_ITEM; i++)
    ;
  npc_number.item_end = i - 1;


  /* Read the bitmaps */
  for (i = 0; i < npc_number.num; i++) {
    if (IS_NPC_JACK(i))
      continue;          /* Jacks are read when requested by the server */

    snprintf(filename, sizeof(filename), "%s.spr", npc_info[i].file);
    strcpy(filename, img_file_name(filename));
    if ((game_npc_info[i].n_bmps = read_xbitmaps(filename, 256, bmp)) <= 0) {
      start_error_msg("Error reading sprite file `%s' at read_npc_files().\n",
		      filename);
      return 1;
    }
    game_npc_info[i].bmp = (XBITMAP **) malloc(sizeof(XBITMAP *)
					       * game_npc_info[i].n_bmps);
    if (game_npc_info[i].bmp == NULL) {
      start_error_msg("OUT OF MEMORY for NPC bitmaps\n");
      return 1;
    }
    for (j = 0; j < game_npc_info[i].n_bmps; j++)
      game_npc_info[i].bmp[j] = bmp[j];
  }

  npc_info = game_npc_info;
  return 0;
}

/* Read miscellaneous bitmap files, like status bars, "quit"
 * confirmation, fonts, etc. */
int read_bitmap_files(void)
{
  char *error_file = NULL;
  int n;

  /* This is *required* for starting up.  Failing to read this is
   * treated separatedly */
  if ((font_8x8 = read_bmp_font(img_file_name("8x8.fon"))) == NULL) {
    printf("Error reading required file `%s': sorry, I can't go on.\n", 
	   img_file_name("8x8.fon"));
    return 1;
  }

  /* If this fails for 8bpp, we assume that the 8bpp data files were
   * not created, and give a chance to the user to do it. */
  if ((font_msg = read_bmp_font(img_file_name("msg.fon"))) == NULL
      && (SCREEN_BPP != 1 || ! create_8bpp_files()
	  || (font_msg = read_bmp_font(img_file_name("msg.fon"))) == NULL))
    error_file = "msg.fon";

  /* Read the fonts */
  if (error_file == NULL &&
      (font_name = read_bmp_font(img_file_name("name.fon"))) == NULL)
    error_file = "name.fon";
  if (error_file == NULL &&
      (font_score = read_bmp_font(img_file_name("score.fon"))) == NULL)
    error_file = "score.fon";


  /* Read the status bar bitmaps */
  if (error_file == NULL) {
    n = read_xbitmaps(img_file_name("power-bar.spr"), 3, powerbar_bmp);
    if (n < 3) {
      int i;
      
      for (i = 0; i < n; i++)
	destroy_xbitmap(powerbar_bmp[i]);
      for (i = 0; i < 3; i++)
	powerbar_bmp[i] = NULL;
    }
  }
  if (error_file == NULL &&
      (energybar_bmp = read_xbitmap(img_file_name("bar.spr"))) == NULL)
    error_file = "bar.spr";
  if (error_file == NULL &&
      (msg_back_bmp = read_xbitmap(img_file_name("msg-back.spr"))) == NULL)
    error_file = "msg-back.spr";
  if (error_file == NULL &&
      (namebar_bmp = read_xbitmap(img_file_name("bar-name.spr"))) == NULL)
    error_file = "bar-name.spr";
  if (error_file == NULL &&
      (quit_bmp = read_xbitmap(img_file_name("quit.spr"))) == NULL)
    error_file = "quit.spr";


  if (error_file != NULL) {
    start_error_msg("Error reading sprite file `%s' at read_bitmap_files()\n",
		    img_file_name(error_file));
    return 1;
  }
    
  return read_npc_files();
}

/* Read a palette from the file `filename' into the game palette
 * `palette_data[]', taking care of color correction. */
int read_palette(char *filename)
{
  FILE *f;
  int i;

  if ((f = fopen(filename, "rb")) == NULL)
    return 1;
  for (i = 0; i < 256; i++) {
    int c;

    c = fgetc(f) + color_correction[0];
    palette_data[3 * i] = (c < 0) ? 0 : (c > 255) ? 255 : c;
    c = fgetc(f) + color_correction[1];
    palette_data[3 * i + 1] = (c < 0) ? 0 : (c > 255) ? 255 : c;
    c = fgetc(f) + color_correction[2];
    palette_data[3 * i + 2] = (c < 0) ? 0 : (c > 255) ? 255 : c;
  }
  fclose(f);
  return 0;
}


/* Read the map, tiles and palette given the map name */
int read_map_file(char *map_name)
{
  int i, j;
  char filename[269];
  char tiles_filename[256], parallax_filename[256];
  char *music_name, *parallax_name;

  /* Read the map */
  snprintf(filename, sizeof(filename), "%s%s%s.map", DATA_DIR, MAPS_DIR, map_name);
  map = read_map(filename, &map_w, &map_h, NULL, NULL, tiles_filename);
  if (map == NULL) {
    start_error_msg("Can't read map from file `%s'\n", filename);
    return 1;
  }
  map_bmp = create_xbitmap(map_w + 2, map_h + 2, SCREEN_BPP);
  if (map_bmp == NULL) {
    start_error_msg("Out of memory for map bitmap.\n");
    return 1;
  }
  map_line = (MAP **) malloc(sizeof(MAP *) * map_h);
  if (map_line == NULL) {
    start_error_msg("Out of memory for map.\n");
    return 1;
  }

  if (SCREEN_BPP == 1) {
    int map_border_color = MAP8_BORDER_COLOR;
    int map_wall_color = MAP8_WALL_COLOR;
    int map_back_color = MAP8_BACK_COLOR;

    for (i = 0; i < map_w + 2; i++)
      map_bmp->line[0][i] = map_bmp->line[map_h + 1][i] = map_border_color;
    for (i = 0; i < map_h + 2; i++)
      map_bmp->line[i][0] = map_bmp->line[i][map_w + 1] = map_border_color;
    for (i = 0; i < map_h; i++) {
      map_line[i] = map + i * map_w;
      for (j = 0; j < map_w; j++)
	map_bmp->line[i+1][j+1] = ((IS_MAP_BLOCK(map_line[i][j].block)
				    || map_line[i][j].block == MAP_SECRET)
				   ? map_wall_color : map_back_color);
    }
  } else if (SCREEN_BPP == 2) {    /* 2 bpp */
    int2bpp **line;
    int map_border_color = MAP16_BORDER_COLOR;
    int map_wall_color = MAP16_WALL_COLOR;
    int map_back_color = MAP16_BACK_COLOR;

    line = (unsigned short **) map_bmp->line;
    for (i = 0; i < map_w + 2; i++)
      line[0][i] = line[map_h + 1][i] = map_border_color;
    for (i = 0; i < map_h + 2; i++)
      line[i][0] = line[i][map_w + 1] = map_border_color;
    for (i = 0; i < map_h; i++) {
      map_line[i] = map + i * map_w;
      for (j = 0; j < map_w; j++)
	line[i+1][j+1] = ((IS_MAP_BLOCK(map_line[i][j].block)
			   || map_line[i][j].block == MAP_SECRET)
			  ? map_wall_color : map_back_color);
    }
  } else {     /* assume 4 bpp */
    int4bpp **line;
    int map_border_color = MAP32_BORDER_COLOR;
    int map_wall_color = MAP32_WALL_COLOR;
    int map_back_color = MAP32_BACK_COLOR;

    line = (int4bpp **) map_bmp->line;
    for (i = 0; i < map_w + 2; i++)
      line[0][i] = line[map_h + 1][i] = map_border_color;
    for (i = 0; i < map_h + 2; i++)
      line[i][0] = line[i][map_w + 1] = map_border_color;
    for (i = 0; i < map_h; i++) {
      map_line[i] = map + i * map_w;
      for (j = 0; j < map_w; j++)
	line[i+1][j+1] = ((IS_MAP_BLOCK(map_line[i][j].block)
			   || map_line[i][j].block == MAP_SECRET)
			  ? map_wall_color : map_back_color);
    }
  }

  /* Get default values for tiles, music and parallax file */
  if (tiles_filename[0] == '\0')
    strcpy(tiles_filename, map_name);
  parallax_filename[0] = '\0';
#ifdef HAS_SOUND
  map_music_num = get_music_id("fase1");
#endif

  /* Now try to get these from the map */
  music_name = strchr(tiles_filename, ':');  /* Look for the music name */
  if (music_name != NULL) {
    parallax_name = strchr(music_name + 1, ':');
    if (parallax_name != NULL)
      *parallax_name++ = '\0';
    *music_name++ = '\0';
#ifdef HAS_SOUND
    if (*music_name != '\0')
      map_music_num = get_music_id(music_name);
#endif
    if (parallax_name != NULL)
      strcpy(parallax_filename, parallax_name);
  }

  /* Now actually read the files */
  snprintf(filename, sizeof(filename), "tiles/%s.spr", tiles_filename);
  strcpy(filename, img_file_name(filename));
  tile = (XBITMAP **) malloc(sizeof(XBITMAP *) * MAX_TILES);
  if (tile == NULL) {
    start_error_msg("Out of memory for tiles.\n");
    return 1;
  }
  if ((n_tiles = read_xbitmaps(filename, MAX_TILES, tile)) <= 0) {
    start_error_msg("Can't read tiles from file `%s'.\n", filename);
    return 1;
  }
  if (parallax_filename[0] != '\0') {
    snprintf(filename, sizeof(filename), "parallax/%s.spr", parallax_filename);
    strcpy(filename, img_file_name(filename));
    parallax_bmp = read_xbitmap(filename);
    if (parallax_bmp == NULL) {
      start_error_msg("Can't read parallax from file `%s'.\n", filename);
      return 1;
    }
  }

  if (SCREEN_BPP == 1) {
    /* Read the palette */
    snprintf(filename, sizeof(filename), "tiles/%s.pal", tiles_filename);
    strcpy(filename, img_file_name(filename));
    if (read_palette(filename)) {
      start_error_msg("Can't read palette from file `%s'.\n", filename);
      return 1;
    }
  }

  return 0;
}


/* Read the keyboard configuration */
void read_keyboard_config(void)
{
  int done = 0;
  char filename[256], *p;

  p = getenv("HOME");
  if (p != NULL) {
    snprintf(filename, sizeof(filename), "%s/.loser", p);
    if (parse_key_info(&key_config, filename) == 0)
      done = 1;
  }
  if (! done) {
    snprintf(filename, sizeof(filename), "%s%s", DATA_DIR, "keyboard.dat");
    parse_key_info(&key_config, filename);
  }
}



/**** Setup file ****/

enum {
  SETUP_STRING,
  SETUP_INTEGER,
  SETUP_END,
};

typedef struct SETUP {
  int type;
  char *name;
  void *value;
} SETUP;

static void set_option(SETUP *setup, char *name, char *value)
{
  int i;

  for (i = 0; setup[i].type != SETUP_END; i++)
    if (strcmp(setup[i].name, name) == 0) {
      switch (setup[i].type) {
      case SETUP_INTEGER:
	*((int *) (setup[i].value)) = atoi(value);
	break;

      case SETUP_STRING:
	strcpy((char *) (setup[i].value), value);
	break;
      }
      break;
    }
}

/* Read setup from the file `file' */
static int read_setup(char *file, SETUP *setup)
{
  FILE *f = NULL;
  int line_no, i;
  char filename[1024], *home;
  char line[1024], *str;
  char option[256], value[256];

  home = getenv("HOME");
  if (home != NULL) {
    snprintf(filename, sizeof(filename), "%s%c.%s", home, DIRECTORY_SEP, COLOR_CORRECTION_FILE);
    f = fopen(filename, "r");
  }

  if (f == NULL) {
    snprintf(filename, sizeof(filename), "%s%s", DATA_DIR, COLOR_CORRECTION_FILE);
    f = fopen(filename, "r");
  }

  if (f == NULL)
    return 1;

  line_no = 0;
  while (fgets(line, 1024, f)) {
    line_no++;
    for (str = line + strlen(line) - 1; str >= line && isspace(*str); str--)
      ;
    if (str < line)
      continue;       /* Empty line */
    *(str + 1) = '\0';

    for (str = line; isspace(*str); str++)
      ;
    if (*str == '#')
      continue;       /* Comment */

    i = 0;
    while (*str != '=' && *str != '\0' && ! isspace(*str))
      option[i++] = *str++;
    option[i] = '\0';

    while (isspace(*str))
      str++;
    if (*str != '=')
      continue;
    str++;
    while (isspace(*str))
      str++;
    i = 0;
    while (*str != '\0' && ! isspace(*str))
      value[i++] = *str++;
    value[i] = '\0';

    set_option(setup, option, value);
  }
  fclose(f);
  return 0;
}

/* Reads the color correction values from the default file
 * (`loser.col'), writting the RGB correction values in `delta[0]',
 * delta[1], and `delta[2]'. */
void get_color_correction(int *delta)
{
  SETUP color_correction_setup[] = {
    { SETUP_INTEGER, "r", NULL },
    { SETUP_INTEGER, "g", NULL },
    { SETUP_INTEGER, "b", NULL },
    { SETUP_END }
  };

  color_correction_setup[0].value = (void *) (delta);
  color_correction_setup[1].value = (void *) (delta + 1);
  color_correction_setup[2].value = (void *) (delta + 2);

  delta[0] = delta[1] = delta[2] = 0;

  read_setup(COLOR_CORRECTION_FILE, color_correction_setup);
}
