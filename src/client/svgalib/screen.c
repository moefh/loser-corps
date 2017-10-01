/* screen.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

#include "../game.h"

GraphicsContext *screen_gc;
static GraphicsContext *virtual_gc;
static int got_keyboard = 0;

int screen_bpp;

void compile_bitmap(XBITMAP *bmp) { }


/* Draw part of a bitmap line on the screen.  This is used for
 * parallax drawing. */
void draw_bmp_line(XBITMAP *bmp, int bx, int by, int x, int y, int w)
{
  unsigned char *src, *dest;

  src = bmp->line[by] + bx;
  dest = virtual_gc->vbuf + virtual_gc->bytewidth * y + x;
  memcpy(dest, src, w);
}


/* Create virtual screen */
static int create_virtual_screen(int mode, int w, int h, int bpp)
{
  char *graph_mem;

  /* Create the virtual graphics context */
  graph_mem = (char *) malloc(w * h * bpp);
  if (graph_mem == NULL)
    return 1;
  gl_setcontextvirtual(w, h, bpp, 8*bpp, graph_mem);
  virtual_gc = gl_allocatecontext();
  gl_getcontext(virtual_gc);

  gl_setcontext(virtual_gc);
  return 0;
}

/* Displace the screen according to non-transparent parts of the
 * source bitmap. */
void draw_bitmap_tr_displace(XBITMAP *bmp, int x, int y)
{
  int i, j, w;
  char *src, *dest;

  w = bmp->w;
  src = bmp->line[bmp->tr_first];
  for (i = bmp->tr_first; i <= bmp->tr_last; i++) {
    dest = virtual_gc->vbuf + virtual_gc->bytewidth * (i+y) + x;
    for (j = w; j > 0; j--) {
      if (*src)
	*dest = *(dest + (j & 4) + ((j & 2) ? virtual_gc->bytewidth : 0));
      src++;
      dest++;
    }
  }
}


#define darken(dest, c)   *dest = darken_map[c][*dest]
#define lighten(dest, c)  *dest = lighten_map[c][*dest]
#define alpha25(dest, c)  *dest = alpha25_map[c][*dest]
#define alpha50(dest, c)  *dest = alpha50_map[c][*dest]
#define alpha75(dest, c)  *dest = alpha75_map[c][*dest]
#define blink(dest, src)  *dest = MAKECOLOR8(255,255,255)

#define define_func_tr(pixfunc)						\
void draw_bitmap_tr_##pixfunc(XBITMAP *bmp, int x, int y)		\
{									\
  int i, j, w;								\
  unsigned char *src, *dest;						\
									\
  w = bmp->w;								\
  src = bmp->line[bmp->tr_first];					\
  for (i = bmp->tr_first; i <= bmp->tr_last; i++) {			\
    dest = virtual_gc->vbuf + virtual_gc->bytewidth * (i+y) + x;	\
    for (j = w; j > 0; j--) {						\
      if (*src)								\
	pixfunc(dest, *src);						\
      src++;								\
      dest++;								\
    }									\
  }									\
}

define_func_tr(blink)
define_func_tr(lighten)
define_func_tr(darken)
define_func_tr(alpha25)
define_func_tr(alpha50)
define_func_tr(alpha75)

int set_game_palette(unsigned char *palette_data)
{
  if (SCREEN_BPP == 1) {
    int pal_data[3 * 256];
    int i;

    for (i = 0; i < 256; i++) {
      pal_data[3 * i] = palette_data[3 * i] / 4;
      pal_data[3 * i + 1] = palette_data[3 * i + 1] / 4;
      pal_data[3 * i + 2] = palette_data[3 * i + 2] / 4;
    }
    vga_setpalvec(0, 256, pal_data);
    return MAKECOLOR8(255,255,255);
  }
  return MAKECOLOR16(255,255,255);
}

int install_game_palette(unsigned char *palette_data)
{
  int pal_data[3 * 256];
  int i;

  vga_getpalvec(0, 256, pal_data);
  for (i = 0; i < 256; i++) {
    palette_data[3 * i] = pal_data[3 * i] * 4;
    palette_data[3 * i + 1] = pal_data[3 * i + 1] * 4;
    palette_data[3 * i + 2] = pal_data[3 * i + 2] * 4;
  }
  return MAKECOLOR8(255,255,255);
}

