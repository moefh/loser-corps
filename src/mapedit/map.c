/* map.c */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#include "map.h"
#include "common.h"
#include "bitmap.h"
#include "npc_data.h"
#include "mapedit.h"

#define IS_NPC_ITEM(npc)   (npc_info[(npc)].type == NPC_TYPE_ITEM)
#define IS_NPC_ENEMY(npc)  (npc_info[(npc)].type == NPC_TYPE_ENEMY)

static NPC_INFO *npc_info;
static int num_npcs, npc_item_start, npc_enemy_start;

NPC_NAME npc_names[MAX_NPCS];


typedef struct NPC_DICT {
  int npc;
  char name[256];
} NPC_DICT;

static NPC_DICT *npc_dict = NULL;

static NPC_DICT default_npc_dict[] = {
  { 1000, "cannon" },
  { 1001, "ghost/chaser" },
  { 1002, "orange" },
  { 1003, "loser-shadow" },
  { 1004, "meat/ball" },
  { 1005, "meat/lord" },
  { 1006, "meat/slug" },
  { 1007, "ghost/walker" },

  { 2000, "tele/teleporter" },
  { 2001, "tele/target" },
  { 2002, "energy" },
  { 2003, "invincibility" },
  { 2004, "power-up" },
  { 2005, "shadow" },
  { 2006, "route" },
  { 2008, "scenario/water" },
  { 2009, "door/up" },
  { 2010, "door/down" },
  { 2011, "door/left" },
  { 2012, "door/right" },
  { 2013, "lever/star" },
  { 2014, "lever/star-invisible" },
  { 2015, "tele/teleporter-invisible" },
  { 2016, "scenario/moss" },
  { 2017, "lever/down" },
  { 2018, "lever/top" },
  { 2019, "lever/left" },
  { 2020, "lever/right" },
  { 2021, "tele/door" },
  { 2022, "scenario/eyes" },
  { 2023, "info/text" },
  { 2024, "info/arrow-exit" },
  { 2025, "info/arrow-down" },
  { 2026, "info/arrow-up" },
  { 2027, "info/arrow-left" },
  { 2028, "info/arrow-right" },
  { 2029, "door/rock-up" },
  { 2030, "door/rock-down" },
  { 2031, "door/rock-left" },
  { 2032, "door/rock-right" },
  { 2033, "door/plat-left" },
  { 2034, "door/plat-right" },
  { 2035, "energy-tank" },

  { 10000, "text" },

  { -1, "" }
};

/* Return the NPC number given the NPC name (according to `npc_info') */
static int search_npc_name(char *name)
{
  int i;

  for (i = 0; i < num_npcs; i++)
    if (strcmp(npc_info[i].npc_name, name) == 0)
      return i;
  return -1;
}

/* Convert the NPC number in the map file to the real NPC number
 * (according to the `npc_info' table) given the NPC dictionary
 * `dict'). */
static int search_npc_dict(NPC_DICT *dict, int npc)
{
  int i;

  for (i = 0; dict[i].npc >= 0; i++)
    if (dict[i].npc == npc)
      return search_npc_name(dict[i].name);
  return -1;
}


void build_npc_menu(STATE *s);

/* Build the npc_names[] array with the NPC names for the NPC menu */
void build_npc_table(STATE *s)
{
  int i, name;
  char cur_name[256], last_name[256];

  npc_info = s->npc_info;
  num_npcs = s->num_npcs;

  for (i = 0; i < num_npcs; i++)
    if (npc_info[i].type == NPC_TYPE_ENEMY) {
      npc_enemy_start = i;
      break;
    }

  for (i = 0; i < num_npcs; i++)
    if (npc_info[i].type == NPC_TYPE_ITEM) {
      npc_item_start = i;
      break;
    }

  name = 0;
  last_name[0] = '\0';
  for (i = 0; i < num_npcs; i++) {
    if (npc_info[i].type == NPC_TYPE_ENEMY
	|| npc_info[i].type == NPC_TYPE_ITEM) {
      static char *names[2] = { "Enemy", "Item" };
      char *str_type = names[(npc_info[i].type == NPC_TYPE_ENEMY) ? 0 : 1];
      char *p;

      strcpy(cur_name, npc_info[i].npc_name);
      p = strchr(cur_name, '/');

      npc_names[name].npc = i;
#if 1
      sprintf(npc_names[name].name, "%s: %s", str_type, cur_name);
#else
      strcpy(npc_names[name].name, cur_name);
#endif
      npc_names[name].child = 0;

      if (p != NULL && last_name[0] != '\0') {
	*p++ = '\0';

	if (strncmp(cur_name, last_name, strlen(cur_name)) == 0)
	  npc_names[name].child = 1;
      }
      strcpy(last_name, cur_name);
      name++;
    }
  }

  npc_names[name].npc = -1;
  npc_names[name].name[0] = '\0';

  build_npc_menu(s);
}

