/* game.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 *                    Roberto A. G. Motta (TMS)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>

#define BLA

#include "common.h"
#include "game.h"
#include "rmsg.h"


/* Distance between jack and the screen edge before start scrolling: */
#define SCROLL_DIST_X1   100
#define SCROLL_DIST_Y1   90
#define SCROLL_DIST_X2   100
#define SCROLL_DIST_Y2   40

#define MESSAGE_Y    80             /* Position of the messages */

char *messages[12];                 /* Message strings */

int map_w, map_h;                   /* Map dimensions */
MAP *map, **map_line;               /* Map */
XBITMAP *map_bmp;                   /* Map bitmap */
int map_music_num;                  /* Map music number */
static int show_map = 0;            /* 1 to show the map */
static int white_color;             /* White color */

static int tremor_delta[] = { 1, 2, 3, 2, 1, 0, -1, -2, -3, -2, -1, 0 };
static int tremor_index;            /* Index of current tremor_delta */
static int tremor_left;             /* # of frames left of tremor */
static int tremor_intensity;

int game_frame;                     /* Frame counter (% GAME_FRAME_REPEAT) */
int transp_color, add_color;
int n_tiles;
int do_draw_parallax = 1;           /* 0 to draw tile[0] instead */
XBITMAP *energybar_bmp, *powerbar_bmp[3], *namebar_bmp, *quit_bmp;
XBITMAP *msg_back_bmp;
XBITMAP **tile;
XBITMAP *parallax_bmp;
BMP_FONT *font_8x8, *font_name, *font_score, *font_msg;

KEY_CONFIG key_config;              /* Key configuration */

/* For frame skipping: */
static int frames_per_draw;

/* Screen info: */
int n_retraces;
int screen_x, screen_y;
int screen_free = 0;                /* 1 if screen-follow-character is off */

#ifdef HAS_SOUND
SOUND_INFO *sound_info;             /* Pointer to sound info */
#endif
NPC_INFO *npc_info;                 /* Pointer to NPC info */
TILESET_INFO *tileset_info;         /* Tileset info */
JACK *jack;                         /* Pointer to jack viewed in the screen */
int n_jacks;                        /* Total number of jacks in the game */
JACK *jacks;                        /* All jacks */
PLAYER *player;                     /* Jack info for this player */
SERVER *server;                     /* Server info */

char image_dir[IMAGE_DIR_SIZE];     /* Image directory */

/* Game key state */
unsigned char game_key[256];
signed char game_key_trans[256];

static SERVER game_server;
static PLAYER game_player;
static int credits_running = 0;     /* 1 to run, 2 to pause credits */
static int show_fps = 0;            /* 1 to start showing fps count */
static int show_message = -1;       /* >= 0 to show message and player menu */

int (*display_msg)(const char *fmt, ...);


/* Start a tremor at (x,y) with the specified duration and intensity.
 * Note that the tremor will only occur if it's position (x,y) is near
 * the player's jack. */
void start_tremor(int x, int y, int duration, int intensity)
{
  if (ABS(jack->x - x) > 1000 || ABS(jack->y - y) > 1000)
    return;

  tremor_left = duration;
  tremor_intensity = intensity;
  tremor_index = 0;
}

/* Install the palette and change the colors of the bitmaps (fonts,
 * etc.) to sane values */
int install_palette(void)
{
  int white;

  switch (SCREEN_BPP) {
  case 1: white = calc_install_palette(); break;
  case 2: white = MAKECOLOR16(255,255,255); break;
  case 4: white = MAKECOLOR32(255,255,255); break;
  default: white = 0;
  }

  color_font(font_8x8, white + add_color);
  color_font(font_msg, white + add_color);
  set_jack_id_colors();

  if (SCREEN_BPP == 1)
    set_game_palette(palette_data);

  return white;
}

/* Change the color of a font */
void color_font(BMP_FONT *font, int color)
{
  int i;

  if (font == NULL)
    return;

  for (i = 0; i < font->n; i++)
    color_bitmap(font->bmp[i], color);
}

static void compile_tiles(void)
{
  int i;

  for (i = 0; i < n_tiles; i++)
    if (tile[i]->transparent)
      compile_bitmap(tile[i]);
}

