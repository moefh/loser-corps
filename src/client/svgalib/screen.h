/* screen.h */

#include <vga.h>
#include <vgagl.h>
#include <vgakeyboard.h>

#define SCREEN_W    (screen_gc->width)
#define SCREEN_H    (screen_gc->height)
#define SCREEN_BPP  (screen_bpp)

extern GraphicsContext *screen_gc;
extern int screen_bpp;

void compile_bitmap(XBITMAP *bmp);

int poll_keyboard(void);

void graph_query_bpp(int);
void install_graph(int);
void close_graph(void);
int install_game_palette(unsigned char *);
int set_game_palette(unsigned char *);
int clear_screen(void);
void display_screen_delta(int delta);
void fps_draw(int x, int y, FPS_INFO *fps);
void draw_energy_bar(int x, int y, int w, int h, int energy, int color);
void draw_bitmap_tr_displace(XBITMAP *bmp, int x, int y);
void draw_bitmap_tr_blink(XBITMAP *bmp, int x, int y);
void draw_bmp_line(XBITMAP *bmp, int bx, int by, int x, int y, int w);

#define display_screen()                 display_screen_delta(0)

#define draw_bitmap_lighten(bmp,x,y)     draw_bitmap_tr_lighten((bmp),(x),(y))
#define draw_bitmap_darken(bmp,x,y)      draw_bitmap_tr_darken((bmp),(x),(y))

#if 0
#define draw_bitmap_tr_lighten(bmp,x,y)    draw_bitmap_tr((bmp),(x),(y))
#define draw_bitmap_tr_darken(bmp,x,y)     draw_bitmap_tr((bmp),(x),(y))
#define draw_bitmap_tr_alpha25(bmp, x, y)  draw_bitmap_tr(bmp, x, y)
#define draw_bitmap_tr_alpha50(bmp, x, y)  draw_bitmap_tr(bmp, x, y)
#define draw_bitmap_tr_alpha75(bmp, x, y)  draw_bitmap_tr(bmp, x, y)
#endif

#define draw_bitmap_alpha25(bmp, x, y)     draw_bitmap_tr_alpha25(bmp, x, y)
#define draw_bitmap_alpha50(bmp, x, y)     draw_bitmap_tr_alpha50(bmp, x, y)
#define draw_bitmap_alpha75(bmp, x, y)     draw_bitmap_tr_alpha75(bmp, x, y)

void draw_bitmap_tr_alpha25(XBITMAP *bmp, int x, int y);
void draw_bitmap_tr_alpha50(XBITMAP *bmp, int x, int y);
void draw_bitmap_tr_alpha75(XBITMAP *bmp, int x, int y);
void draw_bitmap_tr_lighten(XBITMAP *bmp, int x, int y);
void draw_bitmap_tr_darken(XBITMAP *bmp, int x, int y);

#define draw_bitmap(bmp, x, y) \
  gl_putbox((x), (y), (bmp)->w, (bmp)->h, (bmp)->data)

#define draw_bitmap_tr(bmp, x, y) \
  gl_putboxmask((x), (y), (bmp)->w, (bmp)->h, (bmp)->data)

#define draw_pixel(x, y, c) \
  gl_setpixel((x), (y), (c))
