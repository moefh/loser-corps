/* pal8bpb.c
 *
 * Copyright (C) 1999 Ricardo R. Massaro
 *
 * Functions for dealing with the palette in 8bpp.
 */

#include <stdio.h>

#include "common.h"

#ifndef SQR
#define SQR(a)   ((a)*(a))
#endif

unsigned char palette_data[3 * 256];

unsigned char best_fit_map[32][32][32];

unsigned char lighten_map[256][256];
unsigned char darken_map[256][256];
unsigned char alpha25_map[256][256];
unsigned char alpha50_map[256][256];
unsigned char alpha75_map[256][256];


static unsigned char calc_best_fit(unsigned char *pal, int r, int g, int b)
{
  int i, best;
  unsigned long dist, tmp;

  best = 1;
  if (pal[3] == r && pal[4] == g && pal[5] == b)
    return best;
  dist = SQR(pal[3] - r) + SQR(pal[4] - g) + SQR(pal[5] - b);

  for (i = 1; i < 256; i++) {
    tmp = SQR(pal[3*i] - r) + SQR(pal[3*i+1] - g) + SQR(pal[3*i+2] - b);
    if (tmp == 0)         /* Exact match */
      return i;
    if (dist > tmp) {
      dist = tmp;
      best = i;
    }
  }
  return best;
}

/* ola */
static void calc_best_fit_map(unsigned char best_fit_map[32][32][32],
			      unsigned char *pal)
{
  int r, g, b;

  for (r = 0; r < 32; r++)
    for (g = 0; g < 32; g++)
      for (b = 0; b < 32; b++)
	best_fit_map[r][g][b] = calc_best_fit(pal, r<<3, g<<3, b<<3);
}

static INLINE unsigned char best_fit(int r, int g, int b)
{
  return best_fit_map[r>>3][g>>3][b>>3];
}

#if 0
static unsigned char calc_lighten(unsigned char pix, unsigned char back)
{
  int r1, g1, b1, r2, g2, b2;

  r1 = palette_data[3 * pix];
  g1 = palette_data[3 * pix + 1];
  b1 = palette_data[3 * pix + 2];
  r2 = palette_data[3 * back];
  g2 = palette_data[3 * back + 1];
  b2 = palette_data[3 * back + 2];

  if (r1 < r2) r1 = r2;
  if (g1 < g2) g1 = g2;
  if (b1 < b2) b1 = b2;

  return best_fit(r1, g1, b1);
}

static unsigned char calc_darken(unsigned char pix, unsigned char back)
{
  int r1, g1, b1, r2, g2, b2;

  r1 = palette_data[3 * pix];
  g1 = palette_data[3 * pix + 1];
  b1 = palette_data[3 * pix + 2];
  r2 = palette_data[3 * back];
  g2 = palette_data[3 * back + 1];
  b2 = palette_data[3 * back + 2];

  if (r1 > r2) r1 = r2;
  if (g1 > g2) g1 = g2;
  if (b1 > b2) b1 = b2;

  return best_fit(r1, g1, b1);
}
#else

static unsigned char calc_lighten(unsigned char pix, unsigned char back)
{
  if ((SQR(palette_data[3*pix])
       + SQR(palette_data[3*pix+1])
       + SQR(palette_data[3*pix+2]))
      > (SQR(palette_data[3*back])
	 + SQR(palette_data[3*back+1])
	 + SQR(palette_data[3*back+2])))
    return pix;
  return back;
}

static unsigned char calc_darken(unsigned char pix, unsigned char back)
{
  if ((SQR(palette_data[3*pix])
       + SQR(palette_data[3*pix+1])
       + SQR(palette_data[3*pix+2]))
      < (SQR(palette_data[3*back])
	 + SQR(palette_data[3*back+1])
	 + SQR(palette_data[3*back+2])))
    return pix;
  return back;
}

static unsigned char calc_alpha25(unsigned char pix, unsigned char back)
{
  int r, g, b;

  r = ((int)palette_data[3 * pix] + 3*(int)palette_data[3 * back]) / 4;
  g = ((int)palette_data[3 * pix + 1] + 3*(int)palette_data[3 * back + 1]) / 4;
  b = ((int)palette_data[3 * pix + 2] + 3*(int)palette_data[3 * back + 2]) / 4;

  return best_fit(r, g, b);
}

static unsigned char calc_alpha50(unsigned char pix, unsigned char back)
{
  int r, g, b;

  r = (palette_data[3 * pix] + (int)palette_data[3 * back]) / 2;
  g = (palette_data[3 * pix + 1] + (int)palette_data[3 * back + 1]) / 2;
  b = (palette_data[3 * pix + 2] + (int)palette_data[3 * back + 2]) / 2;

  return best_fit(r, g, b);
}

static unsigned char calc_alpha75(unsigned char pix, unsigned char back)
{
  int r, g, b;

  r = (3*(int)palette_data[3 * pix] + (int)palette_data[3 * back]) / 4;
  g = (3*(int)palette_data[3 * pix + 1] + (int)palette_data[3 * back + 1]) / 4;
  b = (3*(int)palette_data[3 * pix + 2] + (int)palette_data[3 * back + 2]) / 4;

  return best_fit(r, g, b);
}


#endif

static void calc_map(unsigned char map[256][256],
		     unsigned char (*f)(unsigned char, unsigned char))
{
  int i, j;

  for (i = 0; i < 256; i++)
    for (j = 0; j < 256; j++)
      map[i][j] = f(i, j);
}


/* Install `palette_data' as the default game palette, calculating the
 * maps for lighten, darken and transparency effects. */
int calc_install_palette(void)
{
  int white;

  calc_best_fit_map(best_fit_map, palette_data);  /* This takes a while */

  calc_map(lighten_map, calc_lighten);
  calc_map(darken_map, calc_darken);
  calc_map(alpha25_map, calc_alpha25);
  calc_map(alpha50_map, calc_alpha50);
  calc_map(alpha75_map, calc_alpha75);

  white = best_fit(255,255,255);

  return white;
}