/* Add `add_color' to all current bitmaps */
static void color_to_all_bmps(int add_color)
{
  int i;

  add_color_bmps(font_score->bmp, font_score->n, add_color);
  for (i = 0; i < n_jacks; i++)
    if (jacks[i].active) {
      add_color_bmps(jacks[i].bmp, jacks[i].n_bmp, add_color);
      add_color_bmps(jacks[i].c_mask, jacks[i].n_bmp, add_color);
      add_color_bmp(jacks[i].name_bmp, add_color);
      add_color_bmp(jacks[i].score_bmp, add_color);
    }
  for (i = 0; i < npc_number.num; i++)
    add_color_bmps(npc_info[i].bmp, npc_info[i].n_bmps, add_color);
  add_color_bmp(map_bmp, add_color);
  add_color_bmp(namebar_bmp, add_color);
  add_color_bmp(energybar_bmp, add_color);
  add_color_bmp(quit_bmp, add_color);
}

/* Change the map to `map_name', returning 1 on error */
int client_change_map(char *map_name)
{
  int i;

  /* Stop playing the current music and free the parallax bitmap */
#ifdef HAS_SOUND
  snd_start_music(-1, 0);
#endif
  if (parallax_bmp != NULL) {
    destroy_xbitmap(parallax_bmp);
    parallax_bmp = NULL;
  }

  /* Destroy and reset current bitmaps */
  color_to_all_bmps(-add_color);
  for (i = 0; i < n_tiles; i++)
    destroy_xbitmap(tile[i]);
  free(tile);
  n_tiles = 0;
  destroy_xbitmap(map_bmp);

  /* Read new map, tiles and palette */
  if (read_map_file(map_name))
    return 1;
  white_color = install_palette();

  /* Reset new bitmaps */
  color_to_all_bmps(add_color);
  add_color_bmps(tile, n_tiles, add_color);
  compile_tiles();

  /* Start playing new music */
#ifdef HAS_SOUND
  snd_start_music(map_music_num, 1);
#endif

  return 0;
}


/* Send the keyboard to the server.
 * Return 1 if the game should end (because ESC was pressed). */
