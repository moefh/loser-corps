/* hplatform.c
 *
 * The `horizontal platform' enemy library, by Ricardo Massaro
 * (massaro@ime.usp.br).
 */

#include <stdio.h>

#include <../server/server.h>
#include <../server/rect.h>

#ifndef ABS
#define ABS(x)   (((x) < 0) ? -(x) : (x))
#endif


/* Return 1 if the enemy will block the jack if it (the enemy) moves
 * by (move_dx, move_dy). */
static int enemy_blocks_jack(SERVER *server, ENEMY *e, int dx, int dy, JACK *j)
{
  RECT er, jr, tmp;
  NPC_INFO *e_npc, *j_npc;

  e_npc = server->npc_info + e->npc;
  j_npc = server->npc_info + j->npc;

  er.x = e->x + dx;
  er.y = e->y + dy;
  er.w = e_npc->clip_w;
  er.h = e_npc->clip_h;

  jr.x = j->x;
  jr.y = j->y;
  jr.w = j_npc->clip_w;
  jr.h = j_npc->clip_h;

  return rect_interception(&er, &jr, &tmp);
}

/* Push jacks by (move_dx, move_dy) before the enemy moves.  Return 1
 * if can't push some jack (in this case, the enemy can't move). */
static int push_jacks(SERVER *server, ENEMY *e, int move_dx, int move_dy)
{
  CLIENT *c;
  NPC_INFO *j_npc;

  /* First, verify if we can move without smashing any jack */
  for (c = server->client; c != NULL; c = c->next) {
    j_npc = server->npc_info + c->jack.npc;
    if (enemy_blocks_jack(server, e, move_dx, move_dy, &c->jack)
	&& is_map_enemy_blocked(server,
				c->jack.x + move_dx, c->jack.y + move_dy,
				j_npc->clip_w, j_npc->clip_h)) {
      return 1;
    }
  }

  /* Now, push all jacks hit by the enemy */
  for (c = server->client; c != NULL; c = c->next) {
    if (enemy_blocks_jack(server, e, move_dx, move_dy, &c->jack)) {
      c->jack.x += move_dx;
      c->jack.y += move_dy;
    }
  }

  return 0;
}

static int do_move_step(SERVER *server, ENEMY *e, int move_dx, int move_dy,
			void (*adjust_jacks_func)(SERVER *, ENEMY *, int, int))
{
  int flags;
  NPC_INFO *npc;

  npc = server->npc_info + e->npc;

  if (push_jacks(server, e, move_dx, move_dy))
    return 1;
  flags = calc_jack_movement(server, 1, e->x, e->y,
			     npc->clip_w, npc->clip_h,
			     move_dx, move_dy, &move_dx, &move_dy);
  e->x += move_dx;
  e->y += move_dy;

  if (adjust_jacks_func != NULL)
    adjust_jacks_func(server, e, move_dx, move_dy);

  return !!flags;
}

/* Move the enemy by (e->dx / 1000, e->dy / 1000).  Return 1 if it
 * can't be moved.  Jacks are pushed away to make room for the
 * movement.  If adjust_jacks_func is not null, it will be called for
 * each pixel movement.  It is useful for adjusting the jacks (e.g.,
 * when a jack is standing on a platform) */
static int do_move_enemy(SERVER *server, ENEMY *e,
			 void (*adjust_jacks_func)(SERVER *,ENEMY *,int,int))
{
  int move_dx, move_dy;
  int c, steps, unit;

  move_dx = e->dx / 1000;
  move_dy = e->dy / 1000;

  if (move_dx < 0)
    unit = -1;
  else
    unit = 1;
  steps = (unit > 0) ? move_dx : -move_dx;
  for (c = 0; c < steps; c++)
    if (do_move_step(server, e, unit, 0, adjust_jacks_func))
      break;
  move_dx = (unit > 0) ? c : -c;

  if (move_dy < 0)
    unit = -1;
  else
    unit = 1;
  steps = (unit > 0) ? move_dy : -move_dy;
  for (c = 0; c < steps; c++)
    if (do_move_step(server, e, 0, unit, adjust_jacks_func)) {
      break;
    }
  move_dy = (unit > 0) ? c : -c;

  if (move_dx != e->dx / 1000 || move_dy != e->dy / 1000)
    return 1;
  return 0;
}



/* Move the jacks standing ow walking on the platform along with the
 * platform. */
static void adjust_jacks(SERVER *server, ENEMY *e, int move_dx, int move_dy)
{
  NPC_INFO *j_npc;
  CLIENT *c;

  for (c = server->client; c != NULL; c = c->next) {
    if (c->jack.state != STATE_WALK && c->jack.state != STATE_STAND)
      continue;
    j_npc = server->npc_info + c->jack.npc;

    if (enemy_blocks_jack(server, e, 0, -(move_dy + 1), &c->jack)
	&& ! is_map_enemy_blocked(server,
				  c->jack.x + move_dx, c->jack.y + move_dy,
				  j_npc->clip_w, j_npc->clip_h)) {
      c->jack.x += move_dx;
      c->jack.y += move_dy;
    }
  }
}

/* Do platform enemy movement */
void move_enemy(SERVER *server, ENEMY *e, NPC_INFO *npc)
{
  if (e->dx == 0) {      /* Start movement */
    e->state_data = e->x;
    if (e->dir < 0)
      e->dx = -3000;
    else
      e->dx = 3000;
    e->vulnerability = 0;
  }
  e->frame++;

  if (do_move_enemy(server, e, adjust_jacks)) {
    e->dx = -e->dx;
    e->dy = -e->dy;
  } else if (e->level != 0
	     && (ABS(e->state_data - (e->x + e->dx / 1000))
		 > e->level * server->tile_w)) {
    e->dx = -e->dx;
    e->dy = -e->dy;
  }
}
