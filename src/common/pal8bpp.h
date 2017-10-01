/* pal8bpp.h */

#ifndef PAL8BPP_H_FILE
#define PAL8BPP_H_FILE

extern unsigned char palette_data[3 * 256];

extern unsigned char best_fit_map[32][32][32];
extern unsigned char lighten_map[256][256];
extern unsigned char darken_map[256][256];
extern unsigned char alpha25_map[256][256];
extern unsigned char alpha50_map[256][256];
extern unsigned char alpha75_map[256][256];

#define MAKECOLOR8(r, g, b)  best_fit_map[(r)>>3][(g)>>3][(b)>>3]

int calc_install_palette(void);

#endif
