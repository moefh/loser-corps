/* draw.c */

#include <stdio.h>

#include "game.h"


void draw_parallax_tile(int x, int y, int w, int h, int sx, int sy)
{
  int i;

  if (parallax_bmp == NULL)
    return;

#if 0
  printf("parallax_bmp(%d,%d,%d,%d)/(%d,%d), screen(%d,%d)\n",
	 sx, sy, w, h, parallax_bmp->w, parallax_bmp->h, x, y);
#endif

  if (sy + h >= parallax_bmp->h)
    h = parallax_bmp->h - sy - 1;
  if (sx + w >= parallax_bmp->w)
    w = parallax_bmp->w - sx - 1;

  if (sx < 0)
    for (i = (sy < 0) ? -sy : 0; i < h; i++)
      draw_bmp_line(parallax_bmp, 0, sy + i, x - sx, y + i, w + sx);
  else
    for (i = (sy < 0) ? -sy : 0; i < h; i++)
      draw_bmp_line(parallax_bmp, sx, sy + i, x, y + i, w);
}

void draw_bitmap_alpha(int alpha, XBITMAP *bmp, int x, int y)
{
  switch (alpha) {
  case 25: draw_bitmap_alpha25(bmp, x, y); break;
  case 50: draw_bitmap_alpha50(bmp, x, y); break;
  case 75: draw_bitmap_alpha75(bmp, x, y); break;
  default: draw_bitmap(bmp, x, y); break;
  }
}

void draw_bitmap_tr_alpha(int alpha, XBITMAP *bmp, int x, int y)
{
  switch (alpha) {
  case 25: draw_bitmap_tr_alpha25(bmp, x, y); break;
  case 50: draw_bitmap_tr_alpha50(bmp, x, y); break;
  case 75: draw_bitmap_tr_alpha75(bmp, x, y); break;
  default: draw_bitmap_tr(bmp, x, y); break;
  }
}


void draw_background(void)
{
  int x, y, w, h;
  int tile_x, tile_y, tile_w, tile_h;
  int start_x, start_y;
  MAP *map_l;

  tile_w = tile[0]->w;
  tile_h = tile[0]->h;
  tile_x = screen_x / tile_w;
  tile_y = screen_y / tile_h;
  start_x = DISP_X - 1 - screen_x % tile_w;
  start_y = DISP_Y - 1 - screen_y % tile_h;
  w = (SCREEN_W + tile_w - 1) / tile_w;
  h = (SCREEN_H + tile_h - 1) / tile_h;

  if (w * tile_w + start_x < DISP_X + SCREEN_W)
    w++;
  if (h * tile_h + start_y < DISP_Y + SCREEN_H)
    h++;

  for (y = 0; y < h; y++) {
    map_l = map_line[tile_y + y] + tile_x;
    for (x = 0; x < w; x++, map_l++) {
      if ((map_l->back<0 || tile[map_l->back]->transparent)
	   && (map_l->fore<0 || tile[map_l->fore]->transparent)) {
	if (do_draw_parallax) {
	  if (parallax_bmp != NULL) {
	    int tx, ty, px, py;
	    int map_pw, map_ph;

	    map_pw = map_w * tile_w;
	    map_ph = map_h * tile_h;
	    tx = start_x + x * tile_w;
	    ty = start_y + y * tile_h;
	    px = screen_x - ((screen_x * (map_pw - parallax_bmp->w))
			     / (map_pw - SCREEN_W - 1));
	    py = screen_y - ((screen_y * (map_ph - parallax_bmp->h))
			     / (map_ph - SCREEN_H - 1));
	    px += tx - DISP_X;
	    py += ty - DISP_Y;
	    draw_parallax_tile(tx, ty, tile_w, tile_h, px, py);
	  }
	} else {
	  if (tile[0]->transparent)
	    draw_bitmap_tr(tile[0],
			   start_x + x * tile_w, start_y + y * tile_h);
	  else
	    draw_bitmap(tile[0],
			start_x + x * tile_w, start_y + y * tile_h);
	}
      }

      if (map_l->back >= 0) {
	if (tile[map_l->back]->transparent)
	  draw_bitmap_tr(tile[map_l->back],
			 start_x + x * tile_w, start_y + y * tile_h);
	else
	  draw_bitmap(tile[map_l->back],
		      start_x + x * tile_w, start_y + y * tile_h);
      }

    }
  }
}

