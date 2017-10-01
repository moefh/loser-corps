/* calc.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 *
 * Functions to make the game calculations.
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <time.h>

#include "server.h"
#include "game.h"
#include "rect.h"
#include "connect.h"


#ifdef _WINDOWS
static unsigned long int get_past_time(void)
{
  static struct _timeb cur;
  static int done = 0;
  unsigned long int msec;
  struct _timeb last;

  if (! done) {
    done = 1;
    _ftime(&cur);
    return 0;
  }
  last = cur;
  _ftime(&cur);
  msec = cur.millitm - last.millitm;
  msec += (cur.time - last.time) * 1000;
  return msec;
}
#else /* _WINDOWS */
static unsigned long int get_past_time(void)
{
  static struct timeval cur;
  static int done = 0;
  unsigned long int msec;
  struct timeval last;

  if (! done) {
    done = 1;
    gettimeofday(&cur, NULL);
    return 0;
  }
  last = cur;
  gettimeofday(&cur, NULL);
  msec = (cur.tv_usec - last.tv_usec) / 1000;
  msec += (cur.tv_sec - last.tv_sec) * 1000;
  return msec;
}
#endif /* _WINDOWS */


static unsigned int estimated_fps = 30;

/* Convert a time interval in seconds to game frames */
int srv_sec2frame(int sec)
{
 return sec * estimated_fps;
}


static WEAPON *create_new_weapon(SERVER *server, int n_npc, int level,
				 int x, int y, int dx, int dy, int dx2,
				 int dir, int frame)
{
  WEAPON *w;
  NPC_INFO *npc;

  w = srv_allocate_weapon(server, n_npc);
  if (w == NULL) {
    DEBUG("Out of memory for weapon\n");
    return NULL;
  }
  w->level = level;
  w->x = x;
  w->y = y;
  w->dx = dx;
  w->dy = dy;
  w->dx2 = dx2;
  w->dir = dir;
  w->frame = frame;
  w->create = 1;
  w->destroy = 0;

  npc = server->npc_info + n_npc;
  if (is_map_blocked(server, w->x, w->y, npc->clip_w, npc->clip_h)) {
    w->x += dx;
    w->y += dy;
    w->destroy = 1;
  }
  return w;
}

/* Create a weapon for an enemy */
static void create_enemy_weapon(SERVER *server, ENEMY *e)
{
  NPC_INFO *enemy_npc, *weapon_npc;
  WEAPON *w;
  int x, y, dx, dy, dx2, frame;
  int weapon_npc_num;

  if (server->quit || server->reset || e->weapon <= 0)
    return;

  enemy_npc = server->npc_info + e->npc;
  weapon_npc_num = enemy_npc->weapon_npc[0];   /* FIXME: weapon power */
  weapon_npc = server->npc_info + weapon_npc_num;
  e->weapon = 0;

  if ((dx2 = weapon_npc->accel) <= 0)
    dx = weapon_npc->speed;
  else
    dx = 1;
  if (e->dir == DIR_LEFT) {
    dx = -dx;
    dx2 = -dx2;
    x = e->x - weapon_npc->clip_w - dx;
    frame = 1;
  } else {
    x = e->x + enemy_npc->clip_w - dx;
    frame = 0;
  }
  dy = 0;
  y = e->y + enemy_npc->weapon_y[0] - weapon_npc->clip_h / 2;

  w = create_new_weapon(server, weapon_npc_num, 0, x,y,
			dx, dy, dx2, e->dir, frame);
  if (w == NULL)
    return;

  w->creator_npc = e->npc;
  w->creator_id = e->id;
  w->creator_team = -1;
  w->move = srv_missile_move;
}

