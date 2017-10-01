/* draw.c */

#include <stdio.h>
#include <string.h>

/* USE_ASM is now defined in the new configure system
 * (keeping this comment for reference for now)
#ifndef sparc
#define USE_ASM 1
#endif
*/

#include "mapedit.h"
#include "map.h"
#include "bitmap.h"

#if USE_ASM
#include "inline_asm.h"
#else
#define single_memcpy(dest, src, n)     memcpy((dest), (src), (n))
#endif

int screen_bpp;
static int n_width, n_height;
static XImage *img;
static XBITMAP *tmp;
static int dbl_screen;

static void draw_sel(unsigned char *dest, int line, int w)
{
  int i;

  if (line & 1) {
    w--;
    dest++;
  }
  for (i = w; i > 0; i--, i--, dest++)
    *dest++ = 7;
}

static void lighten_pixel(unsigned short *dest)
{
  unsigned char r, g, b;
  int c;

  c = *dest;
  r = ((((c >> 11) & 0x1f) << 3) + 255) / 2;
  g = ((((c >> 5 ) & 0x3f) << 2) + 255) / 2;
  b = ((((c      ) & 0x1f) << 3) + 255) / 2;
  *dest = MAKECOLOR16(r,g,b);
}

static void draw_sel_16(unsigned short *dest, int line, int w)
{
  int i, c;
  unsigned char r, g, b;

  for (i = w; i > 0; i--) {
    c = *dest;
    r = ((((c >> 11) & 0x1f) << 3) + 255) / 2;
    g = ((((c >> 5 ) & 0x3f) << 2) + 32) / 2;
    b = ((((c      ) & 0x1f) << 3)) / 2;
    *dest++ = MAKECOLOR16(r,g,b);
  }
}


static void draw_hline(int x, int y, int w, int color)
{
  int i;

  if (SCREEN_BPP == 1) {
    if (dbl_screen) {
      unsigned char *dest = tmp->line[y] + x;
      for (i = w; i > 0; i--)
	*dest++ = color;
    } else {
      char *dest = img->data + img->bytes_per_line * y + x;
      for (i = w; i > 0; i--)
	*dest++ = color;
    }
  } else {
    unsigned short *dest;

    if (dbl_screen) {
      dest = ((unsigned short *) tmp->line[y]) + x;
      for (i = w; i > 0; i--)
	*dest++ = color;
    } else {
      dest = ((unsigned short *)
	      (((unsigned char *) img->data) + img->bytes_per_line * y) + x);
      for (i = w; i > 0; i--)
	*dest++ = color;
    }
  }
}

static void draw_vline(int x, int y, int h, int color)
{
  int i;

  if (SCREEN_BPP == 1) {
    if (dbl_screen) {
      unsigned char *dest = tmp->line[y] + x;
      for (i = h; i > 0; i--) {
	*dest = color;
	dest += tmp->w;
      }
    } else {
      char *dest = img->data + img->bytes_per_line * y + x;
      for (i = h; i > 0; i--) {
	*dest = color;
	dest += img->bytes_per_line;
      }
    }
  } else {
    unsigned short *dest;

    if (dbl_screen) {
      dest = ((unsigned short *) tmp->line[y]) + x;
      for (i = h; i > 0; i--) {
	*dest = color;
	dest += tmp->w;
      }
    } else {
      dest = ((unsigned short *)
	      (((unsigned char *) img->data) + img->bytes_per_line * y) + x);
      for (i = h; i > 0; i--) {
	*dest = color;
	dest += img->bytes_per_line/2;
      }
    }
  }
}

