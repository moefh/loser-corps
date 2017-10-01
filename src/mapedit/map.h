/* map.h */

#ifndef MAP_H
#define MAP_H

#include "common.h"

#define ITEM_TEXT_LEN  256

/* Layer bits */
#define ML_FORE   0x01
#define ML_BACK   0x02
#define ML_BLOCK  0x04


/* Default parameters for new map (old values -- for fast 8 bpp) */
#define DEFAULT_MAX_Y_SPEED   14000   /* 10000 */
#define DEFAULT_JUMP_HOLD      1100   /*   500 */
#define DEFAULT_GRAVITY        1600   /*   800 */
#define DEFAULT_MAX_X_SPEED    8000   /*  6000 */
#define DEFAULT_WALK_ACCEL     1600   /*  1000 */
#define DEFAULT_JUMP_ACCEL      400   /*   200 */
#define DEFAULT_ATTRICT         800   /*   500 */
#define DEFAULT_FRAME_SKIP        0   /*     0 */

typedef struct NPC_NAME NPC_NAME;

struct NPC_NAME {
  int npc;               /* NPC number */
  char name[256];        /* NPC definition name */
  int child;             /* 1 if is a child */
};


typedef struct MAP_DATA {
  short int back;            /* Background */
  short int fore;            /* Foreground */
  short int block;           /* Block */
  short int sel;             /* 1 if selected */
} MAP_DATA;

typedef struct MAP_OBJECT MAP_OBJECT;

struct MAP_OBJECT {
  int npc;                   /* Object NPC */
  int x, y;                  /* Position in the map */
  int dir;                   /* Direction */
  int respawn;               /* Respawn time, in seconds */
  int level;                 /* Level, if enemy */
  int duration;              /* Duration, if item */
  int target;                /* Object target */
  int vulnerability;         /* Vulnerability (%) */
  char text[ITEM_TEXT_LEN];  /* Item text */
  MAP_OBJECT *temp;          /* Reserved, used for target handling */
  MAP_OBJECT *next;
};

typedef struct MAP {
  char author[256];      /* Author */
  char comment[256];     /* Comments */
  char tiles[256];       /* Tiles */
  
  int maxyspeed;         /* Max vertical speed */
  int jumphold;          /* Jump hold */
  int gravity;           /* Gravity */
  int maxxspeed;         /* Max horizontal speeed */
  int accel;             /* Walk accel */
  int jumpaccel;         /* Jump accel */
  int attrict;           /* Friction */
  int frameskip;         /* Frame skip */
  int tile_w, tile_h;    /* Tile dimensions */
  
  MAP_OBJECT *obj;       /* Object list */
  
  int w, h;              /* Dimensions */
  MAP_DATA *data;        /* Map tiles */
  MAP_DATA *line[0];     /* Pointer to map lines */
} MAP;

#define MAP_MAGIC_0   0x0050414d
#define MAP_MAGIC_1   0x0150414d
#define MAP_MAGIC_2   0x0250414d

extern NPC_NAME npc_names[];

MAP *create_map(int w, int h);
void destroy_map(MAP *map);
MAP *read_map(char *filename);
int write_map(char *filename, MAP *map);

void fill_default_obj_values(MAP_OBJECT *obj);

int get_npc_index(int npc);
char *get_npc_name(int npc);
int get_npc_number(char *name);

#endif /* MAP_H */
