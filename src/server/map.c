/* map.c
 *
 * Copyright (C) 1998,1999 Ricardo R. Massaro
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "server.h"
#include "npc_data.h"

typedef struct MAP_OBJECT {
  int npc;                   /* Object NPC */
  int map_id;                /* Item or enemy map id, after created */
  int x, y;                  /* Position in the map */
  int dir;                   /* Direction */
  int respawn;               /* Respawn time, in seconds */
  int level;                 /* Level, if enemy */
  int duration;              /* Duration, if item */
  int target;                /* Target object */
  int vulnerability;         /* Vulnerability to weapons (%) */
  char text[ITEM_TEXT_LEN];  /* Target map name */
} MAP_OBJECT;

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
static int search_npc_name(SERVER *server, char *name)
{
  int i;

  for (i = 0; i < server->n_npcs; i++)
    if (strcmp(server->npc_info[i].npc_name, name) == 0)
      return i;
  return -1;
}

/* Convert the NPC number in the map file to the real NPC number
 * (according to the `npc_info' table) given the NPC dictionary
 * `dict'). */
static int search_npc_dict(SERVER *server, NPC_DICT *dict, int npc)
{
  int i;

  for (i = 0; dict[i].npc >= 0; i++)
    if (dict[i].npc == npc)
      return search_npc_name(server, dict[i].name);
  return -1;
}



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


/* Read the NPC dictionary, returns the number of bytes read (-1 on
 * error). */
static int read_npc_dict(SERVER *server, FILE *f)
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
  }
  npc_dict[n_npcs].npc = -1;
  npc_dict[n_npcs].name[0] = '\0';

  return n_bytes;
}