static int client_send(SERVER *server, int *esc_pressed)
{
  int scan;
  int new_jack_follow;
  int increment;

  if (*esc_pressed == 2)
    return 1;

  if (player->msg_to >= 0) {
    NETMSG msg;

    msg.type = MSGT_STRING;
    msg.string.subtype = MSGSTR_MESSAGE;
    msg.string.data = player->msg_to;
    strncpy(msg.string.str, messages[player->msg_n], MAX_STRING_LEN-1);
    msg.string.str[MAX_STRING_LEN-1] = '\0';
    send_message(server->fd, &msg, 1);

    player->msg_to = -1;
  }

  if (poll_keyboard())
    return 1;
  if (game_key_trans[SCANCODE_ESCAPE] > 0) {
    if (show_message >= 0)
      show_message = -1;
    else 
      *esc_pressed = 1;
    game_key_trans[SCANCODE_ESCAPE] = 0;
  }
  client_send_key_end(server);
  net_flush(server->fd);

  /* Handle key presses */
  new_jack_follow = -1;
  for (scan = 0; scan < 127; scan++)
    if (game_key_trans[scan] > 0) {
      if (scan == key_config.screen_follow)
	screen_free = ! screen_free;
      else if (scan == key_config.show_map)
	show_map = ! show_map;
      else if (scan == key_config.show_fps)
	show_fps = ! show_fps;
      else if (scan == key_config.show_credits) {
	if (init_credits() == 0)
	  credits_running = ! credits_running;
      } else if (scan == key_config.pause_credits) {
	if (credits_running == 1)
	  credits_running = 2;
	else if (credits_running == 2)
	  credits_running = 1;
      } else if (scan == key_config.show_parallax) {
	do_draw_parallax = ! do_draw_parallax;
      }

      switch (scan) {
      case SCANCODE_Y:
	if (*esc_pressed)
	  *esc_pressed = 2;
	break;

      case SCANCODE_N:
	if (*esc_pressed != 0)
	  *esc_pressed = 0;
#if 0
	else
	  player->show_namebar = ! player->show_namebar;
	break;

      case SCANCODE_C:
	if ((game_key[SCANCODE_LEFTSHIFT] || game_key[SCANCODE_RIGHTSHIFT])
	    && init_credits() == 0)
	  credits_running = ! credits_running;
	break;

      case SCANCODE_SPACE:
	if (credits_running == 1)
	  credits_running = 2;
	else if (credits_running == 2)
	  credits_running = 1;
	break;

      case SCANCODE_TAB:
	n_retraces++;
	n_retraces %= 10;
#endif
	break;

      case SCANCODE_F1: show_message = 0; break;
      case SCANCODE_F2: show_message = 1; break;
      case SCANCODE_F3: show_message = 2; break;
      case SCANCODE_F4: show_message = 3; break;
      case SCANCODE_F5: show_message = 4; break;
      case SCANCODE_F6: show_message = 5; break;
      case SCANCODE_F7: show_message = 6; break;
      case SCANCODE_F8: show_message = 7; break;
      case SCANCODE_F9: show_message = 8; break;
      case SCANCODE_F10: show_message = 9; break;
      case SCANCODE_F11: show_message = 10; break;
      case SCANCODE_F12: show_message = 11; break;

      case SCANCODE_1: new_jack_follow = 0; break;
      case SCANCODE_2: new_jack_follow = 1; break;
      case SCANCODE_3: new_jack_follow = 2; break;
      case SCANCODE_4: new_jack_follow = 3; break;
      case SCANCODE_5: new_jack_follow = 4; break;
      case SCANCODE_6: new_jack_follow = 5; break;
      case SCANCODE_7: new_jack_follow = 6; break;
      case SCANCODE_8: new_jack_follow = 7; break;
      case SCANCODE_9: new_jack_follow = 8; break;
      case SCANCODE_0: new_jack_follow = 9; break;
      case SCANCODE_MINUS: new_jack_follow = player->id; break;
      }

      game_key_trans[scan] = 0;
    }


  if (new_jack_follow >= 0) {
    if (game_key[SCANCODE_LEFTSHIFT] || game_key[SCANCODE_RIGHTSHIFT])
      new_jack_follow += 10;

    if (new_jack_follow < n_jacks) {
      if (show_message >= 0) {
	player->msg_to = new_jack_follow;
	player->msg_n = show_message;
	show_message = -1;
      } else
	jack = jacks + new_jack_follow;
    }
  }

  if (game_key[SCANCODE_LEFTSHIFT]) increment = 10;
  else if (game_key[SCANCODE_RIGHTSHIFT]) increment = 1;
  else increment = 5;

  for (scan=0; scan < 127; scan++)
    if (game_key[scan] != 0) {
      if (scan == key_config.screen_down)
	screen_y += increment;
      else if (scan == key_config.screen_up)
	screen_y -= increment;
      else if (scan == key_config.screen_left)
	screen_x -= increment;
      else if (scan == key_config.screen_right)
	screen_x += increment;
    }

  return 0;
}

/* Change the non-transparent pixels of a bitmap to the color */
void color_bitmap(XBITMAP *bmp, int color)
{
  int i;

  if (bmp->bpp == 1) {
    unsigned char *p;

    p = bmp->line[0];
    for (i = bmp->h * bmp->w; i > 0; i--) {
      if (*p != XBMP8_TRANSP)
	*p = color;
      p++;
    }
  } else if (bmp->bpp == 2) {
    int2bpp *p;

    if (convert_16bpp_to == 15) {
      p = (int2bpp *) bmp->line[0];
      for (i = bmp->h * bmp->w; i > 0; i--) {
	if (*p != XBMP15_TRANSP)
	  *p = color;
	p++;
      }
    } else {
      p = (int2bpp *) bmp->line[0];
      for (i = bmp->h * bmp->w; i > 0; i--) {
	if (*p != XBMP16_TRANSP)
	  *p = color;
	p++;
      }
    }
  } else {
    int4bpp *p;

    p = (int4bpp *) bmp->line[0];
    for (i = bmp->h * bmp->w; i > 0; i--) {
      if (*p != XBMP32_TRANSP)
	*p = color;
      p++;
    }
  }
}

/* Add a value to the non-transparent pixels of a bitmap */
void add_color_bmp(XBITMAP *bmp, int add)
{
  int i;

  if (bmp->bpp == 1) {
    unsigned char *p;

    p = bmp->line[0];
    for (i = bmp->w * bmp->h; i > 0; i--) {
      if (*p != 0)
	*p += add;
      p++;
    }
  } else if (bmp->bpp == 2) {
    int2bpp *p;

    if (convert_16bpp_to == 15) {
      p = (int2bpp *) bmp->line[0];
      for (i = bmp->w * bmp->h; i > 0; i--) {
	if (*p != XBMP15_TRANSP)
	  *p += add;
	p++;
      }
    } else {
      p = (int2bpp *) bmp->line[0];
      for (i = bmp->w * bmp->h; i > 0; i--) {
	if (*p != XBMP16_TRANSP)
	  *p += add;
	p++;
      }
    }
  } else {
    int4bpp *p;

    p = (int4bpp *) bmp->line[0];
    for (i = bmp->w * bmp->h; i > 0; i--) {
      if (*p != XBMP32_TRANSP)
	*p += add;
      p++;
    }
  }
}

