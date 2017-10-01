/* movement.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#include <stdio.h>

#include "server.h"
#include "rect.h"

#if 0
#define OLD_CLIP_METHOD
#endif


#ifndef ABS
#define ABS(x)   ((x < 0) ? -(x) : (x))
#endif

#define SIGN(x)   ((x < 0) ? -1 : (x > 0) ? 1 : 0)

     
typedef struct POINT {
  int x, y;
} POINT;


#if 0
#define POINT_TO_MAP(s, x, y)  ((x < 0 || y < 0				     \
				 || x > (s)->map_w * (s)->tile_w	     \
				 || y > (s)->map_h * (s)->tile_h) ? 0	     \
				: ((s)->map[((y) / (s)->tile_h) * (s)->map_w \
					   +  (x) / (s)->tile_w]))
#else

static INLINE int POINT_TO_MAP(SERVER *s, int x, int y)
{
  if (x < 0 || y < 0
      || x >= s->map_w * s->tile_w || y >= s->map_h * s->tile_h)
    return MAP_BLOCK;
  return s->map[(y / s->tile_h) * s->map_w + x / s->tile_w];
}

#endif



#ifdef OLD_CLIP_METHOD

typedef struct DUPLET {
  int v1, v2;
} DUPLET;


static void clip_point(int *pos, int delta, int tile_size)
{
  if (delta > 0)
    *pos = (*pos / tile_size) * tile_size - 1;
  else if (delta < 0) {
    if (*pos < 0)
      *pos = 0;
    else
      *pos = (*pos / tile_size + 1) * tile_size;
  }
}


int calc_movement(SERVER *server, int calc_all,
		  int x, int y, int w, int h, int dx, int dy,
		  int *ret_dx, int *ret_dy)
{
  /* "Map" of point exclusions */
  static DUPLET excluded[3][3] = {
    { {  3, -1 }, {  2,  3 }, {  2, -1 } },
    { {  1,  3 }, { -1, -1 }, {  0,  2 } },
    { {  1, -1 }, {  0,  1 }, {  0, -1 } }
  };

  int px[4], py[4], init_px[4], init_py[4];
  int i;
  DUPLET *excl;     /* Excluded points from calculation */

  if (dx == 0 && dy == 0) {
    *ret_dx = dx;
    *ret_dy = dy;
    return 0;
  }

  px[0] = x;
  py[0] = y;
  px[1] = x + w;
  py[1] = y;
  px[2] = x;
  py[2] = y + h;
  px[3] = x + w;
  py[3] = y + h;
  for (i = 0; i < 4; i++) {
    init_px[i] = px[i];
    init_py[i] = py[i];
  }

  excl = &excluded[SIGN(dy) + 1][SIGN(dx) + 1];

  for (i = 0; i < 4; i++) {
    if (! calc_all && (excl->v1 == i || excl->v2 == i)) {  /* Excluded */
      px[i] += dx;
      py[i] += dy;
      continue;
    }

    px[i] += dx;
    py[i] += dy;

    {
      int clipped = 0;

      if (POINT_TO_MAP(server, init_px[i] + dx, init_py[i]) == MAP_BLOCK) {
	clip_point(&px[i], dx, server->tile_w);
	clipped = 1;
      }

      if (POINT_TO_MAP(server, init_px[i], init_py[i] + dy) == MAP_BLOCK) {
	clip_point(&py[i], dy, server->tile_h);
	clipped = 1;
      }

      if (! clipped
	  && (POINT_TO_MAP(server, init_px[i] + dx, init_py[i] + dy)
	      == MAP_BLOCK))
	clip_point(&px[i], dx, server->tile_w);
    }
  }

  {
    int tmp_dx, tmp_dy;
    int ret_flags = 0;

    for (i = 0; i < 4; i++) {
      tmp_dx = px[i] - init_px[i];
      tmp_dy = py[i] - init_py[i];
      if (ABS(tmp_dx) < ABS(dx)) {
	ret_flags |= CM_X_CLIPPED;
	dx = tmp_dx;
      }
      if (ABS(tmp_dy) < ABS(dy)) {
	ret_flags |= CM_Y_CLIPPED;
	dy = tmp_dy;
      }
    }

    *ret_dx = dx;
    *ret_dy = dy;
    return ret_flags;
  }      
}

#else /* OLD_CLIP_METHOD */

