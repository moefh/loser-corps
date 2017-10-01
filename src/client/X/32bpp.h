/* 32bpp.h */

void draw_pixel_32(int x, int y, int color)
{
  ((int4bpp *) (img->data + img->bytes_per_line * y))[x] = color;
}

void draw_pixel_32_dbl(int x, int y, int color)
{
  ((int4bpp *) tmp->line[y])[x] = color;
}


void draw_bmp_line_32(XBITMAP *bmp, int bx, int by, int x, int y, int w)
{
  int4bpp *s, *d;

  s = ((int4bpp *) bmp->line[by]) + bx;
  d = ((int4bpp *) (img->data + img->bytes_per_line * y)) + x;
  memcpy(d, s, w << 2);
}

void draw_bmp_line_32_dbl(XBITMAP *bmp, int bx, int by, int x, int y, int w)
{
  int4bpp *s, *d;

  s = ((int4bpp *) bmp->line[by]) + bx;
  d = ((int4bpp *) tmp->line[y]) + x;
  memcpy(d, s, w << 2);
}



#define blink32(dest, src)   (*(dest) = MAKECOLOR32(255,255,255))

static INLINE void darken32(int4bpp *dest, int c)
{
  unsigned char *s, *d;

  s = (unsigned char *) &c;
  d = (unsigned char *) dest;
  if (*d > *s) *d = *s;
  if (*++d > *++s) *d = *s;
  if (*++d > *++s) *d = *s;
}

static INLINE void lighten32(int4bpp *dest, int c)
{
  unsigned char *s, *d;

  s = (unsigned char *) &c;
  d = (unsigned char *) dest;
  if (*d < *s) *d = *s;
  if (*++d < *++s) *d = *s;
  if (*++d < *++s) *d = *s;
}


static INLINE void alpha32_25(int4bpp *d, int c)
{
#if 0
  c &= 0xccc;
  *d &= 0xccc;
  *d += c<<1;
  *d >>= 2;
#else
  unsigned char *dest = (unsigned char *)d, *src = (unsigned char *)&c;

  dest[0] = (unsigned char) (((3*(unsigned short) dest[0]) + src[0]) / 4);
  dest[1] = (unsigned char) (((3*(unsigned short) dest[1]) + src[1]) / 4);
  dest[2] = (unsigned char) (((3*(unsigned short) dest[2]) + src[2]) / 4);
#endif
}


static INLINE void alpha32_50(int4bpp *d, int c)
{
#if 0
  c &= 0xeee;
  *d &= 0xeee;
  *d += c;
  *d >>= 1;
#else
  unsigned char *dest = (unsigned char *)d, *src = (unsigned char *)&c;

  dest[0] = (unsigned char) ((((unsigned short) dest[0]) + src[0]) / 2);
  dest[1] = (unsigned char) ((((unsigned short) dest[1]) + src[1]) / 2);
  dest[2] = (unsigned char) ((((unsigned short) dest[2]) + src[2]) / 2);
#endif
}

static INLINE void alpha32_75(int4bpp *d, int c)
{
#if 0
  c &= 0xccc;
  *d &= 0xccc;
  *d += (*d << 1) + c;
  *d >>= 2;
#else
  unsigned char *dest = (unsigned char *)d, *src = (unsigned char *)&c;

  dest[0] = (unsigned char) ((dest[0] + 3*(unsigned short) src[0]) / 4);
  dest[1] = (unsigned char) ((dest[1] + 3*(unsigned short) src[1]) / 4);
  dest[2] = (unsigned char) ((dest[2] + 3*(unsigned short) src[2]) / 4);
#endif
}



void draw_bitmap_32(XBITMAP *bmp, int x, int y)
{
  int4bpp *src, *dest;
  int i;

  src = (int4bpp *) bmp->data;
  dest = ((int4bpp *)
	  (((unsigned char *) img->data) + img->bytes_per_line * y) + x);
  for (i = bmp->h; i > 0; i--) {
    single_memcpy(dest, src, bmp->line_w);
    src += bmp->w;
    dest += img->bytes_per_line/4;
  }
}