/* Add a color to a set of bitmaps */
void add_color_bmps(XBITMAP **list, int n, int add)
{
  int i;

  for (i = 0; i < n; i++)
    add_color_bmp(list[i], add);
}


static void color_jack_bitmap(XBITMAP *bmp, int color)
{
  int i;

  if (bmp->bpp == 1) {
    unsigned char *p;

    p = bmp->line[0];
    for (i = bmp->w * bmp->h; i > 0; i--) {
      if (*p == 13)
	*p = color;
      p++;
    }
  } else if (bmp->bpp == 2) {
    int2bpp *p;

    p = (int2bpp *) bmp->line[0];
    for (i = bmp->w * bmp->h; i > 0; i--) {
      if (*p == XBMP16_JACKCOLOR)
	*p = color;
      p++;
    }
  } else {
    int4bpp *p;

    p = (int4bpp *) bmp->line[0];
    for (i = bmp->w * bmp->h; i > 0; i--) {
      if (*p == XBMP32_JACKCOLOR)
	*p = color;
      p++;
    }
  }
}

static void color_jack(XBITMAP **bmp, int n, int color)
{
  for (n--; n >= 0; n--)
    color_jack_bitmap(bmp[n], color);
}

/* Move the screen (if necessary) to follow jack */
static void screen_move(void)
{
  if (! screen_free) {
    /* Move to follow jack */
    if (screen_x > jack->scr_x - SCROLL_DIST_X1)
      screen_x = jack->scr_x - SCROLL_DIST_X1;
    if (screen_y > jack->scr_y - SCROLL_DIST_Y1)
      screen_y = jack->scr_y - SCROLL_DIST_Y1;
    if (screen_x + SCREEN_W < jack->scr_x + jack->w + SCROLL_DIST_X2)
      screen_x = jack->scr_x + jack->w + SCROLL_DIST_X2 - SCREEN_W;
    if (screen_y + SCREEN_H < jack->scr_y + jack->h + SCROLL_DIST_Y2)
      screen_y = jack->scr_y + jack->h + SCROLL_DIST_Y2 - SCREEN_H;
  }

  /* Keep within the map */
  if (screen_x < 0)
    screen_x = 0;
  if (screen_y < 0)
    screen_y = 0;
  if ((screen_x + SCREEN_W) / tile[0]->w >= map_w)
    screen_x = map_w * tile[0]->w - SCREEN_W - 1;
  if ((screen_y + SCREEN_H) / tile[0]->h >= map_h)
    screen_y = map_h * tile[0]->h - SCREEN_H - 1;
}

/* Start counting the FPS rate */
void fps_init(FPS_INFO *fps)
{
  fps->cur_sec = fps->last_fps = -2;
  fps->frames = 0;
}

/* Count another frame to calculate the current FPS rate.  If it's the
 * right time, fps->last_fps is set to the rate, so you can always
 * read the last FPS rate from it. */
void fps_count(FPS_INFO *fps)
{
#ifdef GRAPH_WIN
  _ftime(&fps->cur_time);
  if (fps->cur_time.time != fps->cur_sec) {
    fps->cur_sec = fps->cur_time.time;
    if (fps->last_fps >= 0)
      fps->last_fps = fps->frames;
    else
      fps->last_fps++;
    fps->frames = 0;
  } else
    fps->frames++;
#else /* GRAPH_WIN */
  gettimeofday(&fps->cur_time, NULL);
  if (fps->cur_time.tv_sec != fps->cur_sec) {
    fps->cur_sec = fps->cur_time.tv_sec;
    if (fps->last_fps >= 0)
      fps->last_fps = fps->frames;
    else
      fps->last_fps++;
    fps->frames = 0;
  } else
    fps->frames++;
#endif /* GRAPH_WIN */
}

/* Draw the FPS rate in the screen */
void fps_draw(int x, int y, FPS_INFO *fps)
{
  if (fps->last_fps > 0)
    gr_printf(x, y, font_8x8, "%d fps", fps->last_fps);
}


