/* server.h
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#ifndef SERVER_H_FILE
#define SERVER_H_FILE


#ifdef _WINDOWS

#include <sys/types.h>
#include <sys/timeb.h>

#else /* _WINDOWS */

#include <unistd.h>

#ifndef __DJGPP__
#include <sys/signal.h>
#include <sys/time.h>
#endif /* __DJGPP__ */

#endif /* _WINDOWS */


#ifndef MAX
#define MAX(a,b)     (((a) > (b)) ? (a) : (b))
#endif /* MAX */

#ifndef MIN
#define MIN(a,b)     (((a) < (b)) ? (a) : (b))
#endif /* MIN */


typedef struct SERVER SERVER;
typedef struct CLIENT CLIENT;
typedef struct MAP_INFO MAP_INFO;
typedef struct MAP_ITEM MAP_ITEM;
typedef struct MAP_ENEMY MAP_ENEMY;
typedef struct START_POS START_POS;
typedef struct NPC_NUMBER NPC_NUMBER;
typedef struct TREMOR_INFO TREMOR_INFO;

#include "common.h"
#include "map.h"
#include "npc_data.h"
#include "jack.h"
#include "weapon.h"
#include "enemy.h"
#include "item.h"
#include "event.h"
#include "sound.h"


#define IS_NPC(server, _npc) (((_npc)>=0) && ((_npc)<(server)->npc_num.num))

#define IS_NPC_JACK(server, _npc) \
           (IS_NPC(server,_npc)&&((server)->npc_info[(_npc)].type==NPC_TYPE_JACK))
#define IS_NPC_WEAPON(server, _npc) \
           (IS_NPC(server,_npc)&&((server)->npc_info[(_npc)].type==NPC_TYPE_WEAPON))
#define IS_NPC_ENEMY(server, _npc) \
           (IS_NPC(server,_npc)&&((server)->npc_info[(_npc)].type==NPC_TYPE_ENEMY))
#define IS_NPC_ITEM(server, _npc) \
           (IS_NPC(server,_npc)&&((server)->npc_info[(_npc)].type==NPC_TYPE_ITEM))


/* Movement parameters */
#define START_JUMP_SPEED      MAX_JUMP_SPEED
#define START_WALK_SPEED      (DEC_WALK_SPEED+1)

#define MAX_JUMP_SPEED        server->parms[0]
#define INC_JUMP_SPEED        server->parms[1]
#define DEC_JUMP_SPEED        server->parms[2]

#define MAX_WALK_SPEED        server->parms[3]
#define INC_WALK_SPEED        server->parms[4]
#define INC_WALK_JUMP_SPEED   server->parms[5]
#define DEC_WALK_SPEED        server->parms[6]

#define FRAME_DELAY           server->parms[7]


enum {       /* Client states */
  CLST_ALIVE,        /* Client's jack lives */
  CLST_DEAD,         /* Client's jack died, but connection is still up */
  CLST_DISCONNECT    /* Client disconnected */
};


struct CLIENT {
  int type;                /* CLIENT_xxx */
  int state;               /* CLST_xxx */
  char host[256];          /* Client machine */
  int port;                /* Client port (very unlikely to use, but...) */
  int fd;                  /* Connection file descriptor */
  JACK jack;               /* Client's jack */
  CLIENT *next;
};

struct NPC_NUMBER {
  int num;                 /* Number of types of NPCs */
  int jack_start;
  int jack_end;
  int weapon_start;
  int weapon_end;
  int enemy_start;
  int enemy_end;
  int item_start;
  int item_end;
};


struct MAP_ENEMY {
  int npc, id;             /* NPC and id of the enemy (when active) */
  int x, y;                /* Position where the enemy will be created */
  int dir;                 /* Direction */
  int intelligence;        /* Enemy intelligence (level) */
  int vulnerability;       /* Enemy vulnerability (%) */
  int respawn_left;        /* Time left before respawn (-1 if active) */
  int respawn_time;        /* Respawn time */
};

struct MAP_ITEM {
  int x, y;                /* Position where the item will be created */
  int npc, id;             /* NPC and ID of the item (if active) */
  int respawn_left;        /* Time left before respawn (-1 if active) */
  int respawn_time;        /* Respawn time */
  int duration;            /* Duration time */
  int level;               /* Level */
  int target_obj_num;      /* Target object number (only used in creation) */
  int target_map_id;       /* Target map ID */
  int target_npc;          /* Target NPC */
  int target_id;           /* Target NPC ID */
  int target_x, target_y;  /* Target position (if it's a teleporter) */
  char text[ITEM_TEXT_LEN]; /* Item text (if item is of type `text') */
};

