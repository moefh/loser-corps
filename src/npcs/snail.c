/* snail.c
 *
 * The `snail' enemy library, by Ricardo Massaro (massaro@ime.usp.br).
 *
 * This was adapted from the ghost enemy code, written originally by
 * Roberto Motta (ragm@linux.ime.usp.br).
 */


#include <stdio.h>

#include <../server/server.h>
#include <../server/rect.h>

/* The size of the field of vision: */
#define X_VISION      (10 * (server)->tile_w)
#define Y_VISION      ((server)->tile_w)


static int is_in_field(SERVER *server,
		       int jack_x, int jack_y, int enemy_x, int enemy_y)
{
  return(jack_x <= enemy_x + X_VISION &&
	 jack_x >= enemy_x - X_VISION &&
	 jack_y <= enemy_y + Y_VISION &&
	 jack_y >= enemy_y - Y_VISION);
}


static CLIENT *verify_target(SERVER *server, ENEMY *e, int target)
{
  CLIENT *c;

  for (c = server->client; c != NULL && c->jack.id != target; c= c-> next)
    ;
  if (c != NULL && c->jack.invisible < 0
      && is_in_field(server, c->jack.x,c->jack.y,e->x,e->y))
    return c;
  return NULL;
}

/* Verify presence of targets */
static int search_target(SERVER *server, ENEMY *e)
{
  CLIENT *c;

  for(c = server->client; c != NULL; c = c->next)
    if(c->jack.invisible < 0
       && is_in_field(server, c->jack.x,c->jack.y,e->x,e->y))
      return(c->jack.id);
  return(-1);
}

static void do_movement(ENEMY *e, SERVER *server, NPC_INFO *npc)
{
  int flags, move_dx, move_dy;

  if (e->state == STATE_WALK || e->state == STATE_STAND) 
  {                          /* Check the floor under enemy */
    calc_enemy_movement(server, e, 0, e->x, e->y, npc->clip_w, npc->clip_h,
			0, 1, &move_dx, &move_dy);
    if (move_dy > 0)
      e->state = STATE_JUMP_END;    /* Start falling */
  }
  else
    e->dy += DEC_JUMP_SPEED;

  {
    int s;

    if (e->dx < 0) {
      e->dx = -e->dx;
      s = -1;
    } else
      s = 1;
    e->dx -= 1000;
    if (e->dx < 0)
      e->dx = 0;
    else
      e->dx *= s;
  }

  flags = calc_enemy_movement(server, e, 1,
			      e->x, e->y, npc->clip_w, npc->clip_h,
			      e->dx / 1000, e->dy / 1000, 
			      &move_dx, &move_dy);

  if (flags & CM_Y_CLIPPED) {
    if (e->dy > 0) {
      e->state = STATE_STAND;
      e->dy = 0;
    } else
      e->dy = -e->dy;
  }

  if ((flags & CM_X_CLIPPED) && e->state == STATE_WALK)
    e->dx *= -1;

  e->x += move_dx;
  e->y += move_dy;

  switch (e->state) {
  case STATE_JUMP_START:
    if (e->dy > 0)
      e->state = STATE_JUMP_END;
    break;

  case STATE_WALK:
    if (e->dx > 0)
      e->dir = DIR_RIGHT;
    else
      e->dir = DIR_LEFT;
  }
}



static void suck_energy(SERVER *server, ENEMY *e)
{
  CLIENT *c;
  RECT re, rc, tmp;

  re.x = e->x;
  re.y = e->y;
  re.w = server->npc_info[e->npc].clip_w;
  re.h = server->npc_info[e->npc].clip_h;

  for (c = server->client; c != NULL; c = c->next) {
    if (c->jack.invincible >= 0)
      continue;
    rc.x = c->jack.x;
    rc.y = c->jack.y;
    rc.w = server->npc_info[c->jack.npc].clip_w;
    rc.h = server->npc_info[c->jack.npc].clip_h;
    if (rect_interception(&re, &rc, &tmp)) {
      if ((--c->jack.energy_delay) <= 0) {
	e->energy += 9;
        if (e->energy > 100)
	  e->energy = 100;
	srv_hit_jack(server, &c->jack, 1, 0);
	c->jack.energy_delay = 10;
      }
    }
  }
}

/* Do cannon enemy movement */
void move_enemy(SERVER *server, ENEMY *e, NPC_INFO *npc)
{
  CLIENT *c;
  /* int move_dx, move_dy; */

  if (server->change_map)
    return;
  e->frame++;

  suck_energy(server, e);
  do_movement(e,server,npc);

  if (e->target_id < 0)
    e->target_id = search_target(server, e);
  c = verify_target(server, e, e->target_id);
  if (c != NULL) {
    if (e->x < c->jack.x)
      e->dir = DIR_RIGHT;
    else if (e->x+npc->clip_w > c->jack.x+server->npc_info[c->jack.npc].clip_w)
      e->dir = DIR_LEFT;
  }
  
  if (e->state == STATE_STAND || e->state == STATE_WALK) {
    e->state = STATE_WALK;

    if (e->dir == DIR_RIGHT)
      e->dx = 2000;
    else
      e->dx = -2000;
  }
}