/* Reset the jack */
void reset_jack(JACK *jack)
{
  jack->active = 1;
  jack->invisible = jack->invincible = 0;
  jack->frame = 0;
  jack->scr_x = jack->x = -2 * DISP_X;
  jack->scr_y = jack->y = -2 * DISP_Y;
  jack->blink = 0;
  jack->trail_len = 0;
#ifdef HAS_SOUND
  jack->sound = -1;
#endif
}

/* Read a jack and color its bmp */
int do_read_jack(int npc, JACK *j, int id)
{
  char filename[272];

  if (npc > npc_number.num) {
    printf("Internal LOSER error: invalid NPC %d\n", npc);
    return 1;
  }

  snprintf(filename, sizeof(filename), "%s.spr", npc_info[npc].file);
  strcpy(filename, img_file_name(filename));
  if (read_jack(filename, j, JACK_COLOR(id))) {
    printf("Error reading file `%s' at do_read_jack()\n", filename);
    return 1;
  }
  color_jack(j->bmp, j->n_bmp, JACK_COLOR(id));
  return 0;
}

/* Join a jack during a game */
int join_jack(int id, int npc)
{
  if (id >= n_jacks) {
    JACK *p;
    int cur_jack_id;

    if (jack != NULL)
      cur_jack_id = jack->id;
    else
      cur_jack_id = -1;

    if (jacks == NULL)           /* Shouldn't happen, but... */
      p = (JACK *) malloc(sizeof(JACK) * (id + 1));
    else
      p = (JACK *) realloc(jacks, sizeof(JACK) * (id + 1));
    if (p == NULL)
      return 1;                  /* Out of memory */
    n_jacks = id + 1;
    jacks = p;
    jacks[id].id = id;

    if (cur_jack_id >= 0)
      jack = jacks + cur_jack_id;
  } else {
    int i;

    for (i = 0; i < jacks[id].n_bmp; i++)
      destroy_xbitmap(jacks[id].bmp[i]);
    destroy_xbitmap(jacks[id].name_bmp);
    destroy_xbitmap(jacks[id].score_bmp);
    jacks[id].n_bmp = 0;
  }
  if (do_read_jack(npc, jacks + id, id))
    return 1;
  reset_jack(jacks + id);
  jacks[id].npc = npc;
  add_color_bmps(jacks[id].bmp, jacks[id].n_bmp, add_color);
  add_color_bmps(jacks[id].c_mask, jacks[id].n_bmp, add_color);
  add_color_bmp(jacks[id].name_bmp, add_color);
  add_color_bmp(jacks[id].score_bmp, add_color);
  return 0;
}


static void show_title_screen(void)
{
  XBITMAP *title_bmp;
  char filename[256];

  if (SCREEN_BPP == 1) {
    strcpy(filename, img_file_name("title.pal"));
    if (read_palette(filename))
      return;
  }
  install_palette();

  strcpy(filename, img_file_name("title.spr"));
  if ((title_bmp = read_xbitmap(filename)) == NULL
      || title_bmp->w > SCREEN_W || title_bmp->h > SCREEN_H)
    return;

  add_color_bmp(title_bmp, add_color);
  draw_bitmap(title_bmp, DISP_X + (SCREEN_W - title_bmp->w) / 2,
	      DISP_Y + (SCREEN_H - title_bmp->h) / 2);
  display_screen();
  while (! game_key[SCANCODE_ENTER]) {
#if (defined GRAPH_X11) || (defined GRAPH_SVGALIB)
    usleep(10000);
#endif
    poll_keyboard();
  }
}

static void get_image_dir(int bpp)
{
  if (bpp > 2)
    bpp = 2;     /* Read 24 and 32 bpp images from 16bpp files */
  snprintf(IMAGE_DIR, IMAGE_DIR_SIZE, IMAGE_DIR_MASK, 8*bpp);
}


static void do_query_bpp(OPTIONS *options)
{
#ifdef GRAPH_DOS
  graph_query_bpp(options->gmode, options->width, options->height,
                  options->bpp, options->page_flip);
#endif /* GRAPH_DOS */
#ifdef GRAPH_X11
  graph_query_bpp(options->width, options->height,
		  options->double_screen, options->no_scanlines,
		  ! options->no_shm);
#endif /* GRAPH_X11 */
#ifdef GRAPH_SVGALIB
  graph_query_bpp(options->gmode);
#endif /* GRAPH_SVGALIB */
}