static void read_object(SERVER *server, MAP_OBJECT *obj, int size, FILE *f)
{
  int i;
  short data;
  NPC_DICT *dict;

  if (npc_dict == NULL)
    dict = default_npc_dict;
  else
    dict = npc_dict;

  memset(obj, 0, sizeof(*obj));
  obj->npc = server->npc_num.item_start;
  obj->x = obj->y = 0;
  obj->dir = 0;
  obj->respawn = 30;
  obj->level = 0;
  obj->duration = 30;
  obj->target = -1;
  obj->vulnerability = 100;
  obj->text[0] = '\0';

  for (i = 0; i < size; i++) {
    data = read_short(f);
    switch (i) {
    case 0:
      obj->npc = search_npc_dict(server, dict, data);
      if (obj->npc < 0) {
	DEBUG("ERROR: can't get NPC number for object `%d'\n", data);
	obj->npc = server->npc_num.item_start;
      }

      if (data == MAP_MAP_TEXT) {
	int n = (size - 1) * 2;         /* # of chars to read */

	if (n > ITEM_TEXT_LEN) {
	  int k;
	  
	  fread(obj->text, 1, ITEM_TEXT_LEN, f);
	  for (k = ITEM_TEXT_LEN-1; k < n; k++)
	    fgetc(f);
	  obj->text[ITEM_TEXT_LEN-1] = '\0';
	} else {
	  fread(obj->text, 1, n, f);
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
}

/* Read the map objects from the file f */
static MAP_OBJECT *read_map_objects(SERVER *server, FILE *f, int *num_objects)
{
  MAP_OBJECT *obj;
  int i, n_objs, obj_size;

  n_objs = read_int(f);
  if (n_objs == 0) {
    *num_objects = 0;
    return NULL;        /* No objects */
  }

  obj = (MAP_OBJECT *) malloc(sizeof(MAP_OBJECT) * n_objs);
  if (obj == NULL) {                    /* OUT OF MEMORY!!! */
    int j;

    *num_objects = 0;
    for (i = 0; i < n_objs; i++) {      /* Ignore objects */
      obj_size = read_short(f);
      for (j = 0; j < obj_size; j++)
	read_short(f);
    }
    return NULL;
  }
  for (i = 0; i < n_objs; i++) {
    obj_size = read_short(f);
    read_object(server, obj + i, obj_size, f);
  }
  *num_objects = n_objs;
  return obj;
}

/* Convert the objects read from the map file to actual items and enemies */
static void convert_map_objects(SERVER *server,
				MAP *map, MAP_OBJECT *obj, int n_objs)
{
  MAP_ENEMY *e = NULL;
  MAP_ITEM *it = NULL;
  int n_e, n_i, i;

  n_e = n_i = 0;
  for (i = 0; i < n_objs; i++)
    if (IS_NPC_ENEMY(server, obj[i].npc))
      n_e++;
    else if (IS_NPC_ITEM(server, obj[i].npc)
	     && strcmp(server->npc_info[obj[i].npc].npc_name, "text") != 0)
      n_i++;

  if (n_e > 0)
    e = (MAP_ENEMY *) malloc(sizeof(MAP_ENEMY) * n_e);
  if (n_i > 0)
    it = (MAP_ITEM *) malloc(sizeof(MAP_ITEM) * n_i);
  n_e = n_i = 0;
  for (i = 0; i < n_objs; i++)
    if (IS_NPC_ENEMY(server, obj[i].npc)) {
      e[n_e].npc = obj[i].npc;
      e[n_e].x = obj[i].x;
      e[n_e].y = obj[i].y;
      e[n_e].dir = obj[i].dir;
      e[n_e].intelligence = obj[i].level;
      e[n_e].respawn_time = obj[i].respawn;
      e[n_e].vulnerability = obj[i].vulnerability;
      obj[i].map_id = n_e;
      n_e++;
    } else if (IS_NPC_ITEM(server, obj[i].npc)
	       && strcmp(server->npc_info[obj[i].npc].npc_name, "text") != 0) {
      it[n_i].npc = obj[i].npc;
      it[n_i].x = obj[i].x;
      it[n_i].y = obj[i].y;
      it[n_i].respawn_time = obj[i].respawn;
      it[n_i].duration = obj[i].duration;
      it[n_i].level = obj[i].level;
      if (obj[i].target >= 0 && obj[i].target < n_objs) {
	it[n_i].target_x = obj[obj[i].target].x;
	it[n_i].target_y = obj[obj[i].target].y;
	strcpy(it[n_i].text, obj[obj[i].target].text);
      } else {
	it[n_i].target_x = it[n_i].target_y = -1;
	it[n_i].text[0] = '\0';
      }
      it[n_i].target_map_id = -1;    /* Will be set later */
      obj[i].map_id = n_i;
      it[n_i].target_obj_num = obj[i].target;
      n_i++;
    }

  /* Fix references for target map IDs */
  for (i = 0; i < n_i; i++)
    if (it[i].target_obj_num >= 0 && it[i].target_obj_num < n_objs)
      it[i].target_map_id = obj[it[i].target_obj_num].map_id;

  map->enemy = e;
  map->n_enemies = n_e;
  map->item = it;
  map->n_items = n_i;
}


/* Read the NPC info */
static int read_npc_info(SERVER *server)
{
  int num_npcs, i;
  NPC_INFO npc_info[MAX_NPCS], *p;
  char filename[256];

  snprintf(filename, sizeof(filename), "%snpcs.dat", DATA_DIR);
  num_npcs = parse_npc_info(npc_info, filename);
  if (num_npcs < 0)
    return 1;

  server->npc_num.num = num_npcs;
  server->npc_num.jack_start = 0;
  for (i=0; i < num_npcs && npc_info[i].type == NPC_TYPE_JACK; i++)
    ;
  server->npc_num.jack_end = i - 1;
  server->npc_num.weapon_start = i;
  for (; i < num_npcs && npc_info[i].type == NPC_TYPE_WEAPON; i++)
    ;
  server->npc_num.weapon_end = i - 1;
  server->npc_num.enemy_start = i;
  for (; i < num_npcs && npc_info[i].type == NPC_TYPE_ENEMY; i++)
    ;
  server->npc_num.enemy_end = i - 1;
  server->npc_num.item_start = i;
  for (; i < num_npcs && npc_info[i].type == NPC_TYPE_ITEM; i++)
    ;
  server->npc_num.item_end = i - 1;

  if (server->npc_info != NULL)
    p = (NPC_INFO *) realloc(server->npc_info, sizeof(NPC_INFO) * num_npcs);
  else
    p = (NPC_INFO *) malloc(sizeof(NPC_INFO) * num_npcs);
  if (p == NULL) {
    DEBUG("Out of memory for NPC info\n");
    return 1;
  }
  server->npc_info = p;
  server->n_npcs = num_npcs;
  memcpy(server->npc_info, npc_info, sizeof(NPC_INFO) * num_npcs);

  return 0;
}

#ifdef HAS_SOUND
/* Read the sound info */
static void read_sound_info(SERVER *server)
{
  char filename[256];

  snprintf(filename, sizeof(filename), "%ssound.dat", DATA_DIR);
  parse_sound_info(&server->sound_info, filename);

  sfx_bump = sfx_id(server, "bump");
  sfx_explosion = sfx_id(server, "explosion");
  sfx_blip = sfx_id(server, "blip");
}
#endif

/* Read a map from a file */
int srv_read_map(SERVER *server, char *filename, MAP *ret_map)
{
  FILE *f;
  char *map;
  int magic, version, map_w, map_h, map_tile_w, map_tile_h;
  int x, y, num_objects;
  MAP_OBJECT *obj;

  if (read_npc_info(server))    /* Re-read NPC info just in case it changed */
    return MSGRET_EMAPFILE;
#ifdef HAS_SOUND
  read_sound_info(server);      /* Re-read sound info also */
#endif

  if ((f = fopen(filename, "rb")) == NULL)
    return MSGRET_EMAPFILE;
  magic = read_int(f);
  switch (magic) {
  case MAP_MAGIC_0: version = 0; break;
  case MAP_MAGIC_1: version = 1; break;
  case MAP_MAGIC_2: version = 2; break;
  default:
    fclose(f);
    DEBUG("%s: Invalid map magic: %d\n", filename, magic);
    return MSGRET_EMAPFMT;
  }

  /* Skip the author, comment and tileset */
  x = fgetc(f);
  for (y = 0; y < x; y++)
    fgetc(f);
  x = fgetc(f);
  for (y = 0; y < x; y++)
    fgetc(f);
  x = fgetc(f);
  for (y = 0; y < x; y++)
    fgetc(f);

  /* Read the parameters */
  for (x = 0; x < 8; x++)
    ret_map->parms[x] = read_int(f);
  if (version >= 1) {
    map_tile_w = read_int(f);
    map_tile_h = read_int(f);
  } else {
    map_tile_w = 64;
    map_tile_h = 64;
  }

  /* Map dimensions */
  map_w = read_int(f);
  map_h = read_int(f);
  if (map_w <= 0 || map_h <= 0 || map_w * map_h <= 0) {
    fclose(f);
    return MSGRET_EMAPFMT;
  }

  /* Allocate map memory */
  map = (char *) malloc(map_w * map_h);
  if (map == NULL) {
    fclose(f);
    return MSGRET_EMEM;
  }

  /* Read dictionary */
  npc_dict = NULL;
  if (version >= 2) {
    int n_bytes;

    n_bytes = read_int(f);
    n_bytes -= read_npc_dict(server, f);
    while (n_bytes-- > 0)
      fgetc(f);
  }    

  /* Read objects */
  if (version >= 1)
    obj = read_map_objects(server, f, &num_objects);
  else {
    num_objects = 0;
    obj = NULL;
  }

  if (npc_dict != NULL) {
    free(npc_dict);
    npc_dict = NULL;
  }

  /* Read data */
  for (y = 0; y < map_h; y++) {
    for (x = 0; x < map_w; x++) {
      read_short(f);                /* Background */
      read_short(f);                /* Foreground */
      map[y * map_w + x] = (char) read_short(f);
      if (feof(f) || ferror(f)) {
	if (obj != NULL)
	  free(obj);
	free(map);
	fclose(f);
	return MSGRET_EMAPFMT;
      }
    }
  }
  fclose(f);

  convert_map_objects(server, ret_map, obj, num_objects);
  if (obj != NULL)
    free(obj);
  ret_map->w = map_w;
  ret_map->h = map_h;
  ret_map->tile_w = map_tile_w;
  ret_map->tile_h = map_tile_h;
  ret_map->data = map;
  return MSGRET_OK;
}
