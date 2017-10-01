/* npc.c
 *
 * General NPC functions
 */


#include <stdio.h>
#include <malloc.h>

#include "game.h"

#define MAX_NPC_TYPES    256

typedef struct {
  NPC *npc;
  int n_npc;
} NPC_TYPE;

static NPC_TYPE types[MAX_NPC_TYPES];

#define SINE_TABLE_SIZE 21

static const int npc_sine_table[SINE_TABLE_SIZE] = {
#if 1
  /* size = 21 */
  0,
  1,
  3,
  4,
  5,
  5,
  5,
  4,
  3,
  1,
  0,
  -1,
  -2,
  -3,
  -4,
  -5,
  -5,
  -5,
  -4,
  -3,
  -1,
#endif


#if 0
  /* size = 14 */
  0,  2,  4,  5,  5,  4,  2,
  0, -2, -4, -5, -5, -4, -2
#endif
};


void remove_all_npcs(void)
{
  int i, j;

  for (i = 0; i < MAX_NPC_TYPES; i++)
    for (j = 0; j < types[i].n_npc; j++)
      types[i].npc[j].active = 0;
}

NPC *allocate_npc(int npc, int id)
{
  NPC_TYPE *type;
  NPC *alloc;
  int i, n, old_n;

  if (npc < 0 || npc >= MAX_NPC_TYPES)
    return NULL;

  type = types + npc;

  if (id < type->n_npc) {
    type->npc[id].active = 1;
    return type->npc + id;
  }

  old_n = type->n_npc;
  n = ((id + 16) / 16) * 16;

  if (type->npc == NULL)
    alloc = (NPC *) malloc(sizeof(NPC) * n);
  else
    alloc = (NPC *) realloc(type->npc, sizeof(NPC) * n);
  if (alloc == NULL)
    return NULL;    /* Out of memory */
  type->n_npc = n;
  type->npc = alloc;
  for (i = old_n; i < n; i++) {
    type->npc[i].npc = npc;
    type->npc[i].id = i;
    type->npc[i].active = 0;
  }
  type->npc[id].active = 1;
  return type->npc + id;
}

/* Remove a missile from the active list */
void free_npc(int npc, int id)
{
  if (npc < MAX_NPC_TYPES && id < types[npc].n_npc)
    types[npc].npc[id].active = 0;
}

NPC *find_npc_id(int npc, int id)
{
  if (npc < MAX_NPC_TYPES && id < types[npc].n_npc
      && types[npc].npc[id].active)
    return types[npc].npc + id;
  return NULL;
}


/* Check if the NPC is heading left */
int npc_is_dir_left(int frame, NPC_INFO *npc)
{
  if (frame >= npc->s_stand
      && frame < npc->s_stand + npc->n_stand) {
    if (frame - npc->s_stand >= npc->n_stand/2)
      return 1;
  } else if (frame >= npc->s_jump
      && frame < npc->s_jump + npc->n_jump) {
    if (frame - npc->s_jump >= npc->n_jump/2)
      return 1;
  } else if (frame >= npc->s_walk
      && frame < npc->s_walk + npc->n_walk) {
    if (frame - npc->s_walk >= npc->n_walk/2)
      return 1;
  }

  return 0;
}

void set_npc_pos(NPC *npc, int npc_num, int x, int y)
{
  NPC_INFO *info;
  XBITMAP *bmp;
  
  info = npc_info + npc_num;
  bmp = info->bmp[info->frame[npc->frame]];

  if (IS_NPC_ITEM(npc_num)) {
    npc->x = x - info->clip_x;
  } else
    if (npc_is_dir_left(npc->frame, info))
      npc->x = x + (info->clip_x + info->clip_w - bmp->w - 1);
    else
      npc->x = x - info->clip_x;

  npc->y = y - info->clip_y;
}

void set_npc(NETMSG *msg)
{
  NPC *n;

  n = find_npc_id(msg->npc_state.npc, msg->npc_state.id);
  if (n != NULL) {
    n->frame = msg->npc_state.frame;
    n->moved = 1;
    set_npc_pos(n, msg->npc_state.npc, msg->npc_state.x, msg->npc_state.y);
  }
}


static INLINE void draw_npc_bitmap(int x, int y, int mode, XBITMAP *bmp)
{
  switch (mode) {
  case NPC_DRAW_NORMAL:   draw_bitmap_tr(bmp, x, y); break;
  case NPC_DRAW_NONE:     /* Don't draw */ break;
  case NPC_DRAW_PARALLAX: /* Special case, handled elsewhere */ break;
  case NPC_DRAW_ALPHA25:  draw_bitmap_tr_alpha25(bmp, x, y); break;
  case NPC_DRAW_ALPHA50:  draw_bitmap_tr_alpha50(bmp, x, y); break;
  case NPC_DRAW_ALPHA75:  draw_bitmap_tr_alpha75(bmp, x, y); break;
  case NPC_DRAW_LIGHTEN:  draw_bitmap_tr_lighten(bmp, x, y); break;
  case NPC_DRAW_DARKEN:   draw_bitmap_tr_darken(bmp, x, y); break;
  case NPC_DRAW_DISPLACE: draw_bitmap_tr_displace(bmp, x, y); break;
  default: draw_bitmap_tr(bmp, x, y);
  }
}