void draw_bitmap(XBITMAP *bmp, int x, int y)
{
  int i;

  if (SCREEN_BPP == 1) {
    unsigned char *src = bmp->data;
    if (dbl_screen) {
      unsigned char *dest = tmp->line[y] + x;
      for (i = bmp->h; i > 0; i--) {
	single_memcpy(dest, src, bmp->w);
	src += bmp->w;
	dest += tmp->w;
      }
    } else {
      char *dest = img->data + img->bytes_per_line * y + x;
      for (i = bmp->h; i > 0; i--) {
	single_memcpy(dest, src, bmp->w);
	src += bmp->w;
	dest += img->bytes_per_line;
      }
    }
  } else {    /* 2 bpp */
    unsigned short *src, *dest;

    /* printf("draw_bitmap() start\n"); */
    src = (unsigned short *) bmp->data;
    if (dbl_screen) {
      dest = ((unsigned short *) tmp->line[y]) + x;
      for (i = bmp->h; i > 0; i--) {
	single_memcpy(dest, src, bmp->line_w);
	src += bmp->w;
	dest += tmp->w;
      }
    } else {
      dest = ((unsigned short *)
	      (((unsigned char *) img->data) + img->bytes_per_line * y) + x);
      for (i = bmp->h; i > 0; i--) {
	single_memcpy(dest, src, bmp->line_w);
	src += bmp->w;
	dest += img->bytes_per_line/2;
      }
    }

    /* printf("draw_bitmap() end\n"); */
  }
}

void draw_bitmap_tr(XBITMAP *bmp, int x, int y)
{
  int i, j, w;

  if (SCREEN_BPP == 1) {
    unsigned char *src;

    w = bmp->w;

    src = bmp->data;
    if (dbl_screen) {
      unsigned char *dest = tmp->line[y] + x;
      for (i = bmp->h; i > 0; i--) {
	for (j = w; j > 0; j--) {
	  if (*src != XBMP8_TRANSP)
	    *dest = *src;
	  src++;
	  dest++;
	}
	dest += tmp->w - w;
      }
    } else {
      for (i = 0; i < bmp->h; i++) {
	char *dest = img->data + img->bytes_per_line * (i+y) + x;
	for (j = w; j > 0; j--) {
	  if (*src != XBMP8_TRANSP)
	    *dest = *src;
	  src++;
	  dest++;
	}
      }
    }
  } else {     /* 2 bpp */
    unsigned short *src, *dest;

    /* printf("draw_bitmap_tr() start\n"); */
    w = bmp->w;

    src = (unsigned short *) bmp->data;
    if (dbl_screen) {
      dest = ((unsigned short *) tmp->line[y]) + x;
      for (i = bmp->h; i > 0; i--) {
	for (j = w; j > 0; j--) {
	  if (*src != XBMP16_TRANSP)
	    *dest = *src;
	  src++;
	  dest++;
	}
	dest += tmp->w - w;
      }
    } else {
      for (i = 0; i < bmp->h; i++) {
	dest = (((unsigned short *)
		(((unsigned char *) img->data) + img->bytes_per_line * (i+y)))
		+ x);
	for (j = w; j > 0; j--) {
	  if (*src != XBMP16_TRANSP)
	    *dest = *src;
	  src++;
	  dest++;
	}
      }
    }
    /* printf("draw_bitmap_tr() end\n"); */
  }
}

static void draw_object_selection(XBITMAP *bmp, int x, int y)
{
  int i, j, w;

  if (SCREEN_BPP == 1) {
    unsigned char *src;

    w = bmp->w;

    src = bmp->data;
    if (dbl_screen) {
      unsigned char *dest = tmp->line[y] + x;
      for (i = bmp->h; i > 0; i--) {
	for (j = w; j > 0; j--) {
	  if (*src && ((i % 4) * (j % 6)) == 0)
	    *dest = 15;
	  src++;
	  dest++;
	}
	dest += tmp->w - w;
      }
    } else {
      for (i = 0; i < bmp->h; i++) {
	char *dest = img->data + img->bytes_per_line * (i+y) + x;
	for (j = w; j > 0; j--) {
	  if (*src && ((i % 4) * (j % 6)) == 0)
	    *dest = 15;
	  src++;
	  dest++;
	}
      }
    }
  } else {     /* 2 bpp */
    unsigned short *src, *dest;

    /* printf("draw_bitmap_tr() start\n"); */
    w = bmp->w;

    src = (unsigned short *) bmp->data;
    if (dbl_screen) {
      dest = ((unsigned short *) tmp->line[y]) + x;
      for (i = bmp->h; i > 0; i--) {
	for (j = w; j > 0; j--) {
	  if (*src != XBMP16_TRANSP)
	    lighten_pixel(dest);
	  src++;
	  dest++;
	}
	dest += tmp->w - w;
      }
    } else {
      for (i = 0; i < bmp->h; i++) {
	dest = (((unsigned short *)
		(((unsigned char *) img->data) + img->bytes_per_line * (i+y)))
		+ x);
	for (j = w; j > 0; j--) {
	  if (*src != XBMP16_TRANSP)
	    lighten_pixel(dest);
	  src++;
	  dest++;
	}
      }
    }
    /* printf("draw_bitmap_tr() end\n"); */
  }
}

