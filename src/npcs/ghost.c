/* ghost.c
 *
 * This was originally written for the server by Roberto Motta
 * (ragm@linux.ime.usp.br), and later adapted to work as an enemy
 * library by Moe (massaro@ime.usp.br).
 */

#include <stdio.h>
#include <stdlib.h>

#include <../server/server.h>
#include <../server/rect.h>

#define ABS(x)  (((x) < 0)?-(x):(x))

#define WEAPON_DELAY  10
#define JUMP_SPEED    ((7 * MAX_JUMP_SPEED) / 4)
#define WALK_SPEED    (2 * MAX_WALK_SPEED / 3)

/* The size of the field of vision: */
static int x_vision, y_vision;


static void drain_energy(SERVER *server, ENEMY *e)
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

static int is_in_field(SERVER *server,
		       int jack_x, int jack_y, int enemy_x, int enemy_y)
{
  return(jack_x <= enemy_x + x_vision &&
	 jack_x >= enemy_x - x_vision &&
	 jack_y <= enemy_y + y_vision &&
	 jack_y >= enemy_y - y_vision);
}


static CLIENT *verify_target(SERVER *server, ENEMY *e, int target)
{
  CLIENT *c;

  for (c = server->client; c != NULL && c->jack.id != target; c = c->next)
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

  for (c = server->client; c != NULL; c = c->next)
    if (c->jack.invisible < 0
	&& is_in_field(server, c->jack.x,c->jack.y,e->x,e->y))
      return c->jack.id;
  return -1;
}



void move_enemy(SERVER *server, ENEMY *e, NPC_INFO *npc)
{
  CLIENT *c = NULL;

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

  e->frame++;
  if (e->target_id < 0)
    e->target_id = search_target(server, e);

  if (e->target_id >= 0) {
    x_vision = (5 * (server)->tile_w);
    y_vision = (5 * (server)->tile_h);
  } else {
    x_vision = (2 * (server)->tile_w);
    y_vision = (2 * (server)->tile_h);
  }

  if ((c = verify_target(server, e, e->target_id)) == NULL)
    e->target_id = -1;

  if (c != NULL) {
    int ex, ey, cx, cy;

    cx = c->jack.x + server->npc_info[c->jack.npc].clip_w/2;
    cy = c->jack.y + server->npc_info[c->jack.npc].clip_h/2;
    ex = e->x + server->npc_info[e->npc].clip_w/2;
    ey = e->y + server->npc_info[e->npc].clip_h/2;
    if (ABS(cx - ex) < 5)
      e->dx = 0;
    else if (cx > ex)
      e->dx = WALK_SPEED;
    else
      e->dx = -WALK_SPEED;

    if (ABS(cy - ey) < 5)
      e->dy = 0;
    else if (ey < cy)
      e->dy = WALK_SPEED;
    else
      e->dy = -WALK_SPEED;
  } else
    e->dx = e->dy = 0;

  if (e->dx > 0)
    e->dir = DIR_RIGHT;
  else if (e->dx < 0)
    e->dir = DIR_LEFT;

  drain_energy(server, e);

  e->x += e->dx / 1000;
  e->y += e->dy / 1000;
}
