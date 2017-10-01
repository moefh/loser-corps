/* map.h
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#ifndef MAP_H
#define MAP_H

typedef struct MAP {
  short int back;         /* Background tile */
  short int fore;         /* Foreground tile */
  short int block;        /* Block tile */
} MAP;

#define MAP_MAGIC_0   0x0050414d
#define MAP_MAGIC_1   0x0150414d
#define MAP_MAGIC_2   0x0250414d

MAP *read_map(char *filename, int *w, int *h,
	      char *author, char *comments, char *tiles);

#endif /* MAP_H */