int get_npc_index(int npc)
{
  int i;

  for (i = 0; npc_names[i].npc >= 0; i++)
    if (npc == npc_names[i].npc)
      return i;
  return -1;
}

int get_npc_number(char *name)
{
  int i;

  for (i = 0; npc_names[i].npc >= 0; i++)
    if (strcmp(name, npc_names[i].name) == 0)
      return npc_names[i].npc;
  return -1;
}

char *get_npc_name(int npc)
{
  int i;

  for (i = 0; npc_names[i].npc >= 0; i++)
    if (npc == npc_names[i].npc)
      return npc_names[i].name;
  return NULL;
}


MAP *create_map(int w, int h)
{
  MAP *map;
  int y;

  map = (MAP *) malloc(sizeof(MAP) + sizeof(MAP_DATA *) * h);
  if (map == NULL)
    return NULL;
  map->data = (MAP_DATA *) malloc(sizeof(MAP_DATA) * w * h);
  if (map->data == NULL)
    return NULL;

  map->author[0] = '\0';
  map->comment[0] = '\0';
  map->tiles[0] = '\0';
  map->maxyspeed = DEFAULT_MAX_Y_SPEED;
  map->jumphold = DEFAULT_JUMP_HOLD;
  map->gravity = DEFAULT_GRAVITY;
  map->maxxspeed = DEFAULT_MAX_X_SPEED;
  map->accel = DEFAULT_WALK_ACCEL;
  map->jumpaccel = DEFAULT_JUMP_ACCEL;
  map->attrict = DEFAULT_ATTRICT;
  map->frameskip = DEFAULT_FRAME_SKIP;
  map->tile_w = map->tile_h = 64;

  map->obj = NULL;

  map->w = w;
  map->h = h;
  for (y = 0; y < map->h; y++)
    map->line[y] = map->data + y * w;
  return map;
}

static void destroy_objects(MAP_OBJECT *obj)
{
  if (obj != NULL) {
    destroy_objects(obj->next);
    free(obj);
  }
}

void destroy_map(MAP *map)
{
  destroy_objects(map->obj);
  free(map->data);
  free(map);
}


#if 0
/*********************/

/* Convert a logical NPC to the map file NPC number */
static int npc_to_map(int npc)
{
  if (npc == npc_item_start + ITEM_MAP_TEXT)
    return MAP_MAP_TEXT;
  if (IS_NPC_ENEMY(npc))
    return npc - npc_enemy_start + MAP_ENEMY_START;
  if (IS_NPC_ITEM(npc))
    return npc - npc_item_start + MAP_ITEM_START;
  return npc;
}

/* Convert a NPC number in the map file to a logical NPC */
static int map_to_npc(int npc)
{
  if (npc >= MAP_MAP_TEXT)
    return npc_item_start + ITEM_MAP_TEXT;

  if (npc >= MAP_ITEM_START)
    return npc - MAP_ITEM_START + npc_item_start;
  if (npc >= MAP_ENEMY_START)
    return npc - MAP_ENEMY_START + npc_enemy_start;

  /* Handle maps written by older versions */
  if (npc >= 6 && npc <= 9)
    return npc - 6 + npc_enemy_start;
  if (npc >= 10 && npc <= 16)
    return npc - 10 + npc_item_start;
  return npc;
}