static void draw_items(int x, int y, int w, int h, int selected,
		       MAP_OBJECT *obj, XBITMAP **bmp, NPC_INFO *info)
{
  int bx, by, i = 0;

  for (; obj != NULL; obj = obj->next) {
    if (obj->npc < 0 || bmp[obj->npc] == NULL)
      continue;
    bx = obj->x - x - info[obj->npc].clip_x + 1;
    by = obj->y - y - info[obj->npc].clip_y + 1;
    if (bx > -DISP_X && bx < w && by > -DISP_Y && by < h) {
      draw_bitmap_tr(bmp[obj->npc], bx + DISP_X, by + DISP_Y);
      if (selected == i)
	draw_object_selection(bmp[obj->npc], bx + DISP_X, by + DISP_Y);
    }
    i++;
  }
}

static void draw_tile_selection(int x, int y, int w, int h)
{
  int i;

  if (SCREEN_BPP == 1) {
    if (dbl_screen) {
      unsigned char *dest = tmp->line[y] + x;
      for (i = 0; i < h; i++) {
	draw_sel(dest, i, w);
	dest += tmp->w;
      }
    } else {
      char *dest = img->data + img->bytes_per_line * y + x;
      for (i = 0; i < h; i++) {
	draw_sel((unsigned char *) dest, i, w);
	dest += img->bytes_per_line;
      }
    }
  } else {
    unsigned short *dest;

    if (dbl_screen) {
      dest = ((unsigned short *) tmp->line[y]) + x;
      for (i = 0; i < h; i++) {
	draw_sel_16(dest, i, w);
	dest += tmp->w;
      }
    } else {
      dest = ((unsigned short *) (img->data + img->bytes_per_line * y)) + x;
      for (i = 0; i < h; i++) {
	draw_sel_16(dest, i, w);
	dest += img->bytes_per_line>>1;
      }
    }
  }
}


static void draw_background(int screen_x, int screen_y,
			    int screen_w, int screen_h,
			    MAP_DATA **map, XBITMAP **bmp, int n_bmps,
			    XBITMAP **back, int n_back_bmps)
{
  int x, y, w, h;
  int tile_x, tile_y, tile_w, tile_h;
  int start_x, start_y;

  tile_w = bmp[0]->w;
  tile_h = bmp[0]->h;
  tile_x = screen_x / tile_w;
  tile_y = screen_y / tile_h;
  start_x = DISP_X - screen_x % tile_w;
  start_y = DISP_Y - screen_y % tile_h;
  w = (screen_w + tile_w - 1) / tile_w;
  h = (screen_h + tile_h - 1) / tile_h;

  if (w * tile_w + start_x < DISP_X + screen_w)
    w++;
  if (h * tile_h + start_y < DISP_Y + screen_h)
    h++;

  if (tile_x + w > n_width)
    w = n_width - tile_x;
  if (tile_y + h > n_height)
    h = n_height - tile_y;

  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++) {
      if ((map[tile_y + y][tile_x + x].back < 0
	   || bmp[map[tile_y + y][tile_x + x].back % n_bmps]->transparent)) {
	int nx, ny, bx, by;

	switch (n_back_bmps) {
	case 1: ny = nx = 1; break;
	case 4: ny = nx = 2; break;
	case 9: ny = nx = 3; break;
	case 16: ny = nx = 4; break;
	case 25: ny = nx = 5; break;
	default:
	  if (n_back_bmps % 2 == 0) {
	    ny = 2;
	    nx = n_back_bmps / 2;
	  } else {
	    ny = 1;
	    nx = n_back_bmps;
	  }
	}
	by = (tile_y + y) % ny;
	bx = (tile_x + x) % nx;
	
	draw_bitmap(back[bx + by * nx],
		    start_x + x * tile_w, start_y + y * tile_h);
      }

      if (map[tile_y + y][tile_x + x].back >= 0) {
	XBITMAP *b;

	b = bmp[map[tile_y + y][tile_x + x].back % n_bmps];
	if (b->transparent)
	  draw_bitmap_tr(b, start_x + x * tile_w, start_y + y * tile_h);
	else
	  draw_bitmap(b, start_x + x * tile_w, start_y + y * tile_h);
      }
    }
}