void draw_foreground(void)
{
  int x, y, w, h;
  int tile_x, tile_y, tile_w, tile_h;
  int start_x, start_y;
  MAP *map_l;

  tile_w = tile[0]->w;
  tile_h = tile[0]->h;
  tile_x = screen_x / tile_w;
  tile_y = screen_y / tile_h;
  start_x = DISP_X - 1 - screen_x % tile_w;
  start_y = DISP_Y - 1 - screen_y % tile_h;
  w = (SCREEN_W + tile_w - 1) / tile_w;
  h = (SCREEN_H + tile_h - 1) / tile_h;

  if (w * tile_w + start_x < DISP_X + SCREEN_W)
    w++;
  if (h * tile_h + start_y < DISP_Y + SCREEN_H)
    h++;

  for (y = 0; y < h; y++) {
    map_l = map_line[tile_y + y] + tile_x;
    for (x = 0; x < w; x++, map_l++)
      if (map_l->fore >= 0) {
        if (tile[map_l->fore]->transparent)
	  draw_bitmap_tr(tile[map_l->fore],
			 start_x + x * tile_w, start_y + y * tile_h);
	else
	  draw_bitmap(tile[map_l->fore],
		      start_x + x * tile_w, start_y + y * tile_h);
      }
  }
}


void draw_background_alpha(int alpha)
{
  int x, y, w, h;
  int tile_x, tile_y, tile_w, tile_h;
  int start_x, start_y;
  MAP *map_l;

  tile_w = tile[0]->w;
  tile_h = tile[0]->h;
  tile_x = screen_x / tile_w;
  tile_y = screen_y / tile_h;
  start_x = DISP_X - 1 - screen_x % tile_w;
  start_y = DISP_Y - 1 - screen_y % tile_h;
  w = (SCREEN_W + tile_w - 1) / tile_w;
  h = (SCREEN_H + tile_h - 1) / tile_h;

  if (w * tile_w + start_x < DISP_X + SCREEN_W)
    w++;
  if (h * tile_h + start_y < DISP_Y + SCREEN_H)
    h++;

  for (y = 0; y < h; y++) {
    map_l = map_line[tile_y + y] + tile_x;
    for (x = 0; x < w; x++, map_l++)
      if (map_l->back >= 0)
	draw_bitmap_alpha(alpha, tile[map_l->back],
			  start_x + x * tile_w, start_y + y * tile_h);
  }
}

void draw_foreground_alpha(int alpha)
{
  int x, y, w, h;
  int tile_x, tile_y, tile_w, tile_h;
  int start_x, start_y;
  MAP *map_l;

  tile_w = tile[0]->w;
  tile_h = tile[0]->h;
  tile_x = screen_x / tile_w;
  tile_y = screen_y / tile_h;
  start_x = DISP_X - 1 - screen_x % tile_w;
  start_y = DISP_Y - 1 - screen_y % tile_h;
  w = (SCREEN_W + tile_w - 1) / tile_w;
  h = (SCREEN_H + tile_h - 1) / tile_h;

  if (w * tile_w + start_x < DISP_X + SCREEN_W)
    w++;
  if (h * tile_h + start_y < DISP_Y + SCREEN_H)
    h++;

  for (y = 0; y < h; y++) {
    map_l = map_line[tile_y + y] + tile_x;
    for (x = 0; x < w; x++, map_l++) {
      if (map_l->fore >= 0) {
        if (tile[map_l->fore]->transparent)
	  draw_bitmap_tr_alpha(alpha, tile[map_l->fore],
			       start_x + x * tile_w, start_y + y * tile_h);
	else
	  draw_bitmap_alpha(alpha, tile[map_l->fore],
			    start_x + x * tile_w, start_y + y * tile_h);
      }
    }
  }
}