/*********************/
#endif

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

/* Write a 4 byte integer */
static void write_int(int s, FILE *f)
{
  fputc(s & 0xff, f);
  fputc((s >> 8) & 0xff, f);
  fputc((s >> 16) & 0xff, f);
  fputc((s >> 24) & 0xff, f);
}

/* Write a 2 byte integer */
static void write_short(short int s, FILE *f)
{
  fputc(s & 0xff, f);
  fputc((s >> 8) & 0xff, f);
}


static void write_string(char *str, FILE *f)
{
  int i, n;

  n = strlen(str);
  fputc(n, f);
  for (i = 0; i < n; i++)
    fputc(str[i], f);
}

static int read_string(char *str, int max, FILE *f)
{
  int n, i;

  n = fgetc(f);
  if (n < 0)
    return -1;
  for (i = 0; i < (max-1) && i < n; i++)
    str[i] = fgetc(f);
  str[i] = '\0';
  for (; i < n; i++)
    fgetc(f);
  return n;
}

/* Initialize object with default values */
void fill_default_obj_values(MAP_OBJECT *obj)
{
  obj->npc = npc_item_start;
  obj->x = obj->y = 0;
  obj->dir = 0;
  obj->respawn = 10;
  obj->level = 0;
  obj->duration = 30;
  obj->target = -1;
  obj->vulnerability = 100;
  obj->text[0] = '\0';
}

/* Read the object data from the file */
static void read_object(MAP_OBJECT *obj, int size, FILE *f)
{
  int i;
  short int data;
  NPC_DICT *dict;

  if (npc_dict == NULL)
    dict = default_npc_dict;
  else
    dict = npc_dict;

  memset(obj, 0, sizeof(*obj));
  fill_default_obj_values(obj);

  /* Read present values */
  for (i = 0; i < size; i++) {
    data = read_short(f);
    switch (i) {
    case 0:
      obj->npc = search_npc_dict(dict, data);

      if (obj->npc < 0) {
	printf("ERROR: can't get NPC number for object `%d'\n", data);
	obj->npc = npc_item_start;
      }
#if 0
      printf("map_obj %5d is NPC %d (%s)\n",
	     data, obj->npc, npc_info[obj->npc].npc_name);
#endif

      if (data == MAP_MAP_TEXT) {
	int n = (size - 1) * 2;         /* # of chars to read */

	if (n > ITEM_TEXT_LEN) {
	  int k;

	  if (fread(obj->text, 1, ITEM_TEXT_LEN, f) != ITEM_TEXT_LEN) {
            printf("ERROR: can't read object text\n");
            return;
          }
          
	  for (k = ITEM_TEXT_LEN-1; k < n; k++)
	    fgetc(f);
	  obj->text[ITEM_TEXT_LEN-1] = '\0';
	} else {
	  if (fread(obj->text, 1, n, f) != n) {
            printf("ERROR: can't read object text\n");
            return;
          }
	  obj->text[n] = '\0';
	}
	i = size;              /* Stop reading */
      }
      break;

    case 1: obj->x = data; break;
    case 2: obj->y = data; break;
    case 3: obj->dir = data; break;
    case 4: obj->respawn = data; break;
    case 5: obj->level = data; break;
    case 6: obj->duration = data; break;
    case 7: obj->target = data; break;
    case 8: obj->vulnerability = data; break;
    }
  }    

#if 0
  /* Convert the NPC number of the map to the logical NPC number */
  obj->npc = map_to_npc(obj->npc);
#endif
}


/* Write the current NPC dictionary, based on the `npc_info' table */
static int write_npc_dict(FILE *f)
{
  int n_bytes, n_npcs, i;

  /* Count the number of bytes in the dictionary */
  n_npcs = 0;
  n_bytes = 2;     /* Size of the number of NPCs */
  for (i = 0; i < num_npcs; i++)
    if (npc_info[i].type == NPC_TYPE_ENEMY
	|| npc_info[i].type == NPC_TYPE_ITEM) {
      n_bytes += 3 + strlen(npc_info[i].npc_name);
      n_npcs++;
    }

  write_int(n_bytes, f);
  write_short(n_npcs, f);

  for (i = 0; i < num_npcs; i++)
    if (npc_info[i].type == NPC_TYPE_ENEMY
	|| npc_info[i].type == NPC_TYPE_ITEM) {
      if (strcmp(npc_info[i].npc_name, "text") == 0)
	write_short(MAP_MAP_TEXT, f);
      else
	write_short(i, f);
      write_string(npc_info[i].npc_name, f);
    }

  return 0;
}