void draw_bitmap_32_dbl(XBITMAP *bmp, int x, int y)
{
  int4bpp *src, *dest;
  int i;

  src = (int4bpp *) bmp->data;
  dest = ((int4bpp *) tmp->line[y]) + x;
  for (i = bmp->h; i > 0; i--) {
    single_memcpy(dest, src, bmp->line_w);
    src += bmp->w;
    dest += tmp->w;
  }
}


#define def_func32(func)						  \
void draw_bitmap_##func##_32(XBITMAP *bmp, int x, int y)		  \
{									  \
  int4bpp *src, *dest;							  \
  int w, i, j;								  \
									  \
  w = bmp->w;								  \
  src = (int4bpp *) bmp->data;						  \
  dest = ((int4bpp *)							  \
	  (((unsigned char *) img->data) + img->bytes_per_line * y) + x); \
  for (i = bmp->h; i > 0; i--) {					  \
    for (j = w; j > 0; j--)						  \
      func(dest++, *src++);						  \
    dest += img->bytes_per_line/4 - w;					  \
  }									  \
}

def_func32(darken32)
def_func32(lighten32)
def_func32(alpha32_25)
def_func32(alpha32_50)
def_func32(alpha32_75)


#define def_func32_dbl(func)					\
void draw_bitmap_##func##_32_dbl(XBITMAP *bmp, int x, int y)	\
{								\
  int4bpp *src, *dest;						\
  int i, j, w;							\
								\
  w = bmp->w;							\
  src = (int4bpp *) bmp->data;					\
  dest = ((int4bpp *) tmp->line[y]) + x;			\
  for (i = bmp->h; i > 0; i--) {				\
    for (j = w; j > 0; j--)					\
      func(dest++, *src++);					\
    dest += tmp->w - w;						\
  }								\
}

def_func32_dbl(darken32)
def_func32_dbl(lighten32)
def_func32_dbl(alpha32_25)
def_func32_dbl(alpha32_50)
def_func32_dbl(alpha32_75)



void draw_bitmap_tr_displace_32(XBITMAP *bmp, int x, int y)
{
  int i, j, w, h;
  int4bpp *src, *dest;

  w = bmp->w;
  h = bmp->tr_last;
  src = (int4bpp *) bmp->line[bmp->tr_first];
  dest = (((int4bpp *)
	   (((unsigned char *) img->data) + (img->bytes_per_line
					     * (bmp->tr_first+y))) + x));
  for (i = bmp->tr_first; i <= h; i++) {
    for (j = w; j > 0; j--) {
      if (*src != XBMP32_TRANSP)
	*dest = *(dest + (j & 4) + ((j & 2) ? (img->bytes_per_line >> 2) : 0));
      src++;
      dest++;
    }
    dest += (img->bytes_per_line >> 2) - w;
  }
}

void draw_bitmap_tr_32(XBITMAP *bmp, int x, int y)
{
  int i, j, w, h;
  int4bpp *src, *dest;

  w = bmp->w;
  h = bmp->tr_last;
  src = (int4bpp *) bmp->line[bmp->tr_first];
  dest = (((int4bpp *)
	   (((unsigned char *) img->data) + (img->bytes_per_line
					     * (bmp->tr_first+y))) + x));
  for (i = bmp->tr_first; i <= h; i++) {
    for (j = w; j > 0; j--) {
      if (*src != XBMP32_TRANSP)
	*dest = *src;
      src++;
      dest++;
    }
    dest += (img->bytes_per_line >> 2) - w;
  }
}