static void draw_foreground(int screen_x, int screen_y,
			    int screen_w, int screen_h,
			    MAP_DATA **map, XBITMAP **bmp, int n_bmps)
{
  int x, y, w, h;
  int tile_x, tile_y, tile_w, tile_h;
  int start_x, start_y;

  tile_w = bmp[0]->w;
  tile_h = bmp[0]->h;
  tile_x = screen_x / tile_w;
  tile_y = screen_y / tile_h;
  start_x = DISP_X - screen_x % tile_w;
  start_y = DISP_Y - screen_y % tile_h;
  w = (screen_w + tile_w - 1) / tile_w;
  h = (screen_h + tile_h - 1) / tile_h;

  if (w * tile_w + start_x < DISP_X + screen_w)
    w++;
  if (h * tile_h + start_y < DISP_Y + screen_h)
    h++;

  if (tile_x + w > n_width)
    w = n_width - tile_x;
  if (tile_y + h > n_height)
    h = n_height - tile_y;

  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++)
      if (map[tile_y + y][tile_x + x].fore >= 0)
	draw_bitmap_tr(bmp[map[tile_y + y][tile_x + x].fore % n_bmps],
		       start_x + x * tile_w, start_y + y * tile_h);
}


static void draw_blocks(int screen_x, int screen_y,
			int screen_w, int screen_h,
			MAP_DATA **map, XBITMAP **bmp, int n_bmps)
{
  int x, y, w, h;
  int tile_x, tile_y, tile_w, tile_h;
  int start_x, start_y;

  tile_w = bmp[0]->w;
  tile_h = bmp[0]->h;
  tile_x = screen_x / tile_w;
  tile_y = screen_y / tile_h;
  start_x = DISP_X - screen_x % tile_w;
  start_y = DISP_Y - screen_y % tile_h;
  w = (screen_w + tile_w - 1) / tile_w;
  h = (screen_h + tile_h - 1) / tile_h;

  if (w * tile_w + start_x < DISP_X + screen_w)
    w++;
  if (h * tile_h + start_y < DISP_Y + screen_h)
    h++;

  if (tile_x + w > n_width)
    w = n_width - tile_x;
  if (tile_y + h > n_height)
    h = n_height - tile_y;

  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++) {
      if (map[tile_y + y][tile_x + x].block >= 0)
	draw_bitmap_tr(bmp[map[tile_y + y][tile_x + x].block % n_bmps],
		       start_x + x * tile_w, start_y + y * tile_h);
    }
}