static RECT clip_block[][2] = {
  { { 0, 0,  2, 2 },  { -1 } },      /* Full */

  { { 0, 0,  1, 1 },  { -1 } },      /* Upper left */
  { { 1, 0,  1, 1 },  { -1 } },      /* Upper right */
  { { 0, 1,  1, 1 },  { -1 } },      /* Lower left */
  { { 1, 1,  1, 1 },  { -1 } },      /* Lower right */

  { { 0, 0,  2, 1 },  { -1 } },      /* Upper */
  { { 0, 0,  1, 2 },  { -1 } },      /* Left */
  { { 1, 0,  1, 2 },  { -1 } },      /* Right */
  { { 0, 1,  2, 1 },  { -1 } },      /* Lower */

  { { 1, 0,  1, 1 },  { 0, 1,  2, 1 } },      /* No upper left */
  { { 0, 0,  1, 1 },  { 0, 1,  2, 1 } },      /* No upper right */
  { { 0, 0,  2, 1 },  { 1, 1,  1, 1 } },      /* No lower left */
  { { 0, 0,  2, 1 },  { 0, 1,  1, 1 } },      /* No lower right */

  { { 0, 0,  1, 1 },  { 1, 1,  1, 1 } },      /* Left cross */
  { { 1, 0,  1, 1 },  { 0, 1,  1, 1 } },      /* Right cross */
};


/* Return the number of blocking */
static int get_block_rect(SERVER *server, int x, int y, RECT *r)
{
  int tile;

  tile = POINT_TO_MAP(server, x, y);
  x = (x / server->tile_w) * server->tile_w;
  y = (y / server->tile_h) * server->tile_h;

  if (tile >= 0 && tile <= 14) {
    r[0] = clip_block[tile][0];
    r[1] = clip_block[tile][1];
    if (r[0].x >= 0) {
      r[0].x = x + (r[0].x * server->tile_w) / 2;
      r[0].y = y + (r[0].y * server->tile_h) / 2;
      r[0].w = (r[0].w * server->tile_w) / 2 - 1;
      r[0].h = (r[0].h * server->tile_h) / 2 - 1;
    } else
      return 0;
    if (r[1].x >= 0) {
      r[1].x = x + (r[1].x * server->tile_w) / 2;
      r[1].y = y + (r[1].y * server->tile_h) / 2;
      r[1].w = (r[1].w * server->tile_w) / 2 - 1;
      r[1].h = (r[1].h * server->tile_h) / 2 - 1;
      return 2;
    }
    return 1;
  }
  return 0;
}


/* Do the clipping for a rectangle wanting to go from `initial' to `final'.
 * If `final' intercepts `block', then `final' is changed to the maximum
 * possible movement without interception */
static int clip_rect(RECT *initial, POINT *delta, RECT *block)
{
  RECT final, inter;

  final = *initial;
  final.x += delta->x;
  final.y += delta->y;

  if (rect_interception(&final, block, &inter)) {
    if (final.x > initial->x)
      delta->x -= inter.w;
    else if (final.x < initial->x)
      delta->x += inter.w;

    if (final.y > initial->y)
      delta->y -= inter.h;
    else if (final.y < initial->y)
      delta->y += inter.h;
    return 1;
  }

  return 0;
}

/* Do collision detection for the vertex `vertex' for the rectangle
 * `initial' moveing with the velocity vector `vel'. Returns the flags
 * containing CM_X_CLIPPED or CM_Y_CLIPPED. */
static int clip_block_vertex(SERVER *server, POINT *vertex,
			     RECT *initial, POINT *vel)
{
  RECT rect[4];
  POINT delta[4];
  int clipped = 0, i, n;

  /* Quick hack to avoid the "chicken bug" */
  if (vel->x + initial->x < 0)
    vel->x = -initial->x;
  if (vel->y + initial->y < 0)
    vel->y = -initial->y;

  if (vel->x != 0) {
    n = get_block_rect(server, vertex->x + vel->x, vertex->y, rect);
    for (i = 0; i < n; i++) {
      delta[i].x = vel->x;
      delta[i].y = 0;
      clipped |= clip_rect(initial, delta + i, rect + i);
    }
    for (i = 0; i < n; i++)
      if (ABS(vel->x) > ABS(delta[i].x))
	vel->x = delta[i].x;
  }

  if (vel->y != 0) {
    n = get_block_rect(server, vertex->x, vertex->y + vel->y, rect);
    for (i = 0; i < n; i++) {
      delta[i].x = 0;
      delta[i].y = vel->y;
      clipped |= clip_rect(initial, delta + i, rect + i);
    }
    for (i = 0; i < n; i++)
      if (ABS(vel->y) > ABS(delta[i].y))
	vel->y = delta[i].y;
  }

  if (! clipped && vel->x != 0 && vel->y != 0) {
    n = get_block_rect(server, vertex->x + vel->x, vertex->y + vel->y, rect);
    for (i = 0; i < n; i++) {
      delta[i] = *vel;
      clipped |= clip_rect(initial, delta + i, rect + i);
    }
    for (i = 0; i < n; i++)
      if (ABS(vel->x) > ABS(delta[i].x))
	vel->x = delta[i].x;
  }

  return clipped;
}