static INLINE void draw_npc(NPC *n, int scr_x, int start_scr_y)
{
  XBITMAP *bmp;
  NPC_INFO *i = npc_info + n->npc;
  int scr_y = start_scr_y;

  if (IS_NPC_ITEM(n->npc))
    switch (i->float_mode) {
    case ITEM_FLOAT_FAST:
      scr_y += npc_sine_table[game_frame % SINE_TABLE_SIZE];
      break;
    case ITEM_FLOAT_SLOW:
      scr_y -= ABS(npc_sine_table[(game_frame/2) % SINE_TABLE_SIZE]);
      break;
    }

  bmp = i->bmp[i->frame[n->frame % (i->n_stand + i->n_jump + i->n_walk)]];
  if (n->x - scr_x >= -DISP_X && n->y - scr_y >= -DISP_Y
      && n->x - scr_x + bmp->w <= SCREEN_W + DISP_X
      && n->y - scr_y + bmp->h <= SCREEN_H + DISP_Y) {
    if (n->blink) {
      n->blink = 0;
      draw_bitmap_tr_blink(bmp, n->x - scr_x + DISP_X, n->y - scr_y + DISP_Y);
    } else
      draw_npc_bitmap(n->x - scr_x + DISP_X, n->y - scr_y + DISP_Y,
		      i->draw_mode, bmp);
  }
}

void draw_npcs(int scr_x, int scr_y, int layer_pos)
{
  int i, j;
  NPC_TYPE *type;
  NPC *npc;

  for (j = npc_number.num; j >= 0; j--) {
    type = types + j;
    for (npc = type->npc, i = 0; i < type->n_npc; i++, npc++)
      if (npc->active && npc_info[npc->npc].layer_pos == layer_pos)
	draw_npc(npc, scr_x, scr_y);
  }
}

void unset_missiles(void)
{
  int i, j;
  NPC *npc;

  for (i = npc_number.weapon_start; i <= npc_number.weapon_end; i++) {
    npc = types[i].npc;
    for (j = types[i].n_npc; j > 0; j--, npc++)
      if (npc->active)
	npc->moved = 0;
  }
}


static void move_missile(NPC *npc)
{
  NPC_INFO *info;

  info = npc_info + npc->npc;
  npc->s_frame++;
  npc->s_frame %= (info->n_stand/2);
  npc->frame = (npc->dx < 0) ? (info->n_stand / 2) : 0;
  npc->frame += info->s_stand + npc->s_frame;

  if (! npc->moved) {
    if (npc->dx2 != 0) {
      int s = 0;

      npc->dx += npc->dx2;
      if (npc->dx < 0) {
	s = 1;
	npc->dx = -npc->dx;
      }
      if (npc->dx >= npc_info[npc->npc].speed) {
	npc->dx = npc_info[npc->npc].speed;
	npc->dx2 = 0;
      }
      if (s)
	npc->dx = -npc->dx;
    }
    npc->x += npc->dx;
    npc->y += npc->dy;
  }
}

void move_missiles(void)
{
  int i, j;
  NPC *npc;

  for (i = npc_number.weapon_start; i <= npc_number.weapon_end; i++) {
    npc = types[i].npc;
    for (j = types[i].n_npc; j > 0; j--, npc++)
      if (npc->active)
	move_missile(npc);
  }
}

static INLINE void draw_npc_parallax(NPC *n, int npc_w, int npc_h,
				     int scr_x, int scr_y)
{
  if (n->x - scr_x >= -DISP_X && n->y - scr_y >= -DISP_Y
      && n->x - scr_x + npc_w <= SCREEN_W + DISP_X
      && n->y - scr_y + npc_h <= SCREEN_H + DISP_Y) {
    if (do_draw_parallax) {
      int px, py;

      px = (scr_x*(parallax_bmp->w-SCREEN_W))/(map_w*tile[0]->w)+(n->x-scr_x);
      py = (scr_y*(parallax_bmp->h-SCREEN_H))/(map_h*tile[0]->h)+(n->y-scr_y);
      draw_parallax_tile(n->x - scr_x + DISP_X, n->y - scr_y + DISP_Y,
			 npc_w, npc_h, px, py);
    } else {
      if (tile[0]->transparent)
	draw_bitmap_tr(tile[0],
		       n->x - scr_x + DISP_X, n->y - scr_y + DISP_Y);
      else
	draw_bitmap(tile[0],
		    n->x - scr_x + DISP_X, n->y - scr_y + DISP_Y);
    }
  }
}

void draw_parallax(void)
{
  int i, j;
  NPC_TYPE *type;
  NPC *npc;

  for (j = npc_number.num; j >= 0; j--) {
    type = types + j;
    for (npc = type->npc, i = 0; i < type->n_npc; i++, npc++)
      if (npc->active) {
	NPC_INFO *info;

	info = npc_info + j;
	if (info->draw_mode == NPC_DRAW_PARALLAX)
	  draw_npc_parallax(npc, info->clip_w, info->clip_h,
			    screen_x, screen_y);
      }
  }
}
