/* item.h
 *
 * Copyright (C) 1998  Ricardo R. Massaro
 */

#ifndef ITEM_H_FILE
#define ITEM_H_FILE

#define ITEM_TEXT_LEN   256    /* Max. item text length accepted */

typedef struct ITEM ITEM;

struct ITEM {
  int active;              /* 1 if active */
  int id;                  /* Item ID */
  int npc;                 /* Item NPC */
  int x, y;                /* Position */
  int map_id;              /* ID of item in the map */
  int create;              /* 1 if created and did not send message */
  int destroy;             /* 1 to destroy */
  int client_frame;        /* Frame to send to the client */
  int changed;             /* 1 if changed state */
  int state;               /* State (for doors, levers...) */
  int state_data;          /* Additional state data */
  
  int duration;            /* Duration, if invincibility or invisibility */
  int level;               /* Level */
  int target_x, target_y;  /* Target position */
  char text[ITEM_TEXT_LEN];    /* Item text */
  int pressed;             /* Valid only when calling get_item(): 1 if player
			    * pressed a key */
  
  void (*get_item)(SERVER *server, ITEM *it, CLIENT *c);
};

ITEM *srv_allocate_item(SERVER *);
void srv_free_item(SERVER *, ITEM *);
void item_fix_frame(ENEMY *, NPC_INFO *);
void create_map_items(SERVER *server);

int get_item_npc(SERVER *server, char *name);

void create_item(SERVER *server, MAP_ITEM *map_item, int map_id);
ITEM *make_new_item(SERVER *server, int npc, int x, int y);

#endif /* ITEM_H_FILE */