static int do_enemy_clip(SERVER *server, RECT *initial, RECT *rect, POINT *vel)
{
  POINT delta;
  int clipped = 0;

  /* Quick hack to avoid the "chicken bug" */
  if (vel->x + initial->x < 0)
    vel->x = -initial->x;
  if (vel->y + initial->y < 0)
    vel->y = -initial->y;

  if (vel->x != 0) {
    delta.x = vel->x;
    delta.y = 0;
    clipped |= clip_rect(initial, &delta, rect);
    if (ABS(vel->x) > ABS(delta.x))
      vel->x = delta.x;
  }

  if (vel->y != 0) {
    delta.x = 0;
    delta.y = vel->y;
    clipped |= clip_rect(initial, &delta, rect);
    if (ABS(vel->y) > ABS(delta.y))
      vel->y = delta.y;
  }

  if (! clipped && vel->x != 0 && vel->y != 0) {
    delta = *vel;
    clipped |= clip_rect(initial, &delta, rect);
    if (ABS(vel->x) > ABS(delta.x))
      vel->x = delta.x;
  }

  return clipped;
}

static int clip_enemy_vertex(SERVER *server, ENEMY *exclude,
			     RECT *initial, POINT *vel)
{
  NPC_INFO *npc;
  ENEMY *e;
  RECT e_rect;
  int i, clipped;

  clipped = 0;
  for (i = 0; i < server->n_enemies; i++) {
    e = server->enemy + i;
    if (! e->active || e == exclude)
      continue;
    npc = server->npc_info + e->npc;
    if (! npc->block)
      continue;
    e_rect.x = e->x + 1;
    e_rect.y = e->y + 1;
    e_rect.w = npc->clip_w - 1;
    e_rect.h = npc->clip_h - 1;
    clipped |= do_enemy_clip(server, initial, &e_rect, vel);
  }
  return clipped;
}

/* Calculate the movement of a jack.  Given the jack clipping rect
 * (x,y,w,h) and its speed (dx, dy), this function returns in
 * (*ret_dx, *ret_dy) the amount of pixels that the jack can move.
 * The return value contains the flags CM_X_CLIPPED or CM_Y_CLIPPED,
 * corresponding to blocking in the horizontal and vertical,
 * respectivelly. */
int calc_jack_movement(SERVER *server, int calc_all,
		       int x, int y, int w, int h,
		       int dx, int dy, int *ret_dx, int *ret_dy)
{
  RECT initial;
  POINT vertex[4], delta;
  int i;

  if (dx == 0 && dy == 0) {
    *ret_dx = *ret_dy = 0;
    return 0;
  }

  w++;
  h++;
  /* Build initial rectangle and 8 want-to-go rectangles */
  initial.x = x;
  initial.y = y;
  initial.w = w;
  initial.h = h;

  /* Build the array of the vertexes */
  vertex[0].x = x;
  vertex[0].y = y;
  vertex[1].x = x + w - 1;
  vertex[1].y = y;
  vertex[2].x = x;
  vertex[2].y = y + h - 1;
  vertex[3].x = x + w - 1;
  vertex[3].y = y + h - 1;

  /* Clip */
  delta.x = dx;
  delta.y = dy;
  for (i = 0; i < 4; i++)
    clip_block_vertex(server, vertex + i, &initial, &delta);

  clip_enemy_vertex(server, NULL, &initial, &delta);

  *ret_dx = delta.x;
  *ret_dy = delta.y;

  return (((ABS(delta.x) < ABS(dx)) ? CM_X_CLIPPED : 0)
	  | ((ABS(delta.y) < ABS(dy)) ? CM_Y_CLIPPED : 0));
}


/* Calculate the movement for an enemy.  This is the same as
 * calc_jack_movement, but ignores the enemy itself for the effect of
 * clipping. */
int calc_enemy_movement(SERVER *server, ENEMY *e, int calc_all,
			int x, int y, int w, int h,
			int dx, int dy, int *ret_dx, int *ret_dy)
{
  RECT initial;
  POINT vertex[4], delta;
  int i;

  if (dx == 0 && dy == 0) {
    *ret_dx = *ret_dy = 0;
    return 0;
  }

  w++;
  h++;
  /* Build initial rectangle and 8 want-to-go rectangles */
  initial.x = x;
  initial.y = y;
  initial.w = w;
  initial.h = h;

  /* Build the array of the vertexes */
  vertex[0].x = x;
  vertex[0].y = y;
  vertex[1].x = x + w - 1;
  vertex[1].y = y;
  vertex[2].x = x;
  vertex[2].y = y + h - 1;
  vertex[3].x = x + w - 1;
  vertex[3].y = y + h - 1;

  /* Clip */
  delta.x = dx;
  delta.y = dy;
  for (i = 0; i < 4; i++)
    clip_block_vertex(server, vertex + i, &initial, &delta);

  clip_enemy_vertex(server, e, &initial, &delta);

  *ret_dx = delta.x;
  *ret_dy = delta.y;

  return (((ABS(delta.x) < ABS(dx)) ? CM_X_CLIPPED : 0)
	  | ((ABS(delta.y) < ABS(dy)) ? CM_Y_CLIPPED : 0));
}

