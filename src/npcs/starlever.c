/* starlever.c
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
static void set_item_frame(SERVER *server, EVENT *e)
{
  ITEM *it = server->item + e->data[0].i;
  int i = e->data[1].i;

  it->client_frame = i;
  it->changed = 1;
}

/* Play a SFX */
static void play_sound(SERVER *server, EVENT *e)
{
  ITEM *it;
  int sfx_id;
  SOUND *s;

  it = server->item + e->data[0].i;
  sfx_id = e->data[1].i;

  s = srv_allocate_sound(server);
  if (s != NULL) {
    s->sfx = sfx_id;
    s->x = it->x;
    s->y = it->y;
  }
}

/* Unblock the map where the door stands */
static void open_door(SERVER *server, EVENT *e)
{
  ITEM *it = server->item + e->data[0].i;
  int block_x = e->data[1].i;
  int block_y = e->data[2].i;

  it->state = 1;
  it->state_data = server->map[block_x + block_y * server->map_w];
  server->map[block_x + block_y * server->map_w] = -1;
}

/* Block the map where the door stands */
static void close_door(SERVER *server, EVENT *e)
{
  ITEM *it = server->item + e->data[0].i;
  int block_x = e->data[1].i;
  int block_y = e->data[2].i;

  it->state = 0;
  server->map[block_x + block_y * server->map_w] = it->state_data;
}


/* Schedule the events of the door opening */
static void program_door_open(SERVER *server, ITEM *target,
			      NPC_INFO *target_npc,
			      int block_x, int block_y)
{
  int i, frames_left = 2;
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

  /* Events for the animation */
  for (i = 1; i < target_npc->n_stand/2; i++) {
    e = srv_allocate_event(server);
    if (e == NULL) {
      printf("ERROR: can't allocate event\n");
      exit(1);
      return;
    }
    e->frames_left = frames_left++;
    e->frames = -1;
    e->func = set_item_frame;
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


/* Schedule the star blink animation */
static void program_star_blink(SERVER *server, ITEM *star, NPC_INFO *npc)
{
  int i, frames_left = 2;
  EVENT *e;

  for (i = 1; i < npc->n_stand/2; i++) {
    e = srv_allocate_event(server);
    if (e == NULL) {
      printf("ERROR: can't allocate event\n");
      exit(1);
      return;
    }
    e->frames_left = frames_left;
    frames_left += 2;
    e->frames = -1;
    e->func = set_item_frame;
    e->data[0].i = star->id;
    e->data[1].i = i;
  }

  for (i = npc->n_stand/2 - 2; i >= 0; i--) {
    e = srv_allocate_event(server);
    if (e == NULL) {
      printf("ERROR: can't allocate event\n");
      exit(1);
      return;
    }
    e->frames_left = frames_left;
    frames_left += 2;
    e->frames = -1;
    e->func = set_item_frame;
    e->data[0].i = star->id;
    e->data[1].i = i;
  }
}

/* Schedule the events of the door closing */
static void program_door_close(SERVER *server, ITEM *target,
			       NPC_INFO *target_npc,
			       int block_x, int block_y, int open_interval)
{
  int i, frames_left, sfx_close_id;
  EVENT *e;

  frames_left = 2 + open_interval;

  /* Event for sound */
  if ((sfx_close_id = sfx_id(server, "closedoor")) >= 0) {
    e = srv_allocate_event(server);
    if (e == NULL) {
      printf("ERROR: can't allocate event\n");
      exit(1);
    }
    e->frames_left = frames_left++;
    e->frames = -1;
    e->func = play_sound;
    e->data[0].i = target->id;
    e->data[1].i = sfx_close_id;
  }

  /* Events for the animation */
  for (i = 1; i < target_npc->n_stand/2; i++) {
    e = srv_allocate_event(server);
    if (e == NULL) {
      printf("ERROR: can't allocate event\n");
      exit(1);
      return;
    }
    e->frames_left = frames_left++;
    e->frames = -1;
    e->func = set_item_frame;
    e->data[0].i = target->id;
    e->data[1].i = target_npc->n_stand/2 - i - 1;
  }

  /* Block event */
  e = srv_allocate_event(server);
  if (e == NULL) {
    printf("ERROR: can't allocate event\n");
    exit(1);
  }
  e->frames_left = frames_left;
  e->frames = -1;
  e->func = close_door;
  e->data[0].i = target->id;
  e->data[1].i = block_x;
  e->data[2].i = block_y;
}



/* This gets executed when a lever is touched */
void get_item(SERVER *server, ITEM *it, CLIENT *client)
{
  ITEM *target;
  int target_map_id, target_npc, target_id;
  int block_x, block_y;

  if (server->change_map || it->map_id < 0)
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
  if (target->state != 0)
    return;     /* Door is not closed */
  target->state = -1;

  if (target->target_x >= 0 && target->target_y >= 0) {
    block_x = target->target_x / server->tile_w;
    block_y = target->target_y / server->tile_h;
  } else {
    block_x = ((it->target_x + server->npc_info[target_npc].clip_w/2)
	       / server->tile_w);
    block_y = ((it->target_y + server->npc_info[target_npc].clip_h/2)
	       / server->tile_h);
  }

  if (block_x < 0 || block_y < 0
      || block_x >= server->map_w || block_y >= server->map_h)
    return;

  program_door_open(server, target,
		    server->npc_info + target_npc, block_x, block_y);
  program_star_blink(server, it, server->npc_info + it->npc);

  if (server->item[target_id].duration >= 0) {
    int duration = srv_sec2frame(server->item[target_id].duration);
    program_door_close(server, target, server->npc_info + target_npc,
		       block_x, block_y, duration);
  }
}