static void do_install_graph(OPTIONS *options)
{
#ifdef HAS_SOUND
  setup_sound(options->midi_dev, options->sound_dev);
#endif /* HAS_SOUND */

#ifdef GRAPH_DOS
  install_graph(options->gmode, options->width, options->height,
                options->bpp, options->page_flip,
		(options->sound_dev) ? DIGI_AUTODETECT : DIGI_NONE,
		(options->midi_dev) ? MIDI_AUTODETECT : MIDI_NONE);
#endif /* GRAPH_DOS */
#ifdef GRAPH_X11
  install_graph(options->width, options->height,
		options->double_screen, options->no_scanlines,
		! options->no_shm);
#endif /* GRAPH_X11 */
#ifdef GRAPH_SVGALIB
  install_graph(options->gmode);
#endif /* GRAPH_SVGALIB */

  if (SCREEN_BPP == 1) {
    transp_color = XBMP8_TRANSP;
    install_game_palette(palette_data);
  } else if (SCREEN_BPP == 2)
    transp_color = (convert_16bpp_to == 15) ? XBMP15_TRANSP : XBMP16_TRANSP;
  else
    transp_color = XBMP32_TRANSP;

  install_palette();
}


/* Create the game window and read init files.  This is done only
 * once.  If this function returns 1, the program will exit. */
int do_game_init(OPTIONS *options)
{
  frames_per_draw = options->frames_per_draw;

  server = &game_server;
  player = &game_player;

  do_query_bpp(options);
  do_install_graph(options);

  get_image_dir(SCREEN_BPP);

  read_keyboard_config();
  if (read_bitmap_files())
    return 1;

  server->game_running = 0;
  player->msg_bmp = create_xbitmap(msg_back_bmp->w,msg_back_bmp->h,SCREEN_BPP);
  if (player->msg_bmp == NULL) {
    printf("Out of memory for message bitmap\n");
    return 1;
  }
  clear_xbitmap(player->msg_bmp, transp_color);

  if (read_messages("game_msg", messages)) {
    printf("Out of memory to read messages.\n");
    return 1;
  }

  return 0;
}

/* Show the startup screen */
int do_game_startup(OPTIONS *options)
{
  show_title_screen();

  if (client_wait(server, options))
    return 1;

  return 0;
}

/* Show the selected message to send and a list of jacks in the screen */
void do_show_message(int n)
{
  int i;

  gr_printf(DISP_X + 10, DISP_Y + MESSAGE_Y, font_msg, messages[n]);
  for (i = 0; i < n_jacks; i++) {
    if (! jacks[i].active)
      continue;
    gr_printf(DISP_X + 10, DISP_Y + MESSAGE_Y + 20 + 10 * i, font_8x8,
	      "%3d.", i + 1, jacks[i].name);
    draw_bitmap_tr(jacks[i].name_bmp, DISP_X + 50,
		   DISP_Y + MESSAGE_Y + 16 + 10 * i);
  }
}


static void draw_player_info(int is_enemy, XBITMAP *name, XBITMAP *score,
			     int energy, int power, int color)
{
  int x = 5;
  int yn, ye;

  if (is_enemy)
    x = SCREEN_W - namebar_bmp->w - 5;

  ye = 2;
  yn = SCREEN_H - namebar_bmp->h - 5;
  if (player->show_namebar && name != NULL) {
    draw_bitmap_tr(namebar_bmp, DISP_X + x, DISP_Y + yn);
    draw_bitmap_tr(name, DISP_X + x + 29, DISP_Y + yn + 20);
    if (score != NULL)
      draw_bitmap_tr(score, DISP_X + x + 29, DISP_Y + yn + 6);
  }
  if (is_enemy)
    x = SCREEN_W - energybar_bmp->w - 5;
  draw_energy_bar(DISP_X + x + 40, DISP_Y + ye + 9, 71, 10, energy, color);
  draw_bitmap_tr(energybar_bmp, DISP_X + x, DISP_Y + ye);
  if (power >= 1 && power <= 3 && powerbar_bmp[power-1] != NULL)
    draw_bitmap_tr(powerbar_bmp[power-1], DISP_X + x + 35, DISP_Y + ye + 28);
}