static void draw_block_selection(int screen_x, int screen_y,
				 int screen_w, int screen_h,
				 MAP_DATA **map, int bmp_w, int bmp_h)
{
  int x, y, w, h;
  int tile_x, tile_y, tile_w, tile_h;
  int start_x, start_y;

  tile_w = bmp_w;
  tile_h = bmp_h;
  tile_x = screen_x / tile_w;
  tile_y = screen_y / tile_h;
  start_x = DISP_X - screen_x % tile_w;
  start_y = DISP_Y - screen_y % tile_h;
  w = (screen_w + tile_w - 1) / tile_w;
  h = (screen_h + tile_h - 1) / tile_h;

  if (w * tile_w + start_x < DISP_X + screen_w)
    w++;
  if (h * tile_h + start_y < DISP_Y + screen_h)
    h++;

  if (tile_x + w > n_width)
    w = n_width - tile_x;
  if (tile_y + h > n_height)
    h = n_height - tile_y;

  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++) {
      if (map[tile_y + y][tile_x + x].sel)
	draw_tile_selection(start_x + x * tile_w, start_y + y * tile_h,
			    bmp_w, bmp_h);
    }
}


static void draw_grid(int screen_x, int screen_y, int screen_w, int screen_h,
		      int tile_w, int tile_h)
{
  int start_x = DISP_X - screen_x % tile_w;
  int start_y = DISP_Y - screen_y % tile_h;
  int w = (screen_w + tile_w - 1) / tile_w;
  int h = (screen_h + tile_h - 1) / tile_h;
  int tile_x = screen_x / tile_w;
  int tile_y = screen_y / tile_h;
  int x, y, color;

  if (w * tile_w + start_x < DISP_X + screen_w)
    w++;
  if (h * tile_h + start_y < DISP_Y + screen_h)
    h++;

  if (tile_x + w > n_width)
    w = n_width - tile_x;
  if (tile_y + h > n_height)
    h = n_height - tile_y;

  if (SCREEN_BPP == 1)
    color = 7;
  else
    color = MAKECOLOR16(192,192,192);

  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++) {
      draw_hline(start_x + x * tile_w, start_y + y * tile_h, tile_w, color);
      draw_vline(start_x + x * tile_w, start_y + y * tile_h, tile_h, color);
    }
}



#if 0
static void double_screen(int screen_w, int screen_h)
{
  int y;

#ifdef USE_ASM

  char *dest;

  dest = img->data + 2 * (DISP_Y * img->bytes_per_line + DISP_X);
  for (y = DISP_Y; y < screen_h + DISP_Y; y++) {
    double_memcpy(dest, tmp->line[y] + DISP_X, screen_w);
    single_memcpy(dest + img->bytes_per_line, dest, 2 * screen_w);
    dest += 2 * img->bytes_per_line;
  }

#else

  int x;
  char *dest, *src, *line;

  for (y = DISP_Y; y < screen_h + DISP_Y; y++) {
    src = tmp->line[y] + DISP_X;
    line = dest = img->data + 2 * (img->bytes_per_line * y + DISP_X);
    for (x = 0; x < screen_w; x++) {
      register char c;

      c = *src++;
      *dest++ = c;
      *dest++ = c;
    }
    memcpy(line + img->bytes_per_line, line, 2 * screen_w);
  }

#endif
}
#else
static void double_screen(int SCREEN_W, int SCREEN_H)
{
  int y;

#ifdef USE_ASM

  if (SCREEN_BPP == 1) {
    char *dest;

    dest = img->data + img->bytes_per_line * (DISP_Y<<1) + (DISP_X<<1);
    for (y = DISP_Y; y < SCREEN_H + DISP_Y; y++) {
      double_memcpy(dest, tmp->line[y] + DISP_X, SCREEN_W);
      single_memcpy(dest + img->bytes_per_line, dest, 2 * SCREEN_W);
      dest += 2 * img->bytes_per_line;
    }
  } else {     /* 2 bpp */
    unsigned short *dest;

    dest = ((unsigned short *) (img->data + img->bytes_per_line * (DISP_Y<<1))
	    + ((DISP_X)<<1));
    for (y = DISP_Y; y < SCREEN_H + DISP_Y; y++) {
      double_memcpy16(dest,
		      ((unsigned short *) tmp->line[y]) + DISP_X,
		      SCREEN_W);
      single_memcpy(((char *) dest) + img->bytes_per_line,
		    dest, SCREEN_W<<2);
      dest += img->bytes_per_line;
    }
  }

#else /* USE_ASM */

  if (SCREEN_BPP == 1) {
    int x;
    char *dest, *line;

    for (y = 0; y < SCREEN_H; y++) {
      unsigned char *src = tmp->line[y + DISP_Y] + DISP_X;
      line = dest = img->data + 2 * (img->bytes_per_line * y);
      for (x = 0; x < SCREEN_W; x++) {
	char c;
	
	c = *src++;
	*dest++ = c;
	*dest++ = c;
      }
      single_memcpy(line + img->bytes_per_line, line, 2 * SCREEN_W);
    }
  } else {    /* 2 bpp */
    int x;
    unsigned short *dest, *src;
    unsigned char *line;

    for (y = 0; y < SCREEN_H; y++) {
      src = ((unsigned short *) (tmp->line[y + DISP_Y])) + DISP_X;
      dest = ((unsigned short *) img->data + (img->bytes_per_line * (y + DISP_Y))) + DISP_X * 2;
      line = (unsigned char *) dest;
      for (x = 0; x < SCREEN_W; x++) {
	unsigned short c;
	
	c = *src++;
	*dest++ = c;
	*dest++ = c;
      }
      single_memcpy(line + img->bytes_per_line, line, 4 * SCREEN_W);
    }
  }

#endif
}