void draw_bitmap_tr_displace_32_dbl(XBITMAP *bmp, int x, int y)
{
  int i, j, w, h;
  int4bpp *src, *dest;

  w = bmp->w;
  h = bmp->tr_last;
  src = (int4bpp *) bmp->line[bmp->tr_first];
  dest = ((int4bpp *) tmp->line[y + bmp->tr_first]) + x;
  for (i = bmp->tr_first; i <= h; i++) {
    for (j = w; j > 0; j--) {
      if (*src != XBMP32_TRANSP)
	*dest = *(dest + (j & 4) + ((j & 2) ? tmp->w : 0));
      src++;
      dest++;
    }
    dest += tmp->w - w;
  }
}

void draw_bitmap_tr_32_dbl(XBITMAP *bmp, int x, int y)
{
  int i, j, w, h;
  int4bpp *src, *dest;

  w = bmp->w;
  h = bmp->tr_last;
  src = (int4bpp *) bmp->line[bmp->tr_first];
  dest = ((int4bpp *) tmp->line[y + bmp->tr_first]) + x;
  for (i = bmp->tr_first; i <= h; i++) {
    for (j = w; j > 0; j--) {
      if (*src != XBMP32_TRANSP)
	*dest = *src;
      src++;
      dest++;
    }
    dest += tmp->w - w;
  }
}

#define def_func32_tr_dbl(func)					\
void draw_bitmap_tr_##func##_32_dbl(XBITMAP *bmp, int x, int y)	\
{								\
  int i, j, w;							\
  int4bpp *src, *dest;						\
								\
  w = bmp->w;							\
  src = (int4bpp *) bmp->data;					\
  dest = ((int4bpp *) tmp->line[y]) + x;			\
  for (i = bmp->h; i > 0; i--) {				\
    for (j = w; j > 0; j--) {					\
      if (*src != XBMP32_TRANSP)				\
          func(dest, *src);					\
      dest++;							\
      src++;							\
    }								\
    dest += tmp->w - w;						\
  }								\
}

def_func32_tr_dbl(darken32)
def_func32_tr_dbl(lighten32)
def_func32_tr_dbl(alpha32_25)
def_func32_tr_dbl(alpha32_50)
def_func32_tr_dbl(alpha32_75)
def_func32_tr_dbl(blink32)


#define def_func32_tr(func)						    \
void draw_bitmap_tr_##func##_32(XBITMAP *bmp, int x, int y)		    \
{									    \
  int i, j, w;								    \
  int4bpp *src, *dest;							    \
									    \
  w = bmp->w;								    \
  src = (int4bpp *) bmp->data;						    \
  for (i = 0; i < bmp->h; i++) {					    \
    dest = (((int4bpp *)						    \
	     (((unsigned char *) img->data) + img->bytes_per_line * (i+y))) \
	    + x);							    \
    for (j = w; j > 0; j--) {						    \
      if (*src != XBMP32_TRANSP)					    \
	func(dest, *src);						    \
      src++;								    \
      dest++;								    \
    }									    \
  }									    \
}

def_func32_tr(darken32)
def_func32_tr(lighten32)
def_func32_tr(alpha32_25)
def_func32_tr(alpha32_50)
def_func32_tr(alpha32_75)
def_func32_tr(blink32)


void draw_energy_bar_32(int x, int y, int w, int h, int energy, int color)
{
  int i, scr_w;
  int4bpp *dest;

  if (energy <= 0) {
    gr_printf(x + 5, y + (h - 8) / 2 + 1, font_8x8, "RIP");
    return;
  }
  if (energy > 100)
    energy = 100;
  energy = (w * energy) / 100;

  scr_w = img->bytes_per_line / 4;
  dest = ((int4bpp *) img->data) + scr_w * y + x;
  for (i = h; i > 0; i--) {
    int j;
#if 0
    int j, r, g, b;

    r = (color >> 11) << 3;
    g = ((color >> 5) & 0x3f) << 2;
    b = (color & 0x1f) << 3;
    r -= 2 * w;
    g -= 2 * w;
    b -= 2 * w;
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
#else
    dest++;
    for (j = 0; j < energy; j++)
      *dest++ = MAKECOLOR32(255, 255, 255);
#endif
    for (; j < w; j++)
      dest++;
    dest += scr_w - (w+1);
  }
}

