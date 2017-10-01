/* map.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#include <stdio.h>
#include <malloc.h>

#include "map.h"


/* Read a 4 byte integer */
static int read_int(FILE *f)
{
  int c1, c2, c3, c4;

  c1 = fgetc(f) & 0xff;
  c2 = fgetc(f) & 0xff;
  c3 = fgetc(f) & 0xff;
  c4 = fgetc(f) & 0xff;
  return (c1 | (c2 << 8) | (c3 << 16) | (c4 << 24));
}

/* Read a 2 byte integer */
static short int read_short(FILE *f)
{
  int c1, c2;

  c1 = fgetc(f) & 0xff;
  c2 = fgetc(f) & 0xff;
  return (c1 | (c2 << 8));
}


static void read_string(char *str, FILE *f)
{
  int n, i;

  n = fgetc(f);
  if (str != NULL) {
    for (i = 0; i < n; i++)
      str[i] = fgetc(f);
    str[i] = '\0';
  } else
    for (i = 0; i < n; i++)
      fgetc(f);
}

static void skip_object(FILE *f)
{
  int i;
  short size;

  size = read_short(f);
  for (i = 0; i < size; i++)
    read_short(f);
}

/* Read a map from a file */
MAP *read_map(char *filename, int *w, int *h,
	      char *author, char *comments, char *tiles)
{
  FILE *f;
  MAP *map;
  int map_w, map_h, x, y, magic, version;

  if ((f = fopen(filename, "rb")) == NULL)
    return NULL;
  magic = read_int(f);
  switch (magic) {
  case MAP_MAGIC_0: version = 0; break;
  case MAP_MAGIC_1: version = 1; break;
  case MAP_MAGIC_2: version = 2; break;
  default:
    fclose(f);
    return NULL;
  }

  /* Skip the author, comment and tile */
  read_string(author, f);
  read_string(comments, f);
  read_string(tiles, f);

  if (feof(f)) {
    fclose(f);
    return NULL;
  }

  /* Skip the parameters */
  for (x = 0; x < 8; x++)
    read_int(f);
  if (version >= 1)
    for (x = 0; x < 2; x++)        /* Skip tile (w,h) */
      read_int(f);

  /* Read the dimensions */
  map_w = read_int(f);
  map_h = read_int(f);
  if (map_w <= 0 || map_h <= 0) {
    fclose(f);
    return NULL;
  }
  if (feof(f)) {
    fclose(f);
    return NULL;
  }

  if (version >= 2) {              /* Skip extensions (npc dict) */
    x = read_int(f);
    while (x-- > 0)
      fgetc(f);
  }

  if (version >= 1) {
    x = read_int(f);               /* Skip objects */
    while (x-- > 0)
      skip_object(f);
  }

  map = (MAP *) malloc(sizeof(MAP) * map_w * map_h);
  if (map == NULL) {
    fclose(f);
    return NULL;
  }
  for (y = 0; y < map_h; y++)
    for (x = 0; x < map_w; x++) {
      map[y * map_w + x].back = read_short(f);
      map[y * map_w + x].fore = read_short(f);
      map[y * map_w + x].block = read_short(f);
      if (feof(f)) {
	fclose(f);
	free(map);
	return NULL;
      }
    }
  fclose(f);
  if (w != NULL)
    *w = map_w;
  if (h != NULL)
    *h = map_h;
  return map;
}