/* Calculate the movement of something.  This is the same as
 * calc_jack_movement(), but ignores all blocking enemies for the
 * effect of clipping. */
int calc_movement(SERVER *server, int calc_all,
		  int x, int y, int w, int h, int dx, int dy,
		  int *ret_dx, int *ret_dy)
{
  RECT initial;
  POINT vertex[4], delta;
  int i;

  if (dx == 0 && dy == 0) {
    *ret_dx = *ret_dy = 0;
    return 0;
  }

  w++;
  h++;
  /* Build initial rectangle and 8 want-to-go rectangles */
  initial.x = x;
  initial.y = y;
  initial.w = w;
  initial.h = h;

  /* Build the array of the vertexes */
  vertex[0].x = x;
  vertex[0].y = y;
  vertex[1].x = x + w - 1;
  vertex[1].y = y;
  vertex[2].x = x;
  vertex[2].y = y + h - 1;
  vertex[3].x = x + w - 1;
  vertex[3].y = y + h - 1;

  /* Clip */
  delta.x = dx;
  delta.y = dy;
  for (i = 0; i < 4; i++)
    clip_block_vertex(server, vertex + i, &initial, &delta);

  *ret_dx = delta.x;
  *ret_dy = delta.y;

  return (((ABS(delta.x) < ABS(dx)) ? CM_X_CLIPPED : 0)
	  | ((ABS(delta.y) < ABS(dy)) ? CM_Y_CLIPPED : 0));
}

/* Return 1 if the rectangle (x,y)-(w,h) is blocked */
int is_map_blocked(SERVER *server, int x, int y, int w, int h)
{
  RECT rect[4];
  int n_rects;
  POINT vertex[4];
  int i, j;

  if (x < 0 || y < 0)
    return 1;

  /* Build the array of the vertexes */
  vertex[0].x = x;
  vertex[0].y = y;
  vertex[1].x = x + w;
  vertex[1].y = y;
  vertex[2].x = x;
  vertex[2].y = y + h;
  vertex[3].x = x + w;
  vertex[3].y = y + h;

  /* Check map */
  for (i = 0; i < 4; i++) {
    n_rects = get_block_rect(server, vertex[i].x, vertex[i].y, rect);
    for (j = 0; j < n_rects; j++)
      if (point_in_rect(vertex[i].x, vertex[i].y,
			rect[j].x, rect[j].y, rect[j].w, rect[j].h))
	return 1;
  }

  return 0;
}


/* Return 1 if the rectangle (x,y)-(w,h) is blocked */
int is_map_enemy_blocked(SERVER *server, int x, int y, int w, int h)
{
  RECT rect[4];
  int n_rects;
  POINT vertex[4];
  int i, j;

  if (x < 0 || y < 0)
    return 1;

  /* Build the array of the vertexes */
  vertex[0].x = x;
  vertex[0].y = y;
  vertex[1].x = x + w;
  vertex[1].y = y;
  vertex[2].x = x;
  vertex[2].y = y + h;
  vertex[3].x = x + w;
  vertex[3].y = y + h;

  /* Check map */
  for (i = 0; i < 4; i++) {
    n_rects = get_block_rect(server, vertex[i].x, vertex[i].y, rect);
    for (j = 0; j < n_rects; j++)
      if (point_in_rect(vertex[i].x, vertex[i].y,
			rect[j].x, rect[j].y, rect[j].w, rect[j].h))
	return 1;
  }

  /* Check enemies */
  for (i = 0; i < server->n_enemies; i++) {
    ENEMY *e;
    NPC_INFO *npc;

    if ((e = server->enemy + i)->active
	&& (npc = server->npc_info + e->npc)->block) {
      RECT re, r, tmp;

      r.x = x; r.y = y;
      r.w = w; r.h = h;
      re.x = e->x; re.y = e->y;
      re.w = npc->clip_w; re.h = npc->clip_h;
      if (rect_interception(&re, &r, &tmp))
	return 1;
    }
  }

  return 0;
}

#endif /* OLD_CLIP_METHOD */
