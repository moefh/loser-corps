/* screen.c */

#include <stdio.h>
#include <allegro.h>

#include "../game.h"

static BITMAP *game_screen[2];
static int game_screen_page;
static signed char my_key_trans[128];

int screen_bpp;

int clear_screen(void)
{
  clear(game_screen[game_screen_page]);

  return (SCREEN_BPP == 1) ? 15 : makecol16(255,255,255);
}

void set_pal_color(int c, int r, int g, int b)
{
  RGB color;

  color.r = r / 4;
  color.g = g / 4;
  color.b = b / 4;
  set_color(c, &color);
}

int install_game_palette(void) { return 15; }

int set_game_palette(unsigned char *palette_data)
{
  if (SCREEN_BPP == 1) {
    RGB pal[256];
    int i;

    for (i = 0; i < 256; i++) {
      pal[i].r = palette_data[3 * i] / 4;
      pal[i].g = palette_data[3 * i + 1] / 4;
      pal[i].b = palette_data[3 * i + 2] / 4;
    }
    set_palette(pal);
    return makecol8(255,255,255);
  }
  return makecol16(255,255,255);
}

void graph_query_bpp(int gmode, int w, int h, int bpp, int flip)
{
  if (allegro_init()) {
    printf("Can't initialize Allegro library.\n");
    exit(1);
  }
  SCREEN_BPP = bpp;
  switch (SCREEN_BPP) {
  case 1: set_color_depth(8); break;
  case 2: set_color_depth(16); break;
  default: printf("Unsupported color depth: %d bytes per pixel\n", bpp);
  }
}

void install_graph(int gmode, int w, int h, int bpp, int flip,
                   int digi_driver, int midi_driver)
{
  set_config_file("play.cfg");

  if (install_timer()) {
    printf("Can't install timer\n");
    exit(1);
  }
  if (install_keyboard()) {
    printf("Can't install keyboard\n");
    exit(1);
  }
  install_sound(digi_driver, midi_driver, "");

  if (set_gfx_mode(gmode, w, h, 0, 0) < 0) {
    printf("Can't switch to graphics mode\n");
    exit(1);
  }
  game_screen[0] = screen;
  game_screen[1] = create_bitmap_ex(SCREEN_BPP * 8,
                                    w + 2 * DISP_X, h * 2 * DISP_Y);
  if (game_screen[1] == NULL) {
    printf("Can't create virtual screen.\n");
    exit(1);
  }
  game_screen_page = 1;
  clear_screen();
}

void close_graph(void)
{
  set_gfx_mode(GFX_TEXT, 80, 25, 0, 0);
}

void draw_bitmap_tr_alpha25(XBITMAP *bmp, int x, int y)
{
  if (SCREEN_BPP == 1)
    draw_bitmap_tr(bmp, x, y);
  else {
    set_trans_blender(0, 0, 0, 64);
    draw_trans_sprite(game_screen[game_screen_page], bmp->allegro_bmp, x, y);
  }
}

void draw_bitmap_tr_alpha50(XBITMAP *bmp, int x, int y)
{
  if (SCREEN_BPP == 1)
    draw_bitmap_tr(bmp, x, y);
  else {
    set_trans_blender(0, 0, 0, 128);
    draw_trans_sprite(game_screen[game_screen_page], bmp->allegro_bmp, x, y);
  }
}

void draw_bitmap_tr_alpha75(XBITMAP *bmp, int x, int y)
{
  if (SCREEN_BPP == 1)
    draw_bitmap_tr(bmp, x, y);
  else {
    set_trans_blender(0, 0, 0, 192);
    draw_trans_sprite(game_screen[game_screen_page], bmp->allegro_bmp, x, y);
  }
}

void draw_bitmap_tr_displace(XBITMAP *bmp, int x, int y)
{
}

void draw_bitmap_tr(XBITMAP *bmp, int x, int y)
{
  if (bmp->compiled != NULL)
    draw_compiled_sprite(game_screen[game_screen_page], bmp->compiled, x, y);
  else
    draw_sprite(game_screen[game_screen_page], bmp->allegro_bmp, x, y);
}

void draw_bitmap(XBITMAP *bmp, int x, int y)
{
  if (bmp->compiled != NULL)
    draw_compiled_sprite(game_screen[game_screen_page], bmp->compiled, x, y);
  else
    blit(bmp->allegro_bmp, game_screen[game_screen_page],
         0, 0, x, y, bmp->w, bmp->h);
}

void compile_bitmap(XBITMAP *bmp)
{
  bmp->compiled = get_compiled_sprite(bmp->allegro_bmp,
                                      (gfx_driver->linear) ? 0 : 1);
  if (bmp->compiled != NULL) {
    int i;

    destroy_bitmap(bmp->allegro_bmp);
    bmp->allegro_bmp = NULL;
    bmp->data = NULL;
    for (i = 0; i < bmp->h; i++)
      bmp->line[i] = NULL;
  }
}

void display_screen(void)
{
  int i;

  if (n_retraces > 0)
    vsync();
  blit(game_screen[game_screen_page], screen, DISP_X, DISP_Y, 0, 0,
       SCREEN_W, SCREEN_H);
  for (i = 1; i < n_retraces; i++)
    vsync();
}

int poll_keyboard(void)
{
  int i;

  for (i = 1; i < 128; i++) {
    if (key[i]) {     /* Key is pressed */
      game_key[i] = 1;
      if (my_key_trans[i] <= 0) {
        my_key_trans[i] = 1;
        game_key_trans[i] = 1;
	client_send_key(server, i, 1);
      }
    } else {            /* Key is not pressed */
      game_key[i] = 0;
      if (my_key_trans[i] >= 0) {
        my_key_trans[i] = -1;
        game_key_trans[i] = -1;
	client_send_key(server, i, -1);
      }
    }
  }
  return 0;
}

void draw_energy_bar(int x, int y, int w, int h, int energy, int color)
{
  if (energy <= 0)
    gr_printf(x, y, font_8x8, "RIP");
  else
    rectfill(game_screen[game_screen_page], x, y,
             x + (energy*w)/100, y + h - 1, color);
}

void draw_pixel(int x, int y, int color)
{
  putpixel(game_screen[game_screen_page], x, y, color);
}
