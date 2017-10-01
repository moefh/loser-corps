/* map.h
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#ifndef MAP_H
#define MAP_H

#define MAP_MAGIC_0   0x0050414d
#define MAP_MAGIC_1   0x0150414d
#define MAP_MAGIC_2   0x0250414d

typedef struct MAP {
  char *data;
  int w, h;
  int tile_w, tile_h;
  int parms[8];
  MAP_ENEMY *enemy;
  int n_enemies;
  MAP_ITEM *item;
  int n_items;
} MAP;

int srv_read_map(SERVER *server, char *filename, MAP *map);
void srv_free_map(MAP *map);

#endif /* MAP_H */