struct TREMOR_INFO {
  int x, y;           /* Tremor location */
  int duration;       /* Tremor duration */
  int intensity;      /* Tremor intensity */
};

#define MAX_START_POS   16     /* Maximum number of starting positions */

struct START_POS {
  int dir;                 /* Direction */
  int x, y;                /* Position */
};

struct MAP_INFO {
  char *map;                 /* Game map */
  int map_w, map_h;          /* Map dimensions */
  int tile_w, tile_h;        /* Map tile dimensions */
  int n_start_pos;           /* Number of start positions */
  int next_start_pos;        /* Next start position to use */
  START_POS start_pos[MAX_START_POS];  /* Start positions */
  int n_map_enemies;         /* # of map enemies */
  MAP_ENEMY *map_enemy;      /* Map enemies */
  int n_map_items;           /* # of map items */
  MAP_ITEM *map_item;        /* Map items */
  int parms[8];              /* Movement parameters */
};

struct SERVER {
  int port;                  /* Port number to listen */
  int sock;                  /* Connection socket */
  int game_running;          /* 1 if there's a game running */
  int quit, reset;           /* quit/reset flags */
  int min_frame_time;        /* Minimum frame time, in milisseconds */
  int max_idle_time;         /* Maximum idle time, in seconds */
  
  /* Map info: */
  int change_map;            /* 1 to change the map */
  char map_file[64];         /* Map base filename (without `.map') */
  char *map;                 /* Game map */
  int map_w, map_h;          /* Map dimensions */
  int tile_w, tile_h;        /* Map tile dimensions */
  int n_start_pos;           /* Number of start positions */
  int next_start_pos;        /* Next start position to use */
  START_POS start_pos[MAX_START_POS];  /* Start positions */
  int n_map_enemies;         /* # of map enemies */
  MAP_ENEMY *map_enemy;      /* Map enemies */
  int n_map_items;           /* # of map items */
  MAP_ITEM *map_item;        /* Map items */
  int parms[8];              /* Movement parameters */
  MAP_INFO new_map_info;     /* Map info to change to */
  
  int n_events;              /* # of allocated events */
  EVENT *event;              /* Allocated events */
  int n_sounds;              /* # of allocated sounds */
  SOUND *sound;              /* Allocated sounds */
  TREMOR_INFO tremor;        /* Tremor info */
  int do_tremor;             /* 1 to send `tremor' msg to clients */
  
  int n_weapons;             /* # of allocated weapons */
  WEAPON *weapon;            /* Allocated weapons */
  int n_enemies;             /* # of allocated enemies */
  ENEMY *enemy;              /* Allocated enemies */
  int n_items;               /* # of allocated items */
  ITEM *item;                /* Allocated items */
  
  int n_npcs;                /* # of NPC infos */
  NPC_INFO *npc_info;        /* NPC info */
  NPC_NUMBER npc_num;        /* NPC numbering info */
  SOUND_INFO sound_info;     /* Sound info */
  
  int n_disconnected;        /* # of disconnected clients */
  int disconnected_id[16];   /* disconnected clients */
  int disconnected_npc[16];  /* disconnected clients */
  
  CLIENT *joined;            /* List of joined clients */
  CLIENT *client;            /* List of playing clients */
};


void srv_reset_server(SERVER *server);
CLIENT *srv_add_client(CLIENT **list, char *host, int port, int fd);
void srv_remove_client(CLIENT **list, int fd, int elect_new_owner);
CLIENT *srv_find_client(CLIENT *list, int fd);
CLIENT *srv_find_client_id(CLIENT *list, int id);
CLIENT *srv_find_client_name(CLIENT *list, char *name);
void srv_free_client_list(CLIENT **client);

int srv_find_free_id(CLIENT *list);
int srv_find_free_team(CLIENT *l1, CLIENT *l2);
void srv_reset_jack(JACK *jack, int id);
void srv_get_jack_start_pos(SERVER *, JACK *);

int srv_sec2frame(int sec);

int server_read_map_file(SERVER *server, MAP_INFO *info, char *map_name);

#endif /* SERVER_H_FILE */
