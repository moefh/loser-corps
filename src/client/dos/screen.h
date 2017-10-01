/* screen.h */

#include <allegro.h>
#include <sys/time.h>

#include "vga_keys.h"

#if 0
#define SCREEN_W     screen_w
#define SCREEN_H     screen_h
#endif
#define SCREEN_BPP   screen_bpp

extern int screen_bpp;
extern int screen_w, screen_h;

void draw_bitmap(XBITMAP *bmp, int x, int y);
void draw_bitmap_tr(XBITMAP *bmp, int x, int y);
void draw_bitmap_tr_alpha25(XBITMAP *bmp, int x, int y);
void draw_bitmap_tr_alpha50(XBITMAP *bmp, int x, int y);
void draw_bitmap_tr_alpha75(XBITMAP *bmp, int x, int y);
void draw_bitmap_tr_displace(XBITMAP *bmp, int x, int y);

#define draw_bitmap_alpha25(bmp, x, y)         \
  draw_bitmap_tr_alpha25(bmp, x, y)

#define draw_bitmap_alpha50(bmp, x, y)         \
  draw_bitmap_tr_alpha50(bmp, x, y)

#define draw_bitmap_alpha75(bmp, x, y)         \
  draw_bitmap_tr_alpha75(bmp, x, y)


void compile_bitmap(XBITMAP *bmp);

int read_bitmaps(char *filename, int max, XBITMAP **bmp);
XBITMAP *read_bitmap(char *filename);
int write_bitmaps(char *filename, int n, XBITMAP **bmp);
int write_bitmap(char *filename, XBITMAP *bmp);

void install_graph(int mode, int w, int h, int bpp, int flip,
                   int digi_driver, int midi_driver);
void graph_query_bpp(int mode, int w, int h, int bpp, int flip);
int clear_screen(void);
int set_game_palette(unsigned char *pal_data);
int install_game_palette(void);
void close_graph(void);
void display_screen(void);
int poll_keyboard(void);

void draw_energy_bar(int x, int y, int w, int h, int energy, int color);
void draw_pixel(int x, int y, int color);