static int client_do_server_comm(int *esc_pressed)
{
#ifdef NET_PLAY
  if (client_receive(server))
    return 1;
  if (client_send(server, esc_pressed))
    return 1;
#else
  if (client_send(server, esc_pressed))
    return 1;
  client_do_server(server);
  if (client_receive(server))
    return 1;
#endif /* NET_PLAY */
  return 0;
}

/* Draw the game frame in the memory bitmap */
static void draw_game_frame(FPS_INFO *fps, int *esc_pressed)
{
  int i;

  /* Draw the screen */
  screen_move();
  draw_parallax();
  draw_background();
  draw_npcs(screen_x, screen_y, NPC_LAYER_BG);
  for (i = 0; i < n_jacks; i++) {
    if (jack != jacks + i && jacks[i].active) {
      if (! jacks[i].invisible)
	jack_draw(jacks[i].id == player->id, jacks + i, screen_x, screen_y);
      else
	jack_draw_displace(jacks + i, screen_x, screen_y);
    }
  }
  if (! jack->invisible)
      jack_draw((! jack->invincible) && jack->id == player->id,
		jack, screen_x, screen_y);
  else
      jack_draw_displace(jack, screen_x, screen_y);
  draw_npcs(screen_x, screen_y, NPC_LAYER_FG);
  draw_foreground();

  /* Draw the player (and enemy) status */
  draw_player_info(0, jacks[player->id].name_bmp, jacks[player->id].score_bmp,
		   player->energy, player->power, JACK_COLOR(player->id));
  if (player->hit_enemy >= 0) {
    player->hit_enemy--;
    if (IS_NPC_JACK(player->enemy_npc)) {
      if (player->enemy_id < n_jacks)
	draw_player_info(1, jacks[player->enemy_id].name_bmp,
			 jacks[player->enemy_id].score_bmp,
			 player->enemy_energy, -1,
			 JACK_COLOR(player->enemy_id));
      else
	draw_player_info(1, NULL, NULL,
			 player->enemy_energy, -1,
			 ENEMY_COLOR);
    } else
      draw_player_info(1, NULL, NULL, player->enemy_energy, -1, 8);
  }

  /* Draw other information:
   * map, frames per second, credits, messages */
  if (show_map) {
    draw_bitmap_tr_alpha50(map_bmp, SCREEN_W + DISP_X - map_bmp->w - 5,
			   SCREEN_H + DISP_Y - map_bmp->h - 5);
    for (i = 0; i < n_jacks; i++)
      if (jacks[i].active &&
	  (i == player->id || (jacks[i].team == jacks[player->id].team
			       && jacks[i].team >= 0)))
	draw_pixel(SCREEN_W+DISP_X - map_bmp->w-4 + jacks[i].scr_x/tile[0]->w,
		   SCREEN_H+DISP_Y - map_bmp->h-4 + jacks[i].scr_y/tile[0]->h,
		   JACK_COLOR(i) + add_color);
  }

  if (show_fps)
    fps_draw(DISP_X + 10, DISP_Y + SCREEN_H - 12, fps);

  if (show_message >= 0)
    do_show_message(show_message);

  if (credits_running && draw_credits((credits_running == 1) ? 1 : 0))
    credits_running = 0;

  /* Display the received message, if any  */
  if (player->msg_left >= 0) {
    player->msg_left--;
    if (player->msg_from < 0 || player->msg_from == player->id)
      draw_bitmap_tr_alpha50(msg_back_bmp, DISP_X + 10, DISP_Y + 10);
#if 1
    draw_bitmap_tr(player->msg_bmp, DISP_X + 10, DISP_Y + 10);
#else
    draw_bitmap_tr_alpha75(player->msg_bmp,
			   DISP_X + (SCREEN_W - player->msg_bmp->w) / 2,
			   DISP_Y + SCREEN_H - player->msg_bmp->h - 10);
#endif
  }

  if (*esc_pressed)
    draw_bitmap_tr(quit_bmp, DISP_X + (SCREEN_W - quit_bmp->w) / 2,
		   DISP_Y + (SCREEN_H - quit_bmp->h) / 2);
}

static void client_display_screen(void)
{
  if (tremor_left >= 0) {
    tremor_left--;
    tremor_index = (tremor_index + 1) % (sizeof(tremor_delta) / sizeof(int));
    display_screen_delta(tremor_intensity * tremor_delta[tremor_index]);
  } else
    display_screen();
}

