/* jack.h
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#ifndef JACK_H_FILE
#define JACK_H_FILE

/* Name bitmap dimensions */
#define NAME_BMP_W      96
#define NAME_BMP_H      font_name->bmp[0]->h

/* Score bitmap dimensions */
#define SCORE_BMP_W     42
#define SCORE_BMP_H     font_score->bmp[0]->h

/* Message bitmap dimensions */
#define MESSAGE_BMP_W   256
#define MESSAGE_BMP_H   font_msg->bmp[0]->h

#define JACK_TRAIL_LEN  6
#define JACK_TRAIL_MARK 2


/* Additional info for the player's jack */
typedef struct PLAYER {
  int id;                 /* Player ID */
  int energy;             /* Player energy */
  int f_energy_tanks;
  int n_energy_tanks;
  int power;              /* Power level */
  int score;              /* Score */
  
  int hit_enemy;          /* 1 if hit enemy */
  int enemy_npc;          /* Hit enemy NPC */
  int enemy_id;           /* Hit enemy ID */
  int enemy_energy;       /* Hit enemy energy */
  int enemy_score;        /* Hit enemy score */
  
  int show_namebar;       /* 1 to show name bar */
  int msg_from;           /* ID of the player that sent the message */
  int msg_to;             /* Jack to send message to */
  int msg_n;              /* Message number */
  XBITMAP *msg_bmp;       /* Message bitmap */
  int msg_left;           /* # of frames left */
} PLAYER;

/* A jack */
typedef struct JACK {
  char name[64];          /* Jack name */
  int id;                 /* Jack ID */
  int npc;                /* Jack NPC */
  int team;               /* Jack team */
  int active;             /* 1 if active */
  int scr_x, scr_y;       /* Position to scroll te screen to */
  int x, y;               /* Position of the upper-left corner of rect */
  int trail_x[JACK_TRAIL_LEN];        /* Trail X */
  int trail_y[JACK_TRAIL_LEN];        /* Trail Y */
  int trail_f[JACK_TRAIL_LEN];        /* Trail frame */
  int trail_len;          /* trail length */
  int frame;              /* Current frame, frame delay */
  int state_flags;        /* State flags */
  int blink;              /* 1 if blinking */
  int invisible;          /* 1 if invisible (don'w draw) */
  int invincible;         /* 1 if invincible */
  
#if defined(HAS_SOUND) || defined(GRAPH_DOS)
  int sound;              /* Scheduled sound for next frame */
  int sound_x, sound_y;   /* Scheduled sound position */
#endif /* HAS_SOUND || GRAPH_DOS */
  
  XBITMAP *name_bmp;      /* Bitmap with the name */
  XBITMAP *score_bmp;     /* Bitmap with the score */
  int w, h;               /* Width, height of bitmaps */
  int n_bmp;              /* Total of bitmaps */
  XBITMAP **bmp;          /* Walk bitmaps */
  XBITMAP **c_mask;       /* Color mask bitmaps */
} JACK;


int read_jack(char *filename, JACK *jack, int color);
void destroy_jack(JACK *jack);

void jack_draw(int own, JACK *jack, int screen_x, int screen_y);
void jack_draw_displace(JACK *jack, int screen_x, int screen_y);
void jack_draw_alpha(JACK *jack, int scr_x, int scr_y, int level);
void player_keyboard(PLAYER *player, unsigned char *scan, signed char *trans);

extern int jack_id_color[][4], jack_n_colors, enemy_color[4];

#define JACK_COLOR(id)  jack_id_color[(id) % jack_n_colors][SCREEN_BPP-1]
#define ENEMY_COLOR     enemy_color[SCREEN_BPP-1]

#endif /* JACK_H_FILE */
