/* jack.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#include <stdio.h>
#include <malloc.h>

#ifdef GRAPH_WIN
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

#include "common.h"
#include "game.h"

#define MAX_BMP  256    /* Maximum bitmaps for jack animation */

/* Set jack ID colors to their default values */
void set_jack_id_colors(void)
{
  if (SCREEN_BPP == 1) {
    jack_id_color[0][0] = MAKECOLOR8(255, 255, 255);
    jack_id_color[1][0] = MAKECOLOR8(0,   0,   255);
    jack_id_color[2][0] = MAKECOLOR8(0,   255, 0  );
    jack_id_color[3][0] = MAKECOLOR8(255, 255, 0  );
    jack_id_color[4][0] = MAKECOLOR8(255, 0,   0  );
    jack_id_color[5][0] = MAKECOLOR8(255, 0,   255);
    jack_id_color[6][0] = MAKECOLOR8(0,   0,   128);
    jack_id_color[7][0] = MAKECOLOR8(128, 0,   0  );
    jack_id_color[8][0] = MAKECOLOR8(0,   128, 0  );
    jack_id_color[9][0] = MAKECOLOR8(128, 0,   128);
    enemy_color[0] = MAKECOLOR8(127,127,127);
  } else if (SCREEN_BPP == 2) {
    jack_id_color[0][1] = MAKECOLOR16(255, 255, 255);
    jack_id_color[1][1] = MAKECOLOR16(0,   0,   255);
    jack_id_color[2][1] = MAKECOLOR16(0,   255, 0  );
    jack_id_color[3][1] = MAKECOLOR16(255, 255, 0  );
    jack_id_color[4][1] = MAKECOLOR16(255, 0,   0  );
    jack_id_color[5][1] = MAKECOLOR16(255, 0,   255);
    jack_id_color[6][1] = MAKECOLOR16(0,   0,   128);
    jack_id_color[7][1] = MAKECOLOR16(128, 0,   0  );
    jack_id_color[8][1] = MAKECOLOR16(0,   128, 0  );
    jack_id_color[9][1] = MAKECOLOR16(128, 0,   128);
    enemy_color[0] = MAKECOLOR16(127,127,127);
  }
}

int jack_id_color[][4] = {
 { 0, 0, 0, MAKECOLOR32(255,255,255)},
 { 0, 0, 0, MAKECOLOR32(0,  0,  255)},
 { 0, 0, 0, MAKECOLOR32(0,  251,0  )},
 { 0, 0, 0, MAKECOLOR32(255,255,0  )},
 { 0, 0, 0, MAKECOLOR32(255,0,  0  )},
 { 0, 0, 0, MAKECOLOR32(255,0,  255)},
 { 0, 0, 0, MAKECOLOR32(0,  0,  128)},
 { 0, 0, 0, MAKECOLOR32(128,0,  0  )},
 { 0, 0, 0, MAKECOLOR32(0,  128,0  )},
 { 0, 0, 0, MAKECOLOR32(128,0,  128)},
};
int jack_n_colors = sizeof(jack_id_color) / sizeof(jack_id_color[0]);

int enemy_color[4] = {
  0, 0, 0, MAKECOLOR32(128,128,128)
};


static void copy_xbitmap_mask(XBITMAP *dest, XBITMAP *src, int color)
{
  int x, y;

  if (SCREEN_BPP == 1) {
    unsigned char *s, *d;
    unsigned char pre_h, pos_h, pre_v, pos_v;

    /* Horizontal */
    for (y = 0; y < dest->h; y++)
      for (x = 0; x < dest->w; x++) {
	s = ((unsigned char *) src->line[y]) + x;
	d = ((unsigned char *) dest->line[y]) + x;
	pre_h = ((x == 0) ? XBMP8_TRANSP : s[-1]);
	pos_h = ((x == dest->w - 1) ? XBMP8_TRANSP : s[1]);
	pre_v = ((y == 0) ? XBMP8_TRANSP : s[-dest->w]);
	pos_v = ((y == dest->h - 1) ? XBMP8_TRANSP : s[dest->w]);

	if (*s != XBMP8_TRANSP && (pre_h == XBMP8_TRANSP
				   || pos_h == XBMP8_TRANSP
				   || pre_v == XBMP8_TRANSP
				   || pos_v == XBMP8_TRANSP))
	  *d = color;
	else
	  *d = XBMP8_TRANSP;
      }
  } else if (SCREEN_BPP == 2) {
    int2bpp *s, *d;

    for (y = 0; y < dest->h; y++)
      for (x = 0; x < dest->w; x++) {
	s = ((unsigned short *) src->line[y]) + x;
	d = ((unsigned short *) dest->line[y]) + x;
	if (*s != XBMP16_TRANSP)
	  *d = color;
	else
	  *d = XBMP16_TRANSP;
      }
  } else {
    int4bpp *s, *d;

    for (y = 0; y < dest->h; y++)
      for (x = 0; x < dest->w; x++) {
	s = ((int4bpp *) src->line[y]) + x;
	d = ((int4bpp *) dest->line[y]) + x;
	if (*s != XBMP32_TRANSP)
	  *d = color;
	else
	  *d = XBMP32_TRANSP;
      }
  }
}


