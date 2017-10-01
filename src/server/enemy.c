/* enemy.c
 *
 * Copyright (C) 1998 Roberto A. G. Motta
 *                    Ricardo R. Massaro
 */

#include <stdio.h>
#include <malloc.h>

#include "server.h"
#include "addon.h"
#include "rect.h"


/* Dumb enemy movement function: do nothing.  This will be used for
 * enemies whose libraries can't be found. */
static void enemy_move_do_nothing(SERVER *server, ENEMY *e, NPC_INFO *npc)
{
}


/* Drain enemy's energy due to a hit */
void srv_hit_enemy(SERVER *server, ENEMY *e, int damage, int damage_type)
{
  e->energy -= (3 * damage * e->vulnerability) / 100;
  if (e->energy <= 0)
    e->destroy = 1;
}


/* Create a new enemy of the specified NPC type with the given
 * position and direction. Return NULL on error (e.g., out of
 * memory). */
ENEMY *create_enemy(SERVER *server, int npc, int x, int y, int dir)
{
  ENEMY *e;
  ADDON_LIBRARY *lib;

  e = srv_allocate_enemy(server, npc);
  if (e == NULL)
    return NULL;

  if (server->npc_info[npc].lib_name[0] != '\0')
    lib = load_addon_library(server->npc_info[npc].lib_name, "move_enemy");
  else
    lib = NULL;

  e->map_id = -1;
  e->target_id = -1;
  e->dir = dir;
  e->x = x;
  e->y = y;
  e->dx = e->dy = e->ax = 0;
  e->frame = 0;
  e->dead = 0;
  e->energy = 100;
  e->destroy = 0;
  e->create = 1;
  e->state = STATE_STAND;
  e->client_frame = 0;
  e->weapon = 0;
  e->level = 0;
  e->state_data = 0;

  if (lib != NULL)
    e->move = lib->function;
  else
    e->move = enemy_move_do_nothing;

  return e;
}

/* Return 1 if the server can create an enemy for the given MAP_ENEMY */
static int check_rect_free(SERVER *server, MAP_ENEMY *me, NPC_INFO *npc)
{
  CLIENT *c;
  RECT enemy, jack, tmp;

  if (is_map_enemy_blocked(server, me->x, me->y, npc->clip_w, npc->clip_h))
    return 0;

  enemy.x = me->x;
  enemy.y = me->y;
  enemy.w = npc->clip_w;
  enemy.h = npc->clip_h;
  for (c = server->client; c != NULL; c = c->next) {
    jack.x = c->jack.x;
    jack.y = c->jack.y;
    jack.w = server->npc_info[c->jack.npc].clip_w;
    jack.h = server->npc_info[c->jack.npc].clip_h;
    if (rect_interception(&jack, &enemy, &tmp))
      return 0;
  }
  return 1;
}

/* Create an enemy based on it's MAP_ENEMY */
void create_map_enemy(SERVER *server, MAP_ENEMY *me, int map_id)
{
  ENEMY *e;

  if (! check_rect_free(server, me, server->npc_info + me->npc))
    return;

  e = create_enemy(server, me->npc, me->x, me->y, me->dir);
  if (e == NULL) {
    me->id = -1;
    me->respawn_left = -1;
    DEBUG("OUT OF MEMORY for enemy\n");
    return;
  }
  e->map_id = map_id;
  e->vulnerability = me->vulnerability;
  e->level = me->intelligence;
  me->id = e->id;
  me->respawn_left = -1;
}

/* Create all enemies in a map */
void create_map_enemies(SERVER *server)
{
  int i;

  for (i = 0; i < server->n_map_enemies; i++)
    create_map_enemy(server, server->map_enemy + i, i);
}



static void reset_enemy(ENEMY *e)
{
  e->weapon_delay = ENEMY_WEAPON_DELAY;
}

/* Add a enemy in the active list, allocating memory of needed */
ENEMY *srv_allocate_enemy(SERVER *server, int npc)
{
  ENEMY *e;
  int i, n, old_n;

  for (i = 0; i < server->n_enemies; i++)
    if (server->enemy[i].active == 0) {
      server->enemy[i].npc = npc;
      server->enemy[i].active = 1;
      reset_enemy(server->enemy + i);
      return server->enemy + i;
    }

  if (server->enemy == NULL) {
    old_n = 0;
    n = 16;
    e = (ENEMY *) malloc(sizeof(ENEMY) * n);
  } else {
    old_n = server->n_enemies;
    n = server->n_enemies + 16;
    e = (ENEMY *) realloc(server->enemy, sizeof(ENEMY) * n);
  }
  if (e == NULL)
    return NULL;    /* Out of memory */
  server->n_enemies = n;
  server->enemy = e;
  for (i = old_n+1; i < server->n_enemies; i++) {
    server->enemy[i].id = i;
    server->enemy[i].active = 0;
  }
  server->enemy[old_n].npc = npc;
  server->enemy[old_n].id = old_n;
  server->enemy[old_n].active = 1;
  reset_enemy(server->enemy + old_n);
  return server->enemy + old_n;
}

/* Remove a enemy from the active list */
void srv_free_enemy(SERVER *server, ENEMY *e)
{
  e->active = 0;
}


/* Set the current enemy frame to send to the client */
void enemy_fix_frame(ENEMY *e, NPC_INFO *npc)
{
  switch (e->state) {
  case STATE_STAND:
    e->frame %= (npc->n_stand/2);
    e->client_frame = (e->dir == DIR_LEFT) ? (npc->n_stand / 2) : 0;
    e->client_frame += npc->s_stand + e->frame;
    break;

  case STATE_WALK:
    e->frame %= (npc->n_walk/2);
    e->client_frame = (e->dir == DIR_LEFT) ? (npc->n_walk / 2) : 0;
    e->client_frame += npc->s_walk + e->frame;
    break;

  case STATE_JUMP_START:
  case STATE_JUMP_END:
    e->frame %= (npc->n_jump/2);
    e->client_frame = (e->dir == DIR_LEFT) ? (npc->n_jump / 2) : 0;
    e->client_frame += npc->s_jump + e->frame;
    break;

  default:
    e->client_frame = 0;
  }
}
