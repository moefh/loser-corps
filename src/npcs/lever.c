/* lever.c
 *
 * Copyright (C) 1999 Ricardo R. Massaro
 *
 * NOTE: the door state is set to:
 *           -1 = moving
 *            0 = closed
 *            1 = open
 *
 * Additionally, the door state_data is set to the blocking tile number
 * if the door is opened (it will be used when closing the door).
 */

#include <stdio.h>
#include <stdlib.h>

#include <../server/server.h>


/* Sets the item's frame */
static void set_door_frame(SERVER *server, EVENT *e)
{
  ITEM *it = server->item + e->data[0].i;
  int i = e->data[1].i;

  it->client_frame = i;
  it->changed = 1;
}


/* Unblock the map where the door stands */
static void open_door(SERVER *server, EVENT *e)
{
  ITEM *it = server->item + e->data[0].i;
  int block_x = e->data[1].i;
  int block_y = e->data[2].i;

  it->state = 1;
  server->map[block_x + block_y * server->map_w] = -1;
}

/* Block the map where the door stands */
static void close_door(SERVER *server, EVENT *e)
{
  ITEM *it = server->item + e->data[0].i;
  int block_x = e->data[1].i;
  int block_y = e->data[2].i;
  int old = e->data[3].i;

  it->state = 0;
  server->map[block_x + block_y * server->map_w] = old;
}

/* Program the lever change */
static void program_lever_change(SERVER *server, ITEM *lever, NPC_INFO *lever_npc)
{
  int frames_left, i;
  EVENT *e;

  frames_left = 2;
  if (lever->client_frame == 0)
    for (i = 1; i < lever_npc->n_stand/2; i++) {
      e = srv_allocate_event(server);
      if (e == NULL) {
	printf("ERROR: can't allocate event\n");
	exit(1);
	return;
      }
      e->frames_left = frames_left++;
      e->frames = -1;
      e->func = set_door_frame;
      e->data[0].i = lever->id;
      e->data[1].i = i;
    }
  else
    for (i = 1; i < lever_npc->n_stand/2; i++) {
      e = srv_allocate_event(server);
      if (e == NULL) {
	printf("ERROR: can't allocate event\n");
	exit(1);
	return;
      }
      e->frames_left = frames_left++;
      e->frames = -1;
      e->func = set_door_frame;
      e->data[0].i = lever->id;
      e->data[1].i = lever_npc->n_stand/2 - i - 1;
    }
}

/* Schedule the events of the door opening */
static void program_door_open(SERVER *server, ITEM *lever, NPC_INFO *lever_npc,
			      ITEM *target, NPC_INFO *target_npc,
			      int block_x, int block_y)
{
  int i, frames_left;
  EVENT *e;
  SOUND *s;
  int sfx_open_id;

  if ((sfx_open_id = sfx_id(server, "opendoor")) >= 0) {
    s = srv_allocate_sound(server);
    if (s != NULL) {
      s->sfx = sfx_open_id;
      s->x = target->x;
      s->y = target->y;
    }
  }

  target->state_data = server->map[block_x + block_y * server->map_w];

  program_lever_change(server, lever, lever_npc);

  /* Events for the animation */
  frames_left = 2;
  for (i = 1; i < target_npc->n_stand/2; i++) {
    e = srv_allocate_event(server);
    if (e == NULL) {
      printf("ERROR: can't allocate event\n");
      exit(1);
      return;
    }
    e->frames_left = frames_left++;
    e->frames = -1;
    e->func = set_door_frame;
    e->data[0].i = target->id;
    e->data[1].i = i;
  }

  /* Unblock map event */
  e = srv_allocate_event(server);
  if (e == NULL) {
    printf("ERROR: can't allocate event\n");
    exit(1);
    return;
  }
  e->frames_left = frames_left;
  e->frames = -1;
  e->func = open_door;
  e->data[0].i = target->id;
  e->data[1].i = block_x;
  e->data[2].i = block_y;
}

/* Schedule the events of the door closing */
static void program_door_close(SERVER *server, ITEM *lever, NPC_INFO*lever_npc,
			       ITEM *target, NPC_INFO *target_npc,
			       int block_x, int block_y)
{
  int i, frames_left;
  EVENT *e;
  SOUND *s;
  int sfx_close_id;

  if ((sfx_close_id = sfx_id(server, "closedoor")) >= 0) {
    s = srv_allocate_sound(server);
    if (s != NULL) {
      s->sfx = sfx_close_id;
      s->x = target->x;
      s->y = target->y;
    }
  }


  program_lever_change(server, lever, lever_npc);

  /* Events for the animation */
  frames_left = 2;
  for (i = 1; i < target_npc->n_stand/2; i++) {
    e = srv_allocate_event(server);
    if (e == NULL) {
      printf("ERROR: can't allocate event\n");
      exit(1);
      return;
    }
    e->frames_left = frames_left++;
    e->frames = -1;
    e->func = set_door_frame;
    e->data[0].i = target->id;
    e->data[1].i = target_npc->n_stand/2 - i - 1;
  }

  /* Block event */
  e = srv_allocate_event(server);
  if (e == NULL) {
    printf("ERROR: can't allocate event\n");
    exit(1);
    return;
  }
  e->frames_left = frames_left;
  e->frames = -1;
  e->func = close_door;
  e->data[0].i = target->id;
  e->data[1].i = block_x;
  e->data[2].i = block_y;
  e->data[3].i = target->state_data;
}



/* This gets executed when a lever is touched */
void get_item(SERVER *server, ITEM *it, CLIENT *client)
{
  ITEM *target;
  int target_map_id, target_npc, target_id;
  int block_x, block_y;

  if (server->change_map || ! it->pressed || it->map_id < 0)
    return;

  target_map_id = server->map_item[it->map_id].target_map_id;
  if (target_map_id < 0
      || server->map_item[target_map_id].npc < 0
      || server->map_item[target_map_id].id < 0)
    return;

  target_npc = server->map_item[target_map_id].npc;
  target_id = server->map_item[target_map_id].id;

  if (target_npc < 0 || target_id < 0)
    return;

  target = server->item + target_id;
  if (target->state < 0)
    return;

  if (target->target_x >= 0 && target->target_y >= 0) {
    block_x = target->target_x / server->tile_w;
    block_y = target->target_y / server->tile_h;
  } else {
    block_x = (it->target_x + server->npc_info[target_npc].clip_w/2)/server->tile_w;
    block_y = (it->target_y + server->npc_info[target_npc].clip_h/2)/server->tile_h;
  }

  if (block_x < 0 || block_y < 0
      || block_x >= server->map_w || block_y >= server->map_h)
    return;

  if (target->state == 0)
    program_door_open(server, it, server->npc_info + it->npc,
		      target, server->npc_info+target_npc,
		      block_x, block_y);
  else
    program_door_close(server, it, server->npc_info + it->npc,
		       target, server->npc_info+target_npc,
		       block_x, block_y);
  target->state = -1;
}
