/* smart.c
 *
 * This was originally written for the server by Guto
 * (ragm@linux.ime.usp.br), and later adapted to work as an enemy
 * library by Moe (massaro@ime.usp.br).
 */

#include <stdio.h>

#include <../server/server.h>


#define WEAPON_DELAY  10
#define JUMP_SPEED    ((7 * MAX_JUMP_SPEED) / 4)

/* The size of the field of vision: */
#define X_VISION      (5 * (server)->tile_w)
#define Y_VISION      (3 * (server)->tile_w)



/* Shoot */
static void enemy_shoot(ENEMY *e)
{
  if(e->weapon_delay <= 0) {
    e->weapon = 1;
    e->weapon_delay = WEAPON_DELAY;
  } else
    e->weapon_delay--;
}

static int is_in_field(SERVER *server,
		       int jack_x, int jack_y, int enemy_x, int enemy_y)
{
  return(jack_x <= enemy_x + X_VISION &&
	 jack_x >= enemy_x - X_VISION &&
	 jack_y <= enemy_y + Y_VISION &&
	 jack_y >= enemy_y - Y_VISION);
}


static int verify_target(SERVER *server, ENEMY *e, int target)
{
  CLIENT *c;

  for(c = server->client; c != NULL && c->jack.id != target; c= c-> next);
  if(c != NULL && c->jack.invisible < 0
     && is_in_field(server, c->jack.x,c->jack.y,e->x,e->y))
    return target;
  return -1;
}

/* Verify presence of targets */
static int search_target(SERVER *server, ENEMY *e)
{
  CLIENT *c;

  for(c = server->client; c != NULL; c = c->next)
    if(c->jack.invisible < 0
       && is_in_field(server, c->jack.x,c->jack.y,e->x,e->y))
      return c->jack.id;
  return -1;
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

/* Limit the enemy speed */
static int limit_speed(int speed, int max)
{
  int s = 1;

  if (speed < 0) {
    speed = -speed;
    s = -1;
  }

  if (speed > max)
    speed = max;

  return ((s < 0) ? -speed : speed);
}


static void smartenemy_movement_calc(ENEMY *e, SERVER *server, NPC_INFO *npc)
{
  int flags, move_dx, move_dy;

  if (e->state == STATE_WALK || e->state == STATE_STAND) {
    calc_movement(server, 0, e->x, e->y, npc->clip_w, npc->clip_h,
		  0, 1, &move_dx, &move_dy);
    if (move_dy > 0)
      e->state = STATE_JUMP_END;
  } else
    e->dy += DEC_JUMP_SPEED;

  e->dx += e->ax;
  e->dx = add_attrict(e->dx, DEC_WALK_SPEED);

  e->dx = limit_speed(e->dx, MAX_WALK_SPEED);
  e->dy = limit_speed(e->dy, MAX_JUMP_SPEED);

  if (e->state == STATE_WALK && e->dx == 0)
    e->state = STATE_STAND;

  flags = calc_movement(server, 1, e->x, e->y, npc->clip_w, npc->clip_h,
			e->dx / 1000, e->dy / 1000, &move_dx, &move_dy);

  if (flags & CM_Y_CLIPPED) {
    if (e->dy > 0) {           /* Hit the ground */
      e->state = STATE_STAND;
      e->dy = 0;
    } else                     /* Hit the ceiling */
      e->dy = -e->dy;
  }

  if (e->state == STATE_WALK && (flags & CM_X_CLIPPED))
    e->dx = -e->dx;

  e->x += move_dx;
  e->y += move_dy;

  switch(e->state) {
  case STATE_JUMP_START:
    if (e->dy > 0)
      e->state = STATE_JUMP_END;
    break;

  case STATE_WALK:
    e->dir = ((e->dx > 0) ? DIR_RIGHT : DIR_LEFT);
    break;
  }
}

static void smartenemy_movement_start(ENEMY *e, SERVER *server)
{
  CLIENT *c;

  for (c = server->client; c != NULL && c->jack.id != e->target_id; 
       c = c->next);

  if (c != NULL) {
    if (e->state == STATE_STAND)
      e->state = STATE_WALK;
    if (c->jack.x >= e->x + 64) {
      e->ax = INC_WALK_SPEED;
      e->dir = DIR_RIGHT;
    } else if (c->jack.x <= e->x - 64) {
      e->ax = -INC_WALK_SPEED;
      e->dir = DIR_LEFT;
    } else {
      e->ax = 0;
      if (c->jack.x >= e->x)
	e->dir = DIR_RIGHT;
      else
	e->dir = DIR_LEFT;
    }
    if (e->state == STATE_WALK) {       /* Jump to reach target */
      if (c->jack.y <= e->y - 32) {
	e->state = STATE_JUMP_START;
	e->dy = -START_JUMP_SPEED;
      }
    }
    if (e->state == STATE_JUMP_START) {  /* End jump to reach target */
      if (c->jack.y <= e->y + 32)
	e->dy -= INC_JUMP_SPEED;
      else
	e->state = STATE_JUMP_END;
    }
  }
}


/* I think enemies should be moved horizontally by setting the
 * acceleration (ax) rather than the speed (dx). This makes the enemy
 * move more smoothly. [Moe] */
void move_enemy(SERVER *server, ENEMY *e, NPC_INFO *npc)
{
  e->frame++;

  if (server->change_map)
    return;

  /* If destroyed, leave an item behind */
  if (e->destroy) {
    ITEM *it;
    it = make_new_item(server, get_item_npc(server, "energy"),
		       e->x + npc->clip_w / 2, e->y + npc->clip_h / 2);
    if (it != NULL) {
      it->x -= server->npc_info[it->npc].clip_w / 2;
      it->y -= server->npc_info[it->npc].clip_h / 2;
    }
    return;
  }

  smartenemy_movement_calc(e,server,npc);

  if (e->target_id >= 0) {
    smartenemy_movement_start(e, server);

    enemy_shoot(e);
    e->target_id = verify_target(server, e, e->target_id);

    if (e->target_id < 0)
      e->ax = 0;
  } else
    e->target_id = search_target(server,e);
}
