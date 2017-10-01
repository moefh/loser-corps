/* draw.h */

#ifndef DRAW_H_FILE
#define DRAW_H_FILE

extern XBITMAP *parallax_bmp;

void draw_parallax_tile(int x, int y, int w, int h, int sx, int sy);

void draw_bitmap_alpha(int alpha, XBITMAP *bmp, int x, int y);
void draw_bitmap_tr_alpha(int alpha, XBITMAP *bmp, int x, int y);
void draw_foreground(void);
void draw_foreground_alpha(int alpha);
void draw_background(void);
void draw_background_alpha(int alpha);

#endif /* DRAW_H_FILE */