/* Create a weapon for a client */
static void create_client_weapon(SERVER *server, CLIENT *client)
{
  NPC_INFO *jack_npc, *weapon_npc;
  WEAPON *w;
  int n_weapon_npc, x, y, dx, dy, dx2, frame;

  if (server->quit || server->reset || client->jack.weapon <= 0)
    return;

  n_weapon_npc = server->npc_info[client->jack.npc].weapon_npc[client->jack.power_level];
  jack_npc = server->npc_info + client->jack.npc;
  weapon_npc = server->npc_info + n_weapon_npc;
  client->jack.weapon = 0;
  client->jack.state_flags |= NPC_STATE_SHOOT;   /* Set state to "shooting" */
  client->jack.shoot_keep = 4;

  if ((dx2 = weapon_npc->accel) <= 0)
    dx = weapon_npc->speed;
  else
    dx = 1;

  if (client->jack.dir == DIR_LEFT) {
    dx = -dx;
    dx2 = -dx2;
    x = client->jack.x - weapon_npc->clip_w - dx;
    frame = 1;
  } else {
    x = client->jack.x + jack_npc->clip_w - dx;
    frame = 0;
  }
  y = (client->jack.y + jack_npc->weapon_y[client->jack.power_level]
       - weapon_npc->clip_h / 2);
  dy = 0;
  w = create_new_weapon(server, n_weapon_npc, client->jack.power_level, x, y,
			dx, dy, dx2, client->jack.dir, frame);
  if (w == NULL)
    return;

  w->creator_npc = client->jack.npc;
  w->creator_id = client->jack.id;
  w->creator_team = client->jack.team;
  w->move = srv_missile_move;
}


/* Calculate an item */
static void calc_item(SERVER *server, ITEM *it, NPC_INFO *item_npc)
{
  CLIENT *client;
  NPC_INFO *jack_npc;
  RECT item, jack, tmp;

  item.x = it->x;
  item.y = it->y;
  item.w = item_npc->clip_w;
  item.h = item_npc->clip_h;

  for (client = server->client; client != NULL; client = client->next) {
    jack_npc = server->npc_info + client->jack.npc;
    jack.x = client->jack.x;
    jack.y = client->jack.y;
    jack.w = jack_npc->clip_w;
    jack.h = jack_npc->clip_h;
    if (rect_interception(&item, &jack, &tmp)) {
      it->pressed = client->jack.using_item;
      it->get_item(server, it, client);
      if (it->destroy && it->map_id >= 0) {
	server->map_item[it->map_id].id = -1;
	server->map_item[it->map_id].respawn_left =
	  srv_sec2frame(server->map_item[it->map_id].respawn_time);
	return;
      }
    }
  }
}


/* Calculate client movement */
static void calc_client(SERVER *server, CLIENT *client)
{
  int i;

  if (client->state != CLST_ALIVE || server->quit || server->reset)
    return;

  client->jack.sound = -1;

  if (client->jack.dead) {
    client->jack.dead = 0;
    srv_get_jack_start_pos(server, &client->jack);
    client->jack.energy = 100;
    client->jack.score--;
    client->jack.changed = 1;
    client->jack.invincible = -1;
    client->jack.power_level = 0;
    client->jack.under_water = 0;
  } else {
    if (client->jack.under_water > 0)
      client->jack.under_water--;
    if (client->jack.invisible >= 0) {
      client->jack.invisible--;
      if (client->jack.invisible < 0)
	client->jack.start_invisible = -1;    /* Terminate invisibility */
    }
    if (client->jack.invincible >= 0) {
      client->jack.invincible--;
      if (client->jack.invincible < 0)
	client->jack.start_invincible = -1;    /* Terminate invisibility */
    }

    jack_move(&client->jack, server->npc_info + client->jack.npc, server);
    jack_input(&client->jack, server);
  }

  create_client_weapon(server, client);
  jack_fix_frame(&client->jack, server->npc_info + client->jack.npc);
  
  if (client->jack.key_trans[KEY_USE] > 0)
    client->jack.using_item = 1;
  else
    client->jack.using_item = 0;

  for (i = 0; i < NUM_KEYS; i++)
    if (client->jack.key_trans[i] > 0)
      client->jack.key_trans[i] = 0;
}

static void get_estimated_fps(void)
{
  static int cur_frame = 0;     /* Current frame % 100 */

  if (cur_frame == 0) {
    unsigned long int msec;

    /* Time elapsed since the last call to get_past_time(): */
    msec = get_past_time();
    if (msec == 0)
      estimated_fps = 30;        /* First frame: no estimative */
    else
      estimated_fps = 100000 / msec;
    if (estimated_fps < 30)
      estimated_fps = 30;
  }
  cur_frame++;
  if (cur_frame >= 100)
    cur_frame = 0;
}