/* Read the NPC dictionary, returns the number of bytes read (-1 on
 * error). */
static int read_npc_dict(FILE *f)
{
  int n_npcs, i;
  int n_bytes = 0;

  n_npcs = read_short(f);
  n_bytes += 2;
  if (n_npcs <= 0)
    return n_bytes;
  npc_dict = (NPC_DICT *) malloc(sizeof(NPC_DICT) * (n_npcs + 1));
  if (npc_dict < 0)
    return -1;
  for (i = 0; i < n_npcs; i++) {
    npc_dict[i].npc = read_short(f);
    read_string(npc_dict[i].name, 256, f);
    n_bytes += 3 + strlen(npc_dict[i].name);
#if 0
    n = (unsigned int) fgetc(f);
    for (j = 0; j < n; j++)
      npc_dict[i].name[j] = fgetc(f);
    npc_dict[i].name[n] = '\0';
#endif
  }
  npc_dict[n_npcs].npc = -1;
  npc_dict[n_npcs].name[0] = '\0';

  return n_bytes;
}

/* Read a map from a file */
MAP *read_map(char *filename)
{
  FILE *f;
  MAP *map;
  MAP_OBJECT *map_obj = NULL;
  int magic, version, map_w, map_h, x, y;
  int tile_w, tile_h;
  char author[256], tiles[256], comment[256];
  int maxyspeed;         /* Max vertical speed */
  int jumphold;          /* Jump hold */
  int gravity;           /* Gravity */
  int maxxspeed;         /* Max horizontal speeed */
  int accel;             /* Walk accel */
  int jumpaccel;         /* Jump accel */
  int attrict;           /* Friction */
  int frameskip;         /* Frame skip */

  if ((f = fopen(filename, "rb")) == NULL)
    return NULL;

  /* Check format */
  magic = read_int(f);
  switch (magic) {
  case MAP_MAGIC_0: version = 0; break;
  case MAP_MAGIC_1: version = 1; break;
  case MAP_MAGIC_2: version = 2; break;
  default:
    fclose(f);
    return NULL;
  }

  /* Read author and tile file */
  read_string(author, 256, f);
  read_string(comment, 256, f);
  read_string(tiles, 256, f);

  /* Read parameters */
  maxyspeed = read_int(f);
  jumphold = read_int(f);
  gravity = read_int(f);
  maxxspeed = read_int(f);
  accel = read_int(f);
  jumpaccel = read_int(f);
  attrict = read_int(f);
  frameskip = read_int(f);
  if (version >= 1) {
    tile_w = read_int(f);
    tile_h = read_int(f);
  } else {
    tile_w = 64;
    tile_h = 64;
  }

  /* Read dimensions */
  map_w = read_int(f);
  map_h = read_int(f);
  if (map_w <= 0 || map_h <= 0) {
    fclose(f);
    return NULL;
  }

  /* Read the NPC dictionary */
  npc_dict = NULL;
  if (version >= 2) {
    int extra_bytes;

    extra_bytes = read_int(f);
    if (extra_bytes >= 2)
      extra_bytes -= read_npc_dict(f);
    for (; extra_bytes > 0; extra_bytes--)
      fgetc(f);
  }

  /* Read object data */
  if (version >= 1) {
    int n_objs, obj_size;
    MAP_OBJECT *obj, *prev = NULL;

    n_objs = read_int(f);
    while (n_objs-- > 0) {
      obj_size = read_short(f);
      obj = (MAP_OBJECT *) malloc(sizeof(MAP_OBJECT));
      read_object(obj, obj_size, f);
      obj->next = NULL;
      if (prev == NULL)
	map_obj = obj;
      else
	prev->next = obj;
      prev = obj;
    }
  }

  if (npc_dict != NULL) {
    free(npc_dict);
    npc_dict = NULL;
  }

  map = (MAP *) malloc(sizeof(MAP) + sizeof(MAP_DATA *) * map_h);
  if (map == NULL) {
    fclose(f);
    return NULL;
  }
  map->data = (MAP_DATA *) malloc(sizeof(MAP_DATA) * map_w * map_h);
  if (map->data == NULL) {
    fclose(f);
    free(map);
    return NULL;
  }

  /* Read the tiles */
  for (y = 0; y < map_h; y++) {
    map->line[y] = map->data + y * map_w;
    for (x = 0; x < map_w; x++) {
      map->line[y][x].back = read_short(f);
      map->line[y][x].fore = read_short(f);
      map->line[y][x].block = read_short(f);
    }
  }
  fclose(f);
  map->w = map_w;
  map->h = map_h;
  strcpy(map->author, author);
  strcpy(map->comment, comment);
  strcpy(map->tiles, tiles);

  map->maxyspeed = maxyspeed;
  map->jumphold = jumphold;
  map->gravity = gravity;
  map->maxxspeed = maxxspeed;
  map->accel = accel;
  map->jumpaccel = jumpaccel;
  map->attrict = attrict;
  map->frameskip = frameskip;
  map->tile_w = tile_w;
  map->tile_h = tile_h;
  map->obj = map_obj;
  return map;
}

