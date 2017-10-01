/* npc_data.h */

#ifndef NPC_DATA_H
#define NPC_DATA_H


#ifndef BITMAP_H_FILE
#define XBITMAP   void *
#endif


typedef struct NPC_INFO_NUMBER {
  int num;                       /* # of NPC infos */
  int jack_start, jack_end;      /* First and last jacks */
  int num_jacks;                 /* # of NPCs of jacks */
  int weapon_start, weapon_end;  /* First and last weapons */
  int enemy_start, enemy_end;    /* First and last enemies */
  int item_start, item_end;      /* First and last items */
} NPC_INFO_NUMBER;

enum {     /* NPC drawing modes */
  NPC_DRAW_NORMAL,
  NPC_DRAW_NONE,
  NPC_DRAW_ALPHA25,
  NPC_DRAW_ALPHA50,
  NPC_DRAW_ALPHA75,
  NPC_DRAW_LIGHTEN,
  NPC_DRAW_DARKEN,
  NPC_DRAW_DISPLACE,
  NPC_DRAW_PARALLAX,
};

enum {     /* Item float modes */
  ITEM_FLOAT_FAST,
  ITEM_FLOAT_SLOW,
  ITEM_FLOAT_NONE,
};

enum {     /* NPC layer position */
  NPC_LAYER_FG,
  NPC_LAYER_BG,
};

/* Info for the NPCs */
typedef struct NPC_INFO {
  /* Info stuff (from `.ci' file) */
  char npc_name[32];                /* NPC name */
  char file[256];                   /* Sprite file name w/o dir or `.spr' */
  int type;                         /* One of NPC_TYPE_* */
  int clip_x, clip_y;               /* Clip rect pos */
  int clip_w, clip_h;               /* Clip rect size */
  
  int s_stand, n_stand;
  int s_jump, n_jump;
  int s_walk, n_walk;
  int shoot_frame;
  
  unsigned char secret[64];       /* Secret string (for secret jacks) */
  int speed, accel;               /* Speed and accel for weapons */
  int draw_mode;                  /* NPC_DRAW_* */
  int float_mode;                 /* ITEM_FLOAT_* */
  int layer_pos;                  /* NPC_LAYER_* */
  int weapon_y[MAX_WEAPONS];      /* Y offset of weapon creation */
  int weapon_npc[MAX_WEAPONS];    /* Weapon NPC */
  int show_hit;                   /* 1 to send hit info to client when hit */
  int block;                      /* 1 to block jack movement */
  
  char lib_name[32];              /* Library name (for items and enemies) */
  
  /* Images (from `.spr' file) */
  int n_bmps;
  int frame[256];
  XBITMAP **bmp;
} NPC_INFO;

int parse_npc_info(NPC_INFO *npc_info, char *info_filename);

#endif
