/* item.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "addon.h"


/* Dumb item notify function: do nothing.  This will be used for items
 * whose libraries can't be found. */
static void get_item_do_nothing(SERVER *server, ITEM *it, CLIENT *client)
{
}

/* Return an item NPC number given its name */
int get_item_npc(SERVER *server, char *name)
{
  int i;

  for (i = server->npc_num.item_start; i <= server->npc_num.item_end; i++)
    if (strcmp(server->npc_info[i].npc_name, name) == 0)
      return i;
  return server->npc_num.item_start;
}

/* Make a new standalone (no-respawn) item */
ITEM *make_new_item(SERVER *server, int npc, int x, int y)
{
  ITEM *it;
  ADDON_LIBRARY *lib;

  it = srv_allocate_item(server);
  if (it == NULL) {
    DEBUG("Out of memory for item\n");
    return NULL;
  }

  if (server->npc_info[npc].lib_name[0] != '\0')
    lib = load_addon_library(server->npc_info[npc].lib_name, "get_item");
  else
    lib = NULL;

  it->npc = npc;
  it->x = x;
  it->y = y;
  it->map_id = -1;
  it->destroy = 0;
  it->create = 1;
  it->client_frame = 0;
  it->state = 0;

  if (lib != NULL)
    it->get_item = lib->function;
  else
    it->get_item = get_item_do_nothing;

  it->target_x = it->target_y = -1;
  it->text[0] = '\0';

  return it;
}


/* Create a item based on a map item */
void create_item(SERVER *server, MAP_ITEM *map_item, int map_id)
{
  ITEM *it;

  it = make_new_item(server, map_item->npc, map_item->x, map_item->y);
  if (it == NULL)
    return;
  it->map_id = map_id;
  map_item->id = it->id;
  map_item->respawn_left = -1;

  /* Copy the item attributes */
  it->duration = map_item->duration;
  it->level = map_item->level;
  it->target_x = map_item->target_x;
  it->target_y = map_item->target_y;
  strcpy(it->text, map_item->text);
}

/* Create the items in a map */
void create_map_items(SERVER *server)
{
  int i;

  for (i = 0; i < server->n_map_items; i++)
    create_item(server, server->map_item + i, i);
}


/* Add an item in the active list, allocating memory if needed */
ITEM *srv_allocate_item(SERVER *server)
{
  ITEM *it;
  int i, n, old_n;

  for (i = 0; i < server->n_items; i++)
    if (server->item[i].active == 0) {
      server->item[i].active = 1;
      server->item[i].changed = 0;
      server->item[i].state = 0;
      return server->item + i;
    }

  if (server->item == NULL) {
    old_n = 0;
    n = 16;
    it = (ITEM *) malloc(sizeof(ITEM) * n);
  } else {
    old_n = server->n_items;
    n = server->n_items + 16;
    it = (ITEM *) realloc(server->item, sizeof(ITEM) * n);
  }
  if (it == NULL)
    return NULL;    /* Out of memory */
  server->n_items = n;
  server->item = it;
  for (i = old_n+1; i < server->n_items; i++) {
    server->item[i].id = i;
    server->item[i].active = 0;
  }
  server->item[old_n].id = old_n;
  server->item[old_n].active = 1;
  server->item[old_n].changed = 0;
  server->item[old_n].state = 0;
  return server->item + old_n;
}

/* Remove an item from the active list */
void srv_free_item(SERVER *server, ITEM *it)
{
  it->active = 0;
}
