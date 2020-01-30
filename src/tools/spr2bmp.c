/* spr2bmp.c
 * Copyright (C) 1999 Ricardo R. Massaro
 *
 * SPR to BMP converter.
 */

#include <stdio.h>
#include "bitmap.h"


/* Write a 16bpp XBITMAP image data to a 24bpp bmp file */
static void write_bmp_data(XBITMAP *bmp, FILE *f)
{
  int x, y;

  for (y = bmp->h - 1; y >= 0; y--) {
    int pad = 0;
    unsigned short *p;

    p = (unsigned short *) bmp->line[y];
    for (x = 0; x < bmp->w; x++, p++) {
      unsigned char r, g, b;

      r = (*p >> 11) & 0x1f;
      g = (*p >> 5) & 0x3f;
      b = *p & 0x1f;

      r = (r << 3) | (r >> 2);
      g = (g << 2) | (g >> 4);
      b = (b << 3) | (b >> 2);

      fputc(b, f);
      fputc(g, f);
      fputc(r, f);

      pad += 3;
    }

    pad %= 4;
    while (pad-- > 0)
      fputc(0, f);
  }
}

static int putw(int w, FILE *f)
{
  int b1, b2;

  b1 = (w & 0xFF00) >> 8;
  b2 = w & 0x00FF;
  if (putc(b2,f)==b2)
    if (putc(b1,f)==b1)
      return w;
  return EOF;
}

static long putl(long l, FILE *f)
{
  int b1, b2, b3, b4;

  b1 = (int)((l & 0xFF000000L) >> 24);
  b2 = (int)((l & 0x00FF0000L) >> 16);
  b3 = (int)((l & 0x0000FF00L) >> 8);
  b4 = (int)l & 0x00FF;
  if (putc(b4,f)==b4)
    if (putc(b3,f)==b3)
      if (putc(b2,f)==b2)
        if (putc(b1,f)==b1)
          return l;
  return EOF;
}


/* Write a 16bpp XBITMAP to a 24bpp bmp file.  Returns 1 on error. */
static int write_bmp(XBITMAP *bmp, char *filename)
{
  FILE *f;
  int line_len;

  if ((f = fopen(filename, "wb")) == NULL)
    return 1;


  line_len = bmp->w * 3;
  if (line_len % 4)
    line_len += 4 - line_len % 4;

  putw(0x4D42, f);                     /* bfType ("BM") */
  putl(54 + line_len * bmp->h, f);     /* bfSize */
  putw(0, f);                          /* bfReserved1 */
  putw(0, f);                          /* bfReserved2 */
  putl(54, f);                         /* bfOffBits */

  /* info_header */
  putl(40, f);                    /* biSize */
  putl(bmp->w, f);                /* biWidth */
  putl(bmp->h, f);                /* biHeight */
  putw(1, f);                     /* biPlanes */
  putw(24, f);                    /* biBitCount */
  putl(0, f);                     /* biCompression */
  putl(bmp->w * bmp->h * 3, f);   /* biSizeImage */
  putl(0, f);                     /* biXPelsPerMeter */
  putl(0, f);                     /* biYPelsPerMeter */
  putl(0, f);                     /* biClrUsed */
  putl(0, f);                     /* biClrImportant */

  write_bmp_data(bmp, f);

  fclose(f);
  return 0;
}


int main(int argc, char *argv[])
{
  XBITMAP *bmp[1024];
  int n_bmps, i;
  char filename[256];

  if (argc < 3) {
    printf("USAGE: %s file.spr prefix\n", argv[0]);
    exit(1);
  }

  n_bmps = read_xbitmaps(argv[1], 1024, bmp);
  if (n_bmps <= 0) {
    printf("Can't read `%s'\n", argv[1]);
    exit(1);
  }

  for (i = 0; i < n_bmps; i++) {
    snprintf(filename, sizeof(filename), "%s%03d.bmp", argv[2], i);
    if (write_bmp(bmp[i], filename)) {
      printf("Can't write `%s'\n", filename);
      exit(1);
    }
    destroy_xbitmap(bmp[i]);
  }

  return 0;
}