/* Destroy all non-jack NPCs, read the map, send messages to all
 * clients about the map change, and then create all non-jack NPCs for
 * the new map. */
static void handle_map_change(SERVER *server)
{
  WEAPON *w;
  ENEMY *e;
  ITEM *it;
  int i;
  CLIENT *client;
  NETMSG msg;

  switch (server->change_map) {
  case 2:    /* Remove objects and events */
    for (i = 0, w = server->weapon; i < server->n_weapons; i++, w++)
      if (w->active && ! w->destroy)
	w->destroy = 1;
    for (i = 0, e = server->enemy; i < server->n_enemies; i++, e++)
      if (e->active && ! e->destroy)
	e->destroy = 1;
    for (i = 0, it = server->item; i < server->n_items; i++, it++)
      if (it->active && ! it->destroy)
	it->destroy = 1;
    for (i = 0; i < server->n_events; i++)
      server->event[i].active = 0;
    server->change_map = 1;
    break;

  case 1:    /* Read map and create objects */
    /* Get the new map info */
    server_copy_map_info(server, &server->new_map_info);

    DEBUG("Changing to map `%s'\n", server->map_file);

    /* Get new start positions for the jacks */
    for (client = server->client; client != NULL; client = client->next)
      srv_get_jack_start_pos(server, &client->jack);

    /* Inform the clients about the new map */
    msg.string.type = MSGT_STRING;
    msg.string.subtype = MSGSTR_SETMAP;
    msg.string.data = 0;
    strcpy(msg.string.str, server->map_file);
    for (client = server->client; client != NULL; client = client->next)
      send_message(client->fd, &msg, 1);
    create_map_enemies(server);
    create_map_items(server);
    server->change_map = 0;
    break;
  }
}


/* Do all the calculations for a frame of the game. */
void srv_calc_frame(SERVER *server)
{
  WEAPON *w;
  ENEMY *e;
  ITEM *it;
  CLIENT *client;
  int i;

  if (server->change_map > 0)
    handle_map_change(server);

  get_estimated_fps();

  /* Clear client flags */
  for (client = server->client; client != NULL; client = client->next) {
    client->jack.start_invisible = client->jack.start_invincible = 0;
    client->jack.item_flags = 0;
  }

  /* Calculate clients */
  for (client = server->client; client != NULL; client = client->next)
    calc_client(server, client);

  /* Calculate weapons */
  for (i = 0; i < server->n_weapons; i++) {
    w = server->weapon + i;
    if (w->active && ! w->destroy)
      w->move(server, w, server->npc_info + w->npc);
  }

  if (! server->change_map) {
    /* Calculate enemies */
    /* Active enemies */
    for (i = 0; i < server->n_enemies; i++) {
      e = server->enemy + i;
      if (e->active) {
	create_enemy_weapon(server, e);
	e->move(server, e, server->npc_info + e->npc);
	e = server->enemy + i;     /* server->enemy could have changed */
	enemy_fix_frame(e, server->npc_info + e->npc);
      }
    }
    /* Non-active enemies (respawn) */
    for (i = 0; i < server->n_map_enemies; i++)
      if (server->map_enemy[i].respawn_left >= 0) {
	if (server->map_enemy[i].respawn_left == 0)
	  create_map_enemy(server, server->map_enemy + i, i);
	else
	  server->map_enemy[i].respawn_left--;
      }
    
    /* Calculate items */
    /* Active items */
    for (i = 0; i < server->n_items; i++) {
      it = server->item + i;
      if (it->active && ! it->destroy)
	calc_item(server, it, server->npc_info + it->npc);
    }
    /* Non-active items (respawn) */
    for (i = 0; i < server->n_map_items; i++)
      if (server->map_item[i].respawn_left >= 0) {
	if (server->map_item[i].respawn_left == 0)
	  create_item(server, server->map_item + i, i);
	else
	  server->map_item[i].respawn_left--;
      }
    
    /* Execute events */
    srv_process_events(server);
  }

  for (client = server->client; client != NULL; client = client->next)
    client->jack.using_item = 0;
}
