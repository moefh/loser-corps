/* meatball.c
 *
 * The meatball enemy library.
 */

#include <stdio.h>
#include <stdlib.h>

#include <../server/server.h>

#define ABS(x)        (((x)<0)?-(x):(x))


#define BREED_DELAY   400
#define MIN_SPEED     2000
#define MAX_SPEED     ((3 * MAX_WALK_SPEED) / 4)

/* The size of the field of vision: */
#define X_VISION      (3 * (server)->tile_w)
#define Y_VISION      (3 * (server)->tile_w)



/* Spawn an enemy */
static void do_spawn(SERVER *server, ENEMY *e)
{
  ENEMY *spawn;
  NPC_INFO *info;

  if (e->energy < 100) {
    e->energy += 7;
    if (e->energy > 100)
      e->energy = 100;
  }

  info = server->npc_info + e->npc;
  spawn = create_enemy(server, info->weapon_npc[0],
		       e->x + server->npc_info[e->npc].clip_w / 2,
		       e->y + server->npc_info[e->npc].clip_h / 2, e->dir);
  if (spawn == NULL)
    return;
  spawn->x -= server->npc_info[spawn->npc].clip_w / 2;
  spawn->y -= server->npc_info[spawn->npc].clip_h / 2;
  spawn->weapon = 0;
  spawn->vulnerability = 1000;
  spawn->level = 0;
}

/* Try to spawn an enemy */
static int try_to_spawn(SERVER *server, ENEMY *e, int fast)
{
  if (e->weapon_delay <= 0) {
    e->weapon_delay = BREED_DELAY;
    do_spawn(server, e);
    return 1;
  }
  e->weapon_delay -= ((fast) ? 10 : 1);
  return 0;
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

  for (c = server->client; c != NULL && c->jack.id != target; c = c->next)
    ;
  if (c != NULL && c->jack.invisible < 0
      && is_in_field(server, c->jack.x,c->jack.y,e->x,e->y))
    return target;
  return -1;
}

/* Verify presence of targets */
static int search_target(SERVER *server, ENEMY *e)
{
  CLIENT *c;

  for (c = server->client; c != NULL; c = c->next)
    if (c->jack.invisible < 0
	&& is_in_field(server, c->jack.x,c->jack.y,e->x,e->y))
      return c->jack.id;
  return -1;
}



static int get_speed(int min, int max)
{
  return random() % (max-min) + min;
}

static int get_walk_speed(int min, int max)
{
  int s;

  s = get_speed(min, max);
  if (random() % 2)
    s = -s;
  return s;
}

/* Do cannon enemy movement */
void move_enemy(SERVER *server, ENEMY *e, NPC_INFO *npc)
{
  int move_dx, move_dy, flags;

  if (server->change_map)
    return;

  e->frame++;
  if (e->target_id < 0)
    e->target_id = search_target(server, e);
  else
    if (! verify_target(server, e, e->target_id))
      e->target_id = -1;

  if (try_to_spawn(server, e, e->target_id >= 0))
    return;

  if (e->dx == 0 && e->dy == 0)
    while (ABS(e->dx) < 1000 || ABS(e->dy) < 1000) {
      e->dx = get_walk_speed(MIN_SPEED, MAX_SPEED);
      e->dy = get_walk_speed(MIN_SPEED, MAX_SPEED);
    }

  flags = calc_movement(server, 1, e->x, e->y, npc->clip_w, npc->clip_h,
			e->dx / 1000, e->dy / 1000, &move_dx, &move_dy);

  if (flags & CM_Y_CLIPPED) {
    int dy;

    e->dx = get_walk_speed(MIN_SPEED, MAX_SPEED);
    dy = get_speed(MIN_SPEED, MAX_SPEED);
    if (e->dy > 0)
      e->dy = -dy;
    else
      e->dy = dy;
  }

  if (flags & CM_X_CLIPPED) {
    int dx;

    dx = get_speed(MIN_SPEED, MAX_SPEED);
    if (e->dx > 0)
      e->dx = -dx;
    else
      e->dx = dx;
    e->dy = get_walk_speed(MIN_SPEED, MAX_SPEED);
  }

  flags = calc_movement(server, 1, e->x, e->y, npc->clip_w, npc->clip_h,
			e->dx / 1000, e->dy / 1000,
			&move_dx, &move_dy);
  e->x += move_dx;
  e->y += move_dy;
}