/* Read the bitmaps of a `jack' from a SPR file */
int read_jack(char *filename, JACK *jack, int color)
{
  XBITMAP *bmp[MAX_BMP];
  int i, n;

  jack->name_bmp = create_xbitmap(NAME_BMP_W, NAME_BMP_H, SCREEN_BPP); 
  if (jack->name_bmp == NULL)
    return 1;
  jack->score_bmp = create_xbitmap(SCORE_BMP_W, SCORE_BMP_H, SCREEN_BPP);
  if (jack->score_bmp == NULL) {
    destroy_xbitmap(jack->name_bmp);
    return 1;
  }
  clear_xbitmap(jack->name_bmp, transp_color);
  clear_xbitmap(jack->score_bmp, transp_color);

  n = read_xbitmaps(filename, MAX_BMP, bmp);
  if (n < 2) {
    for (i = 0; i < n; i++)
      destroy_xbitmap(bmp[i]);
    destroy_xbitmap(jack->name_bmp);
    return 1;
  }

  jack->bmp = (XBITMAP **) malloc(sizeof(XBITMAP *) * n);
  if (jack->bmp == NULL) {
    for (i = 0; i < n; i++)
      destroy_xbitmap(bmp[i]);
    destroy_xbitmap(jack->name_bmp);
    return 1;
  }
  jack->n_bmp = n;
  for (i = 0; i < n; i++)
    jack->bmp[i] = bmp[i];
  jack->w = jack->bmp[0]->w;
  jack->h = jack->bmp[0]->h;


  /* Create the highlight for invincibility and enemies */
  if (n > 0)
    jack->c_mask = (XBITMAP **) malloc(sizeof(XBITMAP *) * n);
  else
    jack->c_mask = NULL;

  if (jack->c_mask != NULL)
    for (i = 0; i < n; i++) {
      jack->c_mask[i] = create_xbitmap(jack->bmp[i]->w, jack->bmp[i]->h,
				       SCREEN_BPP);
      copy_xbitmap_mask(jack->c_mask[i], jack->bmp[i], color);
    }

  return 0;
}

/* Free a `jack' structure */
void destroy_jack(JACK *jack)
{
  if (jack->bmp != NULL) {
    int i;

    for (i = 0; i < jack->n_bmp; i++)
      destroy_xbitmap(jack->bmp[i]);
    free(jack->bmp);
    jack->bmp = NULL;
  }
}


static INLINE void draw_jack_bitmap(int x, int y, int mode, XBITMAP *bmp)
{
  switch (mode) {
  case NPC_DRAW_NORMAL:    draw_bitmap_tr(bmp, x, y); break;
  case NPC_DRAW_NONE:      break;
  case NPC_DRAW_ALPHA25:   draw_bitmap_tr_alpha25(bmp, x, y);  break;
  case NPC_DRAW_ALPHA50:   draw_bitmap_tr_alpha50(bmp, x, y);  break;
  case NPC_DRAW_ALPHA75:   draw_bitmap_tr_alpha75(bmp, x, y);  break;
  case NPC_DRAW_LIGHTEN:   draw_bitmap_tr_lighten(bmp, x, y);  break;
  case NPC_DRAW_DARKEN:    draw_bitmap_tr_darken(bmp, x, y);   break;
  case NPC_DRAW_DISPLACE:  draw_bitmap_tr_displace(bmp, x, y); break;
  default: draw_bitmap_tr(bmp, x, y);
  }
}