static int init_graph(int mode)
{
  if (vga_setmode(mode))
    return 1;
  screen_gc = gl_allocatecontext();
  if (screen_gc == NULL) {
    vga_setmode(TEXT);
    return 1;
  }
  if (gl_setcontextvga(mode) != 0) {
    free(screen_gc);
    vga_setmode(TEXT);
    return 1;
  }
  gl_enableclipping();
  gl_setfont(8, 8, gl_font8x8);
  gl_setwritemode(FONT_COMPRESSED);
  gl_getcontext(screen_gc);

  return 0;
}


static void kb_event_handler(int scancode, int press)
{
  game_key[scancode] = press;
  game_key_trans[scancode] = ((press) ? 1 : -1);

  client_send_key(server, scancode, game_key_trans[scancode]);
}

static void close_keyboard(void)
{
  if (got_keyboard) {
    keyboard_close();
    got_keyboard = 0;
  }
}

int poll_keyboard(void)
{
  keyboard_update();
  return 0;
}

void graph_query_bpp(int gmode)
{
  switch (gmode) {
  case G320x200x256:
  case G320x240x256:
  case G640x480x256:
  case G800x600x256:
  case G1024x768x256:
    SCREEN_BPP = 1;
    break;

  case G320x200x64K:
  case G640x480x64K:
  case G800x600x64K:
  case G1024x768x64K:
    SCREEN_BPP = 2;
    break;

  case G320x200x16M32:
  case G640x480x16M32:
  case G800x600x16M32:
  case G1024x768x16M32:
    SCREEN_BPP = 4;
    convert_16bpp_to = 32;
    break;
  }
}

void install_graph(int gmode)
{
  add_color = 0;
  vga_disabledriverreport();
  if (vga_init() != 0) {
    printf("Can't initialize svgalib.\n");
    exit(1);
  }
  if (init_graph(gmode)) {
    printf("Can't switch to graphics mode.\n");
    exit(1);
  }
  if (create_virtual_screen(gmode, SCREEN_W + 2 * DISP_X,
			    SCREEN_H + 2 * DISP_Y, SCREEN_BPP)) {
    close_graph();
    printf("Out of memory for virtual screen.\n");
    exit(1);
  }
  if (keyboard_init()) {
    printf("Can't open keyboard.\n");
    exit(1);
  }
  got_keyboard = 1;
  atexit(close_keyboard);
  keyboard_seteventhandler(kb_event_handler);
  gl_setwritemode(FONT_COMPRESSED | WRITEMODE_MASKED);
  gl_setfontcolors(0, 15);
}

void close_graph(void)
{
  close_keyboard();
  vga_setmode(TEXT);
}

void draw_energy_bar(int x, int y, int w, int h, int energy, int color)
{
  int i;

  if (energy <= 0) {
    gr_printf(x, y, font_8x8, "RIP");
    return;
  }
  if (energy > 100)
    energy = 100;
  energy = (energy * w) / 100;

  color += add_color;
  for (i = h-1; i >= 0; i--)
    gl_hline(x + 1, y + i, x + energy, color);
}

/* Clear screen; return the white color in the default palette */
int clear_screen(void)
{
  gl_clearscreen(0);
  return (SCREEN_BPP == 1) ? 15 : MAKECOLOR16(255,255,255);
}

void display_screen_delta(int delta)
{
  int i;

  if (n_retraces > 0)
    vga_waitretrace();
#if 1
  gl_setcontext(screen_gc);
  gl_copyboxfromcontext(virtual_gc, DISP_X, DISP_Y, SCREEN_W, SCREEN_H, 0, 0);
  gl_setcontext(virtual_gc);
#else
  gl_copyboxtocontext(DISP_X, DISP_Y, SCREEN_W, SCREEN_H, screen_gc, 0, 0);
#endif
  for (i = 1; i < n_retraces; i++)
    vga_waitretrace();
}
