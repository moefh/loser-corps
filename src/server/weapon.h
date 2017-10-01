/* weapon.h */

#ifndef WEAPON_H_FILE
#define WEAPON_H_FILE

#define WEAPON_LEVELS   4    /* 3 + initial */

typedef struct WEAPON WEAPON;

struct WEAPON {
  int npc;             /* Weapon NPC */
  int id;              /* Weapon ID */
  int active;          /* 1 if active */
  int creator_npc;     /* NPC_xxx of creator */
  int creator_id;      /* NPC id of creator */
  int creator_team;    /* Jack team of creator (if creator is a jack) */
  int dir;             /* Direction */
  int x, y;            /* Position */
  int dx, dy;          /* Speed */
  int dx2;             /* Horizontal accel */
  int level;           /* Destruction level */
  int frame;
  int create;          /* 1 if created and did not send message */
  int destroy;         /* 1 to destroy */
  void (*move)(SERVER *, WEAPON *, NPC_INFO *);
};

WEAPON *srv_allocate_weapon(SERVER *server, int npc);
void srv_free_weapon(SERVER *server, WEAPON *w);
void srv_process_weapons(SERVER *server);
void srv_missile_move(SERVER *server, WEAPON *w, NPC_INFO *npc);

#endif /* WEAPON_H_FILE */
