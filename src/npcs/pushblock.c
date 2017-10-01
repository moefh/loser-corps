/* pushblock.c
 *
 * The `push block' enemy library, by Ricardo R. Massaro
 * (massaro@ime.usp.br).
 */

#include <stdio.h>

#include <../server/server.h>
#include <../server/rect.h>


/* Return 1 if the enemy (e) will block the enemy (j) if it (e) moves
 * by (move_dx, move_dy). */
static int enemy_blocks_enemy(SERVER *server, ENEMY *e,
			      int dx, int dy, ENEMY *j)
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


/* Same as do_move_enemy(), but must be called with either (move_dx =
 * 0 and ABS(move_dy) = 1) or (ABS(move_dx) = 1 and move_dy = 0).
 * This is ised by do_move_enemy(). */
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

  if (move_dx == 0 && move_dy == 0)
    return;

  for (c = server->client; c != NULL; c = c->next) {
    /* Move with the platform */
    if (c->jack.state == STATE_WALK || c->jack.state == STATE_STAND) {
      j_npc = server->npc_info + c->jack.npc;
      
      if (enemy_blocks_jack(server, e, 0, -(move_dy + 1), &c->jack)
	  && ! is_map_enemy_blocked(server,
				    c->jack.x + move_dx, c->jack.y + move_dy,
				    j_npc->clip_w, j_npc->clip_h)) {
	c->jack.x += move_dx;
	c->jack.y += move_dy;
      }
    }

    /* Hit if below */
    if (enemy_blocks_jack(server, e, 0, 1, &c->jack)) {
      NPC_INFO *e_npc, *j_npc;

      e_npc = server->npc_info + e->npc;
      j_npc = server->npc_info + c->jack.npc;
      srv_hit_jack(server, &c->jack, 80, 0);

      if (c->jack.x + j_npc->clip_w / 2 < e->x + e_npc->clip_w / 2) {
	c->jack.x = e->x - j_npc->clip_w;
	c->jack.dx = -10000;
      } else {
	c->jack.x = e->x + e_npc->clip_w;
	c->jack.dx = 10000;
      }
      if (is_map_blocked(server, c->jack.x, c->jack.y,
			 j_npc->clip_w, j_npc->clip_h))
	srv_kill_jack(server, &c->jack);
    }
  }
}



static void hit_jack(SERVER *server, ENEMY *e, CLIENT *c)
{
  NPC_INFO *e_npc, *j_npc;

  e_npc = server->npc_info + e->npc;
  j_npc = server->npc_info + c->jack.npc;
  srv_hit_jack(server, &c->jack, 80, 0);

  if (c->jack.x + j_npc->clip_w / 2 < e->x + e_npc->clip_w / 2) {
    c->jack.x = e->x - j_npc->clip_w;
    c->jack.dx = -10000;
  } else {
    c->jack.x = e->x + e_npc->clip_w;
    c->jack.dx = 10000;
  }
}

static void verify_hit_jacks(SERVER *server, ENEMY *e)
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
    if (rect_interception(&re, &rc, &tmp))
      hit_jack(server, e, c);
  }
}


/* Add the attrict to the enemy speed */
static int add_attrict(int speed, int attrict)
{
  int s = 1;

  if (speed < 0) {
    speed = -speed;
    s = -1;
  }

  speed -= attrict;
  if (speed < 0)
    return 0;

  return ((s < 0) ? -speed : speed);
}


static void do_movement(ENEMY *e, SERVER *server, NPC_INFO *npc)
{
  int flags, move_dx, move_dy;

  /* Verify the ground */
  calc_jack_movement(server, 0, e->x, e->y, npc->clip_w, npc->clip_h,
		     0, 1, &move_dx, &move_dy);
  if (move_dy > 0)
    e->dy += DEC_JUMP_SPEED;

  flags = calc_jack_movement(server, 1, e->x, e->y, npc->clip_w, npc->clip_h,
			     e->dx / 1000, e->dy / 1000, 
			     &move_dx, &move_dy);

  if (flags & CM_Y_CLIPPED) {        /* Hit the ground */
    e->state = STATE_STAND;
    e->dy = 0;
    srv_do_tremor(server, e->x + npc->clip_w / 2, e->y + npc->clip_h / 2,
		  24, 3);
  }

  if (flags & CM_X_CLIPPED)
    e->dx = 0;
  e->dx = add_attrict(e->dx, 300);

  do_move_enemy(server, e, adjust_jacks);
}


static void do_push_by_jack(SERVER *server, ENEMY *e, NPC_INFO *e_npc,
			    CLIENT *c, NPC_INFO *j_npc)
{
  if (c->jack.dx < 0 && enemy_blocks_jack(server, e, 1, 0, &c->jack))
    e->dx -= 800;
  else if (c->jack.dx > 0 && enemy_blocks_jack(server, e, -1, 0, &c->jack))
    e->dx += 800;
}

static void do_push(SERVER *server, ENEMY *e, NPC_INFO *npc)
{
  CLIENT *c;

  for (c = server->client; c != NULL; c = c->next)
    do_push_by_jack(server, e, npc, c, server->npc_info + c->jack.npc);
}


static void kill_npcs(SERVER *server, ENEMY *e)
{
  CLIENT *c;
  ENEMY *en;
  int i;

  for (c = server->client; c != NULL; c = c->next)
    if (enemy_blocks_jack(server, e, 0, 0, &c->jack))
      c->jack.energy = c->jack.f_energy_tanks = 0;

  for (i = 0; i < server->n_enemies; i++) {
    en = server->enemy + i;
    if (en != e && en->active && enemy_blocks_enemy(server, e, 0, 0, en)) {
      en->energy = 0;
      en->destroy = 1;
    }
  }
}

/* Do push block movement */
void move_enemy(SERVER *server, ENEMY *e, NPC_INFO *npc)
{
  e->frame++;
  if (e->vulnerability != 0)
    e->vulnerability = 0;

  do_movement(e, server, npc);
  if (e->dy > 0)
    verify_hit_jacks(server, e);
  do_push(server, e, npc);
  kill_npcs(server, e);
}