/* Process a frame for the client */
static int client_do_frame(FPS_INFO *fps, int *esc_pressed)
{
  int draw_screen_count = frames_per_draw;

  do {
    if (client_do_server_comm(esc_pressed))
      return 1;
    draw_screen_count--;
  } while (draw_screen_count > 0);

  game_frame++;
  if (game_frame == GAME_FRAME_REPEAT)
    game_frame = 0;
  draw_game_frame(fps, esc_pressed);
  fps_count(fps);

  /* Finally, display the screen */
  client_display_screen();
  return 0;
}

static void game_main_loop(FPS_INFO *fps)
{
  int esc_pressed = 0;

#ifdef GRAPH_DOS
  n_retraces = 2;
#else
  n_retraces = 1;
#endif
  fps_init(fps);

  /* This is the main loop */
  while (client_do_frame(fps, &esc_pressed) == 0)
    ;
}


/* Do the game startup: fade screen in */
static void game_start(void)
{
  int i, n, esc = 0;

  for (n = 0; n < 6; n++) {
    if (client_do_server_comm(&esc))
      return;

    screen_move();
    clear_screen();
    if (n < 2) {
      draw_background_alpha(25);
      draw_foreground_alpha(25);
    } else if (n < 4) {
      draw_background_alpha(25);
      draw_foreground_alpha(25);
    } else {
      draw_background_alpha(75);
      draw_foreground_alpha(75);
    }
    display_screen();
  }

  for (n = 0; n < 6; n++) {
    if (client_do_server_comm(&esc))
      return;

    screen_move();
    draw_background();
    for (i = 0; i < n_jacks; i++)
      if (jacks[i].active) {
	if (n < 2)
	  jack_draw_alpha(jacks + i, screen_x, screen_y, 25);
	else if (n < 4)
	  jack_draw_alpha(jacks + i, screen_x, screen_y, 50);
	else
	  jack_draw_alpha(jacks + i, screen_x, screen_y, 75);
      }
    draw_foreground();
    client_display_screen();
  }
}

static int point_in_rect(int px, int py, int x, int y, int w, int h)
{
  return (px >= x && px <= x + w && py >= y && py <= y + h);
}

/* Play sound for the game.  The sound will be played only if it's
 * near the jack */
void play_game_sound(int num, int x, int y)
{
  int dist, min_dist;

  min_dist = 4 * SCREEN_W;

  if (! point_in_rect(x, y, jack->x - min_dist, jack->y - min_dist,
		      2 * min_dist, 2 * min_dist))
    return;

  dist = (x - jack->x) * (x - jack->x) + (y - jack->y) * (y - jack->y);
#ifdef HAS_SOUND
  if (dist < 2000*2000)
    snd_play_sample(num);
#endif
}

/* Start the game */
void do_game(int credits)
{
  FPS_INFO fps;
  NETMSG msg;
  int i;

  white_color = install_palette();
  if (credits && init_credits() == 0)
    credits_running = 1;

  /* Color bitmaps */
  color_to_all_bmps(add_color);
  add_color_bmps(tile, n_tiles, add_color);
  compile_tiles();

  player->msg_to = player->msg_left = -1;
  player->hit_enemy = -1;
  player->show_namebar = 0;   /* Set to 1 to start showing the name bar */

  show_map = screen_free = 0;
  screen_x = screen_y = 0;
  tremor_index = 0;
  tremor_left = -1;        /* No tremor */
  server->game_running = 1;

#ifdef HAS_SOUND
  jack->sound = -1;
  snd_start_music(map_music_num, 1);
#endif /* HAS_SOUND */

  game_start();            /* Fade the screen in */
  game_main_loop(&fps);

#ifdef HAS_SOUND
  snd_start_music(-1, 0);  /* Stop the music */
#endif /* HAS_SOUND */

  /* Disconnect from server */
  server->game_running = 0;
  msg.type = MSGT_QUIT;
  client_send_message(server->fd, &msg, 0);
  client_close(server);

  if (in_record_file != NULL) {
    fclose(in_record_file);
    in_record_file = NULL;
  }

  /* Destroy and reset current bitmaps */
  color_to_all_bmps(-add_color);
  for (i = 0; i < n_tiles; i++)
    destroy_xbitmap(tile[i]);
  free(tile);
  n_tiles = 0;
  destroy_xbitmap(map_bmp);

  /* Remove all objects from the game */
  remove_all_npcs();
  for (i = 0; i < n_jacks; i++)
    remove_jack(i);
}
