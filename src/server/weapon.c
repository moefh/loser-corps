/* weapon.c */

#include <stdio.h>
#include <malloc.h>

#include "server.h"
#include "rect.h"

static int weapon_destruction[WEAPON_LEVELS] = { 5, 7, 9, 11 };



/* Add a weapon in the active list, allocating memory of needed */
WEAPON *srv_allocate_weapon(SERVER *server, int npc)
{
  WEAPON *w;
  int i, n, old_n;

  for (i = 0; i < server->n_weapons; i++)
    if (server->weapon[i].active == 0) {
      server->weapon[i].active = 1;
      server->weapon[i].npc = npc;
      return server->weapon + i;
    }

  if (server->weapon == NULL) {
    old_n = 0;
    n = 16;
    w = (WEAPON *) malloc(sizeof(WEAPON) * n);
  } else {
    old_n = server->n_weapons;
    n = server->n_weapons + 16;
    w = (WEAPON *) realloc(server->weapon, sizeof(WEAPON) * n);
  }
  if (w == NULL)
    return NULL;    /* Out of memory */
  server->n_weapons = n;
  server->weapon = w;
  for (i = old_n+1; i < server->n_weapons; i++) {
    server->weapon[i].id = i;
    server->weapon[i].active = 0;
  }
  server->weapon[old_n].npc = npc;
  server->weapon[old_n].id = old_n;
  server->weapon[old_n].active = 1;
  return server->weapon + old_n;
}

/* Remove a weapon from the active list */
void srv_free_weapon(SERVER *server, WEAPON *w)
{
  w->active = 0;
}

/* Inform the weapon owner that the weapon hit something.
 * If it hit a jack, `c' points to the client's jack. If it
 * hit an enemy, `c' is NULL and `e' points to the enemy.
 */
static void inform_weapon_owner(SERVER *server, WEAPON *w, CLIENT *c, ENEMY *e)
{
  CLIENT *creator;
  int npc, id, energy, score, dead;

  if (! IS_NPC_JACK(server, w->creator_npc))
    return;    /* Only jacks want to know about weapon hits */

  if (c != NULL) {
    npc = c->jack.npc;
    id = c->jack.id;
    energy = c->jack.energy;
    score = c->jack.score;
    dead = c->jack.dead;
  } else {
    npc = e->npc;
    id = e->id;
    energy = e->energy;
    score = 0;
    /* `dead' may be 0 if you don't want to count enemies in the score */
    dead = e->destroy;
  }

  creator = srv_find_client_id(server->client, w->creator_id);
  if (creator != NULL) {
    creator->jack.hit_enemy = 1;
    creator->jack.enemy.id = id;
    creator->jack.enemy.npc = npc;
    creator->jack.enemy.energy = energy;
    creator->jack.enemy.score = score;
    if (dead) {
      creator->jack.score++;
      creator->jack.changed = 1;
    }
  }
}

/* Verify a weapon collosion with the jacks */
static int verify_jacks(SERVER *server, WEAPON *w, RECT *wr)
{
  RECT jr, tmp;
  int flags = 0;
  CLIENT *c;
  NPC_INFO *jack_npc;

  jack_npc = server->npc_info;

  for (c = server->client; c != NULL; c = c->next) {
    if (IS_NPC_JACK(server, w->creator_npc)
	&& (w->creator_id == c->jack.id
	    || (w->creator_team >= 0 && w->creator_team == c->jack.team)))
      continue;       /* Don't hit the missile creator or team partner */
    
    jr.x = c->jack.x;
    jr.y = c->jack.y;
    jr.w = jack_npc[c->jack.npc].clip_w;
    jr.h = jack_npc[c->jack.npc].clip_h;
    if (rect_interception(wr, &jr, &tmp)) {
      if (w->level >= WEAPON_LEVELS)
	w->level = WEAPON_LEVELS - 1;
      srv_hit_jack(server, &c->jack, weapon_destruction[w->level], 0);
      if (w->dir == DIR_LEFT)
	c->jack.dx = -MAX_WALK_SPEED;
      else
	c->jack.dx = MAX_WALK_SPEED;
      inform_weapon_owner(server, w, c, NULL);
      flags = CM_X_CLIPPED;
    }
  }
  return flags;
}


/* Verify a weapon collosion with the enemies */
static int verify_enemies(SERVER *server, WEAPON *w, RECT *wr)
{
  int i, flags = 0;
  ENEMY *e;
  NPC_INFO *enemy_npc;
  RECT er, tmp;

  for (i = server->n_enemies - 1; i >= 0; i--) {
    e = server->enemy + i;
    if (! e->active)
      continue;

#if 0
    if (w->creator_npc == e->npc && w->creator_id == e->id)
      continue;
#else
    if (IS_NPC_ENEMY(server, w->creator_npc))
      continue;
#endif

    enemy_npc = server->npc_info + e->npc;
    er.x = e->x;
    er.y = e->y;
    er.w = enemy_npc->clip_w;
    er.h = enemy_npc->clip_h;
    if (rect_interception(wr, &er, &tmp)) {    /* Weapon hit an enemy */
      if (w->level >= WEAPON_LEVELS)
	w->level = WEAPON_LEVELS - 1;
      srv_hit_enemy(server, e, weapon_destruction[w->level], 0);
      if (e->destroy && e->map_id >= 0) {
	server->map_enemy[e->map_id].id = -1;
	server->map_enemy[e->map_id].x = e->x;
	server->map_enemy[e->map_id].y = e->y;
	server->map_enemy[e->map_id].dir = e->dir;
	server->map_enemy[e->map_id].respawn_left =
	  srv_sec2frame(server->map_enemy[e->map_id].respawn_time);
      }
      if (enemy_npc->show_hit)
	inform_weapon_owner(server, w, NULL, e);
      flags = CM_X_CLIPPED;
    }
  }
  return flags;
}


/* Do a missile movement */
void srv_missile_move(SERVER *server, WEAPON *w, NPC_INFO *npc)
{
  int flags;
  RECT mr;

  if (w->dx < 0)
    w->frame = 1;
  else
    w->frame = 0;

  /* Set missile rect */
  mr.x = w->x + w->dx;
  mr.y = w->y + w->dy;
  mr.w = npc->clip_w;
  mr.h = npc->clip_h;

  /* Verify jacks */
  flags = verify_jacks(server, w, &mr);

  /* Verify enemies */
  if (flags == 0)
    flags = verify_enemies(server, w, &mr);

  /* Verify map */
  if (flags == 0)
    flags = is_map_blocked(server, w->x, w->y, npc->clip_w, npc->clip_h);

  /* Move */
  if (w->dx2 != 0) {
    int s = 0;

    w->dx += w->dx2;
    if (w->dx < 0) {
      w->dx = -w->dx;
      s = 1;
    }
    if (w->dx > server->npc_info[w->npc].speed) {
      w->dx = server->npc_info[w->npc].speed;
      w->dx2 = 0;    /* Stop accel */
    }
    if (s)
      w->dx = -w->dx;
  }
  w->x += w->dx;
  w->y += w->dy;
  if (flags != 0)
    w->destroy = 1;
}
