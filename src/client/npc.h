/* npc.h
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#ifndef NPC_H_FILE
#define NPC_H_FILE

#include "npc_data.h"
#include "spr_data.h"

typedef struct NPC {
  int npc;                 /* NPC type */
  int id;                  /* NPC id */
  int active;              /* 1 if active */
  int x, y;                /* Position */
  int frame;               /* Frame */
  int s_frame;             /* Frame serial # (for auto-animating NPCs) */
  int dx, dy, dx2, moved;  /* For auto-moving NPCs (weapons) */
  int level;               /* Power level (for weapons) */
  int blink;               /* 1 if blinking */
} NPC;


void remove_all_npcs(void);
NPC *allocate_npc(int npc, int id);
void free_npc(int npc, int id);
NPC *find_npc_id(int npc, int id);

void unset_missiles(void);
void move_missiles(void);

int npc_is_dir_left(int frame, NPC_INFO *npc);
void set_npc_pos(NPC *npc, int npc_num, int x, int y);
void set_npc(NETMSG *);

void draw_npcs(int scr_x, int scr_y, int layer_pos);
void draw_parallax(void);


#endif /* NPC_H_FILE */
