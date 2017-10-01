/* enemy.h
 *
 * Copyright (C) 1998 Roberto A. G. Motta
 *                    Ricardo R. Massaro
 */

#ifndef ENEMY_H_FILE
#define ENEMY_H_FILE

#define ENEMY_WEAPON_DELAY   10

#define ENEMY_JUMP_SPEED ((7 * MAX_JUMP_SPEED) / 4)

/* #define ENEMY_WALK_SPEED MAX_WALK_SPEED */
#define ENEMY_WALK_SPEED ((9*MAX_WALK_SPEED)/10)   /* By Moe */

#if 0        /* By Moe */
#define ENEMY_X_VISION 3 * 64
#define ENEMY_Y_VISION 3 * 64
#else
#define ENEMY_X_VISION (5 * (server)->tile_w)
#define ENEMY_Y_VISION (3 * (server)->tile_h)
#endif

typedef struct ENEMY ENEMY;

struct ENEMY {
  int target_id;       /* Player ID */
  
  int active;          /* 1 if active */
  int npc;             /* Enemy NPC */
  int id;              /* Enemy ID */
  int map_id;          /* ID of the enemy in the map */
  int dir;
  int x, y;
  int dx, dy;
  int ax;
  int frame;
  int dead;            /* 1 if dead */
  int energy;          /* Energy */
  int create;          /* 1 if created and did not send message */
  int destroy;         /* 1 to destroy */
  int state;           /* STATE_xxx (walking, standing, etc). */
  int client_frame;    /* Frame to send to the client */
  int weapon;          /* > 0 if weapon was shot */
  int weapon_delay;
  int state_data;
  
  int vulnerability;   /* Vulnerability (%) */
  int level;           /* Enemy level */
  
  void (*move)(SERVER *, ENEMY *, NPC_INFO *);
};


ENEMY *srv_allocate_enemy(SERVER *, int npc);
void srv_free_enemy(SERVER *, ENEMY *);

void srv_hit_enemy(SERVER *server, ENEMY *e, int damage, int damage_type);

void enemy_fix_frame(ENEMY *, NPC_INFO *);
void create_map_enemies(SERVER *server);
ENEMY *create_enemy(SERVER *server, int npc, int x, int y, int dir);
void create_map_enemy(SERVER *server, MAP_ENEMY *me, int map_id);

int calc_enemy_movement(SERVER *server, ENEMY *e, int calc_all,
			int x, int y, int w, int h,
			int dx, int dy, int *ret_dx, int *ret_dy);

#endif /* WEAPON_H_FILE */
