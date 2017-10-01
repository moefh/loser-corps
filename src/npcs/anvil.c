/* anvil.c
 *
 * The `anvil' enemy library.
 */


#include <stdio.h>

#include <../server/server.h>
#include <../server/rect.h>


/* The size of the field of vision: */
#define X_VISION      (3 * (server)->tile_w / 2)
#define Y_VISION      (10 * (server)->tile_w)


static int is_in_field(SERVER *server,
		       int jack_x, int jack_y, int enemy_x, int enemy_y)
{
  return (jack_x <= enemy_x + X_VISION &&
	  jack_x >= enemy_x - X_VISION &&
	  jack_y <= enemy_y + Y_VISION &&
	  jack_y >= enemy_y - Y_VISION);
}


static void anvil_hit_jack(SERVER *server, ENEMY *e, CLIENT *c)
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
      anvil_hit_jack(server, e, c);
    }
  }
}

/* Verify presence of targets */
static CLIENT *search_target(SERVER *server, ENEMY *e, NPC_INFO *npc)
{
  CLIENT *c;

  for (c = server->client; c != NULL; c = c->next)
    if (c->jack.invisible < 0
       && is_in_field(server, c->jack.x, c->jack.y,
		      e->x + npc->clip_w / 2, e->y + npc->clip_h / 2))
      return c;
  return NULL;
}


static void do_move(SERVER *server, ENEMY *e, NPC_INFO *npc)
{
  int flags, move_dx, move_dy;

  flags = calc_enemy_movement(server, e, 1,
			      e->x, e->y, npc->clip_w, npc->clip_h,
			      e->dx / 1000, e->dy / 1000,
			      &move_dx, &move_dy);
  e->x += move_dx;
  e->y += move_dy;
  suck_energy(server, e);
  if (flags & CM_Y_CLIPPED) {
    srv_do_tremor(server, e->x + npc->clip_w / 2, e->y + npc->clip_h / 2,
		  24, 3);
    e->dy = 0;
  }
}

/* Do anvil enemy movement */
void move_enemy(SERVER *server, ENEMY *e, NPC_INFO *npc)
{
  CLIENT *c;

  e->vulnerability = 0;
  e->frame++;

  if (e->target_id >= 0 && e->dy == 0)
    return;

  if ((c = search_target(server, e, npc)) != NULL) {
    e->target_id = c->jack.id;
    e->dy = 20000;
  }

  do_move(server, e, npc);
}