#endif

void display_map(WINDOW *win, int w, int h, IMAGE *image, STATE *s)
{
  img = image->img;
  tmp = s->dbl_bmp;
  if (s->pixel_size == 2)
    dbl_screen = 1;
  else
    dbl_screen = 0;

  n_width = s->map->w;
  n_height = s->map->h;

  if (s->n_bmps == 0 || s->map == NULL) {
    memset(img->data, 0, img->bytes_per_line * img->height);
    return;
  }

  if (dbl_screen) {
    clear_xbitmap(tmp, 0);
    if (s->layers & LAYER_BACKGROUND)
      draw_background(s->x, s->y, w / 2, h / 2,
		      s->map->line, s->bmp, s->n_bmps,
		      s->back_bmp, s->n_back_bmps);

    draw_items(s->x, s->y, w/2, h/2, s->selected_obj, s->map->obj, s->obj_bmp, s->npc_info);

    if (s->layers & LAYER_FOREGROUND)
      draw_foreground(s->x, s->y, w / 2, h / 2, s->map->line, s->bmp, s->n_bmps);
    if (s->layers & LAYER_BLOCKS)
      draw_blocks(s->x, s->y, w / 2, h / 2,
		  s->map->line, s->block_bmp, s->n_block_bmps);
    draw_block_selection(s->x, s->y, w / 2, h / 2, s->map->line, s->block_bmp[0]->w, s->block_bmp[0]->h);
    if (s->layers & LAYER_GRIDLINES)
      draw_grid(s->x, s->y, w / 2, h / 2, s->block_bmp[0]->w, s->block_bmp[0]->h);
    double_screen(w / 2, h / 2);
  } else {
    memset(img->data, 0, img->height * img->bytes_per_line);
    if (s->layers & LAYER_BACKGROUND)
      draw_background(s->x, s->y, w, h, s->map->line, s->bmp, s->n_bmps,
		      s->back_bmp, s->n_back_bmps);

    draw_items(s->x, s->y, w, h, s->selected_obj, s->map->obj, s->obj_bmp, s->npc_info);

    if (s->layers & LAYER_FOREGROUND)
      draw_foreground(s->x, s->y, w, h, s->map->line, s->bmp, s->n_bmps);
    if (s->layers & LAYER_BLOCKS)
      draw_blocks(s->x, s->y, w, h, s->map->line,
		  s->block_bmp, s->n_block_bmps);
    draw_block_selection(s->x, s->y, w, h, s->map->line,
			 s->block_bmp[0]->w, s->block_bmp[0]->h);
    if (s->layers & LAYER_GRIDLINES)
      draw_grid(s->x, s->y, w, h, s->block_bmp[0]->w, s->block_bmp[0]->h);
  }
}