void draw_energy_bar_32_dbl(int x, int y, int w, int h, int energy, int color)
{
  int i, scr_w;
  int4bpp *dest;

  if (energy <= 0) {
    gr_printf(x + 1, y + (h - 8) / 2, font_8x8, "RIP");
    return;
  }
  if (energy > 100)
    energy = 100;
  energy = (energy * w) / 100;
  scr_w = tmp->w;
  dest = ((int4bpp *) tmp->line[y]) + x;
  for (i = h; i > 0; i--) {
    int j;
#if 0
    int j, r, g, b;

    r = (color >> 11) << 3;
    g = ((color >> 5) & 0x3f) << 2;
    b = (color & 0x1f) << 3;
    r -= 2 * w;
    g -= 2 * w;
    b -= 2 * w;
    
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
#else
    dest++;
    for (j = 0; j < energy; j++)
      *dest++ = MAKECOLOR32(255, 255, 255);
#endif
    for (; j < w; j++)
      dest++;
    dest += scr_w - (w+1);
  }
}


void use_32bpp_dbl(void)
{
  draw_pixel = draw_pixel_32_dbl;
  draw_bmp_line = draw_bmp_line_32_dbl;
  draw_bitmap = draw_bitmap_32_dbl;
  draw_bitmap_darken = draw_bitmap_darken32_32_dbl;
  draw_bitmap_lighten = draw_bitmap_lighten32_32_dbl;
  draw_bitmap_alpha25 = draw_bitmap_alpha32_25_32_dbl;
  draw_bitmap_alpha50 = draw_bitmap_alpha32_50_32_dbl;
  draw_bitmap_alpha75 = draw_bitmap_alpha32_75_32_dbl;
  draw_bitmap_tr = draw_bitmap_tr_32_dbl;
  draw_bitmap_tr_blink = draw_bitmap_tr_blink32_32_dbl;
  draw_bitmap_tr_displace = draw_bitmap_tr_displace_32_dbl;
  draw_bitmap_tr_darken = draw_bitmap_tr_darken32_32_dbl;
  draw_bitmap_tr_lighten = draw_bitmap_tr_lighten32_32_dbl;
  draw_bitmap_tr_alpha25 = draw_bitmap_tr_alpha32_25_32_dbl;
  draw_bitmap_tr_alpha50 = draw_bitmap_tr_alpha32_50_32_dbl;
  draw_bitmap_tr_alpha75 = draw_bitmap_tr_alpha32_75_32_dbl;
  draw_energy_bar = draw_energy_bar_32_dbl;
}

void use_32bpp(void)
{
  draw_pixel = draw_pixel_32;
  draw_bmp_line = draw_bmp_line_32;
  draw_bitmap = draw_bitmap_32;
  draw_bitmap_darken = draw_bitmap_darken32_32;
  draw_bitmap_lighten = draw_bitmap_lighten32_32;
  draw_bitmap_alpha25 = draw_bitmap_alpha32_25_32;
  draw_bitmap_alpha50 = draw_bitmap_alpha32_50_32;
  draw_bitmap_alpha75 = draw_bitmap_alpha32_75_32;
  draw_bitmap_tr = draw_bitmap_tr_32;
  draw_bitmap_tr_blink = draw_bitmap_tr_blink32_32;
  draw_bitmap_tr_displace = draw_bitmap_tr_displace_32;
  draw_bitmap_tr_darken = draw_bitmap_tr_darken32_32;
  draw_bitmap_tr_lighten = draw_bitmap_tr_lighten32_32;
  draw_bitmap_tr_alpha25 = draw_bitmap_tr_alpha32_25_32;
  draw_bitmap_tr_alpha50 = draw_bitmap_tr_alpha32_50_32;
  draw_bitmap_tr_alpha75 = draw_bitmap_tr_alpha32_75_32;
  draw_energy_bar = draw_energy_bar_32;
}