static void write_map_objects(MAP_OBJECT *objects, FILE *f)
{
  int n_objs;
  MAP_OBJECT *obj;

  if (objects == NULL) {
    write_int(0, f);
    return;
  }

  /* Count the # of objects */
  n_objs = 0;
  for (obj = objects; obj != NULL; obj = obj->next)
    n_objs++;

  write_int(n_objs, f);    /* Write # of objects */

  for (obj = objects; obj != NULL; obj = obj->next) {
    if (strcmp(npc_info[obj->npc].npc_name, "text") == 0) {
      /* Object size (in short's) */
      write_short(1 + (1+strlen(obj->text))/2, f);
      /* Object NPC */
      write_short(MAP_MAP_TEXT, f);
      /* Object data */
      fwrite(obj->text, 1, strlen(obj->text), f);
      if (strlen(obj->text) % 2 != 0)
	fputc(0, f);        /* Even the number of bytes on the map name */
      continue;
    }
    write_short(9, f);          /* Object size (in short's) */
    write_short(obj->npc, f);   /* Object NPC */
    /* Object data */
    write_short(obj->x, f);
    write_short(obj->y, f);
    write_short(obj->dir, f);
    write_short(obj->respawn, f);
    write_short(obj->level, f);
    write_short(obj->duration, f);
    write_short(obj->target, f);
    write_short(obj->vulnerability, f);
  }
}

/* Write a map to a file */
int write_map(char *filename, MAP *map)
{
  FILE *f;
  int x, y;

  if ((f = fopen(filename, "wb")) == NULL)
    return 1;
  write_int(MAP_MAGIC_2, f);
  write_string(map->author, f);
  write_string(map->comment, f);
  write_string(map->tiles, f);
  write_int(map->maxyspeed, f);
  write_int(map->jumphold, f);
  write_int(map->gravity, f);
  write_int(map->maxxspeed, f);
  write_int(map->accel, f);
  write_int(map->jumpaccel, f);
  write_int(map->attrict, f);
  write_int(map->frameskip, f);
  write_int(map->tile_w, f);
  write_int(map->tile_h, f);
  write_int(map->w, f);
  write_int(map->h, f);

  write_npc_dict(f);
  write_map_objects(map->obj, f);

  for (y = 0; y < map->h; y++)
    for (x = 0; x < map->w; x++) {
      write_short(map->line[y][x].back, f);
      write_short(map->line[y][x].fore, f);
      write_short(map->line[y][x].block, f);
    }
  fclose(f);
  return 0;
}
