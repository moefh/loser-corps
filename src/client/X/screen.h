/* screen.h */

/* Use X11 and MIT shared memory extensions */

#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include <sys/time.h>

#include "vga_keys.h"

#define SCREEN_W   x11_screen_w
#define SCREEN_H   x11_screen_h
#define SCREEN_BPP x11_screen_bpp


extern int x11_screen_w, x11_screen_h, x11_screen_bpp;

extern void (*draw_bitmap)(XBITMAP *bmp, int x, int y);
extern void (*draw_bitmap_darken)(XBITMAP *bmp, int x, int y);
extern void (*draw_bitmap_lighten)(XBITMAP *bmp, int x, int y);
extern void (*draw_bitmap_alpha25)(XBITMAP *bmp, int x, int y);
extern void (*draw_bitmap_alpha50)(XBITMAP *bmp, int x, int y);
extern void (*draw_bitmap_alpha75)(XBITMAP *bmp, int x, int y);
extern void (*draw_bitmap_tr)(XBITMAP *bmp, int x, int y);
extern void (*draw_bitmap_tr_blink)(XBITMAP *bmp, int x, int y);
extern void (*draw_bitmap_tr_displace)(XBITMAP *bmp, int x, int y);
extern void (*draw_bitmap_tr_darken)(XBITMAP *bmp, int x, int y);
extern void (*draw_bitmap_tr_lighten)(XBITMAP *bmp, int x, int y);
extern void (*draw_bitmap_tr_alpha25)(XBITMAP *bmp, int x, int y);
extern void (*draw_bitmap_tr_alpha50)(XBITMAP *bmp, int x, int y);
extern void (*draw_bitmap_tr_alpha75)(XBITMAP *bmp, int x, int y);
extern void (*draw_pixel)(int x, int y, int color);
extern void (*draw_bmp_line)(XBITMAP *bmp, int bx, int by, int x, int y,int w);
extern void (*draw_energy_bar)(int x, int y, int w, int h,
			       int energy, int color);

void fps_init(FPS_INFO *fps);
void fps_count(FPS_INFO *fps);
void fps_draw(int x, int y, FPS_INFO *fps);

void compile_bitmap(XBITMAP *bmp);
void graph_query_bpp(int w, int h, int dbl, int no_scanlines, int shm);
void install_graph(int w, int h, int dbl, int no_scanlines, int shm);
void close_graph(void);
int set_game_palette(unsigned char *palette_data);
int install_game_palette(unsigned char *palette_data);

#define display_screen()   display_screen_delta(0)

void display_screen_delta(int delta);
int clear_screen(void);
int poll_keyboard(void);