/* Draw a `jack' structure in the current graphics context */
void jack_draw(int own, JACK *jack, int scr_x, int scr_y)
{
  NPC_INFO *i;
  int bmp_index;

  i = npc_info + jack->npc;
#ifdef HAS_SOUND
  if (jack->sound >= 0) {
    play_game_sound(jack->sound, jack->sound_x, jack->sound_y);
    jack->sound = -1;
  }
#endif /* HAS_SOUND */

  if (jack->x - scr_x >= -DISP_X && jack->y - scr_y >= -DISP_Y
      && jack->x - scr_x + jack->w <= SCREEN_W + DISP_X
      && jack->y - scr_y + jack->h <= SCREEN_H + DISP_Y) {
    bmp_index = (i->frame[jack->frame]
		 + ((jack->state_flags&NPC_STATE_SHOOT) ? i->shoot_frame : 0));

    if (jack->blink) {
      jack->blink = 0;
      draw_bitmap_tr_blink(jack->bmp[bmp_index],
			   jack->x - scr_x + DISP_X, jack->y - scr_y + DISP_Y);
    } else {
      int c;
      static int mode[JACK_TRAIL_LEN-2] = {
	NPC_DRAW_ALPHA75,
	NPC_DRAW_ALPHA50,
	NPC_DRAW_ALPHA25,
	NPC_DRAW_ALPHA25,
      };

      if (jack->id == player->id && ! own)
	for (c = jack->trail_len - 1; c > 0; c--)
	  if (jack->trail_x[c] - scr_x >= -DISP_X
	      && jack->trail_y[c] - scr_y >= -DISP_Y
	      && jack->trail_x[c] - scr_x + jack->w <= SCREEN_W + DISP_X
	      && jack->trail_y[c] - scr_y + jack->h <= SCREEN_H + DISP_Y)
	    draw_jack_bitmap(jack->trail_x[c] - scr_x + DISP_X,
			     jack->trail_y[c] - scr_y + DISP_Y,
			     ((i->draw_mode == NPC_DRAW_DISPLACE)
			      ? NPC_DRAW_DISPLACE
			      : mode[c-1]),
			     jack->bmp[jack->trail_f[c]]);
      draw_jack_bitmap(jack->x - scr_x + DISP_X, jack->y - scr_y + DISP_Y,
		       i->draw_mode, jack->bmp[bmp_index]);
    }

    /* Draw the mask to indicate other players */
    if (! own && i->draw_mode == NPC_DRAW_NORMAL && jack->c_mask != NULL
	&& jack->c_mask[bmp_index] != NULL) {
      if (SCREEN_BPP == 1)
	draw_bitmap_tr(jack->c_mask[bmp_index],
		       jack->x - scr_x + DISP_X,
		       jack->y - scr_y + DISP_Y);
      else
	draw_bitmap_tr_alpha25(jack->c_mask[bmp_index],
			       jack->x - scr_x + DISP_X,
			       jack->y - scr_y + DISP_Y);
    }
  }
}

/* Draw a `jack' structure in the current graphics context */
void jack_draw_displace(JACK *jack, int scr_x, int scr_y)
{
  NPC_INFO *i;

  i = npc_info + jack->npc;
#ifdef HAS_SOUND
  if (jack->sound >= 0) {
    play_game_sound(jack->sound, jack->sound_x, jack->sound_y);
    jack->sound = -1;
  }
#endif /* HAS_SOUND */

  if (jack->x - scr_x >= -DISP_X && jack->y - scr_y >= -DISP_Y
      && jack->x - scr_x + jack->w <= SCREEN_W + DISP_X
      && jack->y - scr_y + jack->h <= SCREEN_H + DISP_Y) {
    int bmp_index;

    bmp_index = (i->frame[jack->frame]
		 + ((jack->state_flags&NPC_STATE_SHOOT) ? i->shoot_frame : 0));
    draw_bitmap_tr_displace(jack->bmp[bmp_index],
			    jack->x - scr_x + DISP_X,
			    jack->y - scr_y + DISP_Y);
  }
}

/* Draw a `jack' structure in the current graphics context */
void jack_draw_alpha(JACK *jack, int scr_x, int scr_y, int level)
{
  NPC_INFO *i;

  i = npc_info + jack->npc;
#ifdef HAS_SOUND
  if (jack->sound >= 0) {
    play_game_sound(jack->sound, jack->sound_x, jack->sound_y);
    jack->sound = -1;
  }
#endif /* HAS_SOUND */

  if (jack->x - scr_x >= -DISP_X && jack->y - scr_y >= -DISP_Y
      && jack->x - scr_x + jack->w <= SCREEN_W + DISP_X
      && jack->y - scr_y + jack->h <= SCREEN_H + DISP_Y) {
    int bmp_index;

    bmp_index = (i->frame[jack->frame]
		 + ((jack->state_flags&NPC_STATE_SHOOT) ? i->shoot_frame : 0));
    switch (level) {
    case 25:
      draw_bitmap_tr_alpha25(jack->bmp[bmp_index],
			     jack->x - scr_x + DISP_X,
			     jack->y - scr_y + DISP_Y);
      break;
    case 50:
      draw_bitmap_tr_alpha50(jack->bmp[bmp_index],
			     jack->x - scr_x + DISP_X,
			     jack->y - scr_y + DISP_Y);
      break;
    case 75:
      draw_bitmap_tr_alpha75(jack->bmp[bmp_index],
			     jack->x - scr_x + DISP_X,
			     jack->y - scr_y + DISP_Y);
      break;
    case 100:
      draw_bitmap_tr(jack->bmp[bmp_index],
		     jack->x - scr_x + DISP_X,
		     jack->y - scr_y + DISP_Y);
      break;
    }
  }
}
