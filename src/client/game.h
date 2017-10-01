/* game.h
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#ifndef GAME_H
#define GAME_H

#include "bitmap.h"
#include "pal8bpp.h"
#include "screen.h"
#include "draw.h"
#include "map.h"
#include "jack.h"
#include "npc.h"
#include "options.h"
#include "key_data.h"

#ifndef ABS
#define ABS(x)  (((x) < 0) ? -(x) : (x))
#endif

enum {
  CONNECT_LOCAL,
  CONNECT_NETWORK,
  CONNECT_PLAYBACK,
};

#include "client.h"
#include "common.h"

#include "network.h"

#ifdef HAS_SOUND
#include "sound.h"
#endif /* HAS_SOUND */

#define err_log   printf

#ifndef INLINE
#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif /* __GNUC__ */
#endif /* INLINE */


#define IMAGE_DIR image_dir

#define MAX_TILES          256
#define GAME_FRAME_REPEAT  420

#if 0
extern char *npc_file[];
#endif

#define IS_NPC(npc) (((npc)>=0)&&((npc)<npc_number.num))

#define IS_NPC_JACK(npc)   (IS_NPC(npc)&&(npc_info[(npc)].type==NPC_TYPE_JACK))
#define IS_NPC_WEAPON(npc) (IS_NPC(npc)&&(npc_info[(npc)].type==NPC_TYPE_WEAPON))
#define IS_NPC_ENEMY(npc)  (IS_NPC(npc)&&(npc_info[(npc)].type==NPC_TYPE_ENEMY))
#define IS_NPC_ITEM(npc)   (IS_NPC(npc)&&(npc_info[(npc)].type==NPC_TYPE_ITEM))


extern unsigned char game_key[];
extern signed char game_key_trans[];

extern int map_w, map_h;            /* Map dimensions */
extern MAP *map, **map_line;        /* Game map */
extern XBITMAP *map_bmp;            /* Map bitmap */
extern int map_music_num;           /* Map music number */

extern int add_color;                 /* Pixel value to add to sprites */
extern int transp_color;              /* Transparent color */
extern int n_tiles;                   /* Number of tiles */
extern XBITMAP *quit_bmp;             /* "Quit?" message */
extern XBITMAP *powerbar_bmp[3];      /* Power bar bitmaps */
extern XBITMAP *energybar_bmp;        /* Energy bar */
extern XBITMAP *namebar_bmp;          /* Name bar */
extern XBITMAP *msg_back_bmp;         /* Message Background */
extern BMP_FONT *font_8x8;            /* 8x8 font */
extern BMP_FONT *font_name;           /* Name font */
extern BMP_FONT *font_score;          /* Score font */
extern BMP_FONT *font_msg;            /* Message font */
extern XBITMAP **tile;                /* Tiles */
extern XBITMAP *parallax_bmp;         /* Parallax bitmap */
extern int do_draw_parallax;          /* 1 to draw parallax effect */
extern int n_retraces;                /* Number of retraces per frame */

extern int screen_x, screen_y;        /* Top left of screen */
extern int game_frame;                /* Game frame # (% GAME_FRAME_REPEAT) */
#ifdef HAS_SOUND
extern SOUND_INFO *sound_info;        /* Info about sound files */
#endif
extern NPC_INFO_NUMBER npc_number;    /* Info about the number of NPCs */
extern NPC_INFO *npc_info;            /* NPC info */
extern TILESET_INFO *tileset_info;    /* Tileset info */
extern PLAYER *player;                /* Player */
extern JACK *jack;                    /* Pointer jack viewed in screen */
extern int n_jacks;                   /* Number of jacks in the game */
extern JACK *jacks;                   /* List of all jacks */
extern SERVER *server;                /* Server */

extern char image_dir[];              /* Directory where to find the images */
extern int program_argc;
extern char **program_argv;

extern KEY_CONFIG key_config;

extern int (*display_msg)(const char *fmt, ...);
void start_error_msg(char *fmt, ...);
int create_8bpp_files(void);
int game_kb_has_key(void);
int game_kb_get_key(void);

void write_rec_header(FILE *f);
int read_rec_header(int fd);

void start_tremor(int x, int y, int duration, int intensity);

void play_game_sound(int num, int x, int y);
char *img_file_name(char *file);
int read_palette(char *filename);
void get_color_correction(int *delta);

int join_jack(int id, int npc);
int do_game_init(OPTIONS *options);
int do_game_startup(OPTIONS *options);
void do_game(int credits);
int client_change_map(char *map_name);
int read_map_file(char *map_name);
int read_bitmap_files(void);
void read_keyboard_config(void);

int init_credits(void);
int draw_credits(int advance);

void reset_jack(JACK *j);
int do_read_jack(int npc, JACK *j, int id);
void copy_xbitmap(XBITMAP *dest, int x, int y, XBITMAP *src);
void draw_text(XBITMAP *dest, BMP_FONT *font, int x, int y, char *txt);

int install_palette(void);

void color_bitmap(XBITMAP *bmp, int color);
void add_color_bmp(XBITMAP *bmp, int add);
void add_color_bmps(XBITMAP **bmp, int n, int add);
void color_font(BMP_FONT *f, int color);
void set_jack_id_colors(void);

int read_command_line(char *command, int max);

#endif /* GAME_H */
