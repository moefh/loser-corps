/* 8bpp.h */


#define blink8(dest, src)   (*(dest) = MAKECOLOR8(255,255,255))

extern unsigned char darken_map[256][256];
extern unsigned char lighten_map[256][256];
extern unsigned char alpha25_map[256][256];
extern unsigned char alpha50_map[256][256];
extern unsigned char alpha75_map[256][256];

#if 1
static INLINE void darken8(unsigned char *dest, int c)
{
  *dest = darken_map[c][*dest];
}

static INLINE void lighten8(unsigned char *dest, int c)
{
  *dest = lighten_map[c][*dest];
}

static INLINE void alpha8_25(unsigned char *dest, int c)
{
  *dest = alpha25_map[c][*dest];
}

static INLINE void alpha8_50(unsigned char *dest, int c)
{
  *dest = alpha50_map[c][*dest];
}

static INLINE void alpha8_75(unsigned char *dest, int c)
{
  *dest = alpha75_map[c][*dest];
}

#else
static INLINE void darken8(unsigned char *dest, int c)
{
  unsigned char r, g, b, x, y, z;

  r = *dest & 0xe0;
  g = *dest & 0x1c;
  b = *dest & 0x02;
  x = c & 0xe0;
  y = c & 0x1c;
  z = c & 0x02;
  if (r > x)
    r = x;
  if (g > y)
    g = y;
  if (b > z)
    b = z;
  *dest = (r & 0xe0) | (g & 0x1c) | (b & 0x3);
}

static INLINE void lighten8(unsigned char *dest, int c)
{
  unsigned char r, g, b, tmp;

  r = *dest & 0xe0;
  g = *dest & 0x1c;
  b = *dest & 0x02;
  if (r < (tmp = c & 0xe0))
    r = tmp;
  if (g < (tmp = c & 0x1c))
    g = tmp;
  if (b < (tmp = c & 0x02))
    b = tmp;
  *dest = r | g | b;
}

static INLINE void alpha8_25(unsigned char *dest, int c)
{
  unsigned short r, g, b;

  r = *dest & 0xe0;
  r = ((r<<1)+r + (c & 0xe0)) >> 2;
  g = *dest & 0x1c;
  g = ((g<<1)+g + (c & 0x1c)) >> 2;
  b = (*dest & 0x02);
  b = ((b<<1)+b + (c & 0x03)) >> 2;
  *dest = (r & 0xe0) | (g & 0x1c) | (b & 0x3);
}

static INLINE void alpha8_50(unsigned char *dest, int c)
{
  unsigned short r, g, b;
  r = (((short)*dest & 0xe0) + (c & 0xe0)) >> 1;
  g = (((short)*dest & 0x1c) + (c & 0x1c)) >> 1;
  b = (((short)*dest & 0x03) + (c & 0x03)) >> 1;
  *dest = (r & 0xe0) | (g & 0x1c) | (b & 0x3);
}

static INLINE void alpha8_75(unsigned char *dest, int c)
{
  unsigned short r, g, b;

  r = c & 0xe0;
  r = (((short)*dest & 0xe0) + (r<<1)+r) >> 2;
  g = c & 0x1c;
  g = (((short)*dest & 0x1c) + (g<<1)+g) >> 2;
  b = c & 0x02;
  b = (((short)*dest & 0x02) + (b<<1)+b) >> 2;
  *dest = (r & 0xe0) | (g & 0x1c) | (b & 0x3);
}
#endif


#define def_func8(func)							  \
void draw_bitmap_##func##_8(XBITMAP *bmp, int x, int y)			  \
{									  \
  unsigned char *src, *dest;						  \
  int w, i, j;								  \
									  \
  w = bmp->w;								  \
  src = bmp->data;					                  \
  dest = (((unsigned char *) img->data) + img->bytes_per_line * y) + x;   \
  for (i = bmp->h; i > 0; i--) {					  \
    for (j = w; j > 0; j--)						  \
      func(dest++, *src++);						  \
    dest += img->bytes_per_line/2 - w;					  \
  }									  \
}

def_func8(darken8)
def_func8(lighten8)
def_func8(alpha8_25)
def_func8(alpha8_50)
def_func8(alpha8_75)


#define def_func8_dbl(func)					\
void draw_bitmap_##func##_8_dbl(XBITMAP *bmp, int x, int y)	\
{								\
  unsigned char *src, *dest;					\
  int i, j, w;							\
								\
  w = bmp->w;							\
  src = bmp->data;				                \
  dest = tmp->line[y] + x;              			\
  for (i = bmp->h; i > 0; i--) {				\
    for (j = w; j > 0; j--)					\
      func(dest++, *src++);                                     \
    dest += tmp->w - w;						\
  }								\
}

def_func8_dbl(darken8)
def_func8_dbl(lighten8)
def_func8_dbl(alpha8_25)
def_func8_dbl(alpha8_50)
def_func8_dbl(alpha8_75)


#define def_func8_tr_dbl(func)						\
void draw_bitmap_tr_##func##_8_dbl(XBITMAP *bmp, int x, int y)	        \
{									\
  int i, j, w;								\
  unsigned char *src, *dest;						\
									\
  w = bmp->w;								\
  src = bmp->data;                					\
  dest = tmp->line[y] + x;                				\
  for (i = bmp->h; i > 0; i--) {					\
    for (j = w; j > 0; j--) {						\
      if (*src != XBMP8_TRANSP) 					\
          func(dest, *src);      					\
      dest++;								\
      src++;								\
    }									\
    dest += tmp->w - w;							\
  }									\
}

def_func8_tr_dbl(darken8)
def_func8_tr_dbl(lighten8)
def_func8_tr_dbl(alpha8_25)
def_func8_tr_dbl(alpha8_50)
def_func8_tr_dbl(alpha8_75)
def_func8_tr_dbl(blink8)

#define def_func8_tr(func)					\
void draw_bitmap_tr_##func##_8(XBITMAP *bmp, int x, int y)	\
{								\
  int i, j, w;							\
  unsigned char *src, *dest;					\
								\
  w = bmp->w;							\
  src = bmp->data;						\
  for (i = 0; i < bmp->h; i++) {				\
    dest = ((unsigned char *) img->data) + img->bytes_per_line * (i+y) + x;		\
    for (j = w; j > 0; j--) {					\
      if (*src != XBMP8_TRANSP)					\
	func(dest, *src);					\
      src++;							\
      dest++;							\
    }								\
  }								\
}

def_func8_tr(darken8)
def_func8_tr(lighten8)
def_func8_tr(alpha8_25)
def_func8_tr(alpha8_50)
def_func8_tr(alpha8_75)
def_func8_tr(blink8)


void draw_bmp_line_8(XBITMAP *bmp, int bx, int by, int x, int y, int w)
{
  unsigned char *s, *d;

  s = ((unsigned char *) bmp->line[by]) + bx;
  d = ((unsigned char *) (img->data + img->bytes_per_line * y)) + x;
  memcpy(d, s, w);
}

void draw_bmp_line_8_dbl(XBITMAP *bmp, int bx, int by, int x, int y, int w)
{
  unsigned char *s, *d;

  s = ((unsigned char *) bmp->line[by]) + bx;
  d = ((unsigned char *) tmp->line[y]) + x;
  memcpy(d, s, w);
}


void draw_pixel_8(int x, int y, int color)
{
  img->data[img->bytes_per_line * y + x] = color;
}

void draw_pixel_8_dbl(int x, int y, int color)
{
  tmp->line[y][x] = color;
}

void draw_bitmap_8(XBITMAP *bmp, int x, int y)
{
  unsigned char *src, *dest;
  int i;

  src = bmp->data;
  dest = (unsigned char *)img->data + img->bytes_per_line * y + x;
  for (i = bmp->h; i > 0; i--) {
    single_memcpy(dest, src, bmp->w);
    src += bmp->w;
    dest += img->bytes_per_line;
  }
}

void draw_bitmap_8_dbl(XBITMAP *bmp, int x, int y)
{
  unsigned char *src, *dest;
  int i;

  src = bmp->data;
  dest = tmp->line[y] + x;
  for (i = bmp->h; i > 0; i--) {
    single_memcpy(dest, src, bmp->w);
    src += bmp->w;
    dest += tmp->w;
  }
}

void draw_bitmap_tr_displace_8(XBITMAP *bmp, int x, int y)
{
  int i, j, w;
  unsigned char *src, *dest;

  w = bmp->w;
  src = bmp->line[bmp->tr_first];
  for (i = bmp->tr_first; i <= bmp->tr_last; i++) {
    dest = (unsigned char *)img->data + img->bytes_per_line * (i+y) + x;
    for (j = w; j > 0; j--) {
      if (*src != XBMP8_TRANSP)
	*dest = *(dest + (j & 4) + ((j & 2) ? img->bytes_per_line : 0));
      src++;
      dest++;
    }
  }
}

void draw_bitmap_tr_8(XBITMAP *bmp, int x, int y)
{
  int i, j, w;
  unsigned char *src, *dest;

  w = bmp->w;
  src = bmp->line[bmp->tr_first];
  for (i = bmp->tr_first; i <= bmp->tr_last; i++) {
    dest = (unsigned char *)img->data + img->bytes_per_line * (i+y) + x;
    for (j = w; j > 0; j--) {
      if (*src != XBMP8_TRANSP)
	*dest = *src;
      src++;
      dest++;
    }
  }
}

void draw_bitmap_tr_displace_8_dbl(XBITMAP *bmp, int x, int y)
{
  int i, j, w;
  unsigned char *src, *dest;

  w = bmp->w;
  src = bmp->line[bmp->tr_first];
  dest = tmp->line[y + bmp->tr_first] + x;
  for (i = bmp->tr_first; i <= bmp->tr_last; i++) {
    for (j = w; j > 0; j--) {
      if (*src != XBMP8_TRANSP)
	*dest = *(dest + (j & 4) + ((j & 2) ? tmp->w : 0));
      src++;
      dest++;
    }
    dest += tmp->w - w;
  }
}

void draw_bitmap_tr_8_dbl(XBITMAP *bmp, int x, int y)
{
  int i, j, w;
  unsigned char *src, *dest;

  w = bmp->w;
  src = bmp->line[bmp->tr_first];
  dest = tmp->line[y + bmp->tr_first] + x;
  for (i = bmp->tr_first; i <= bmp->tr_last; i++) {
    for (j = w; j > 0; j--) {
      if (*src != XBMP8_TRANSP)
	*dest = *src;
      src++;
      dest++;
    }
    dest += tmp->w - w;
  }
}

void draw_energy_bar_8(int x, int y, int w, int h, int energy, int color)
{
  int i, scr_w;
  unsigned char *dest;

  if (energy <= 0) {
    gr_printf(x + 1, y + (h - 8) / 2, font_8x8, "RIP");
    return;
  }
  if (energy > 100)
    energy = 100;
  w = (w * energy) / 100;

  color += add_color;

  scr_w = img->bytes_per_line;
  dest = (unsigned char *)img->data + scr_w * y + x;
  for (i = h; i > 0; i--) {
    if (w > 0)
      memset(dest + 1, color, w);
    dest += scr_w;
  }
}

void draw_energy_bar_8_dbl(int x, int y, int w, int h, int energy, int color)
{
  int i, scr_w;
  unsigned char *dest;

  if (energy <= 0) {
    gr_printf(x + 5, y + (h - 8) / 2 + 1, font_8x8, "RIP");
    return;
  }
  if (energy > 100)
    energy = 100;
  w = (w * energy) / 100;

  color += add_color;
  scr_w = tmp->w;
  dest = tmp->line[y] + x;
  for (i = h; i > 0; i--) {
    if (w > 0)
      memset(dest + 1, color, w);
    dest += scr_w;
  }
}



void use_8bpp_dbl(void)
{
  draw_pixel = draw_pixel_8_dbl;
  draw_bmp_line = draw_bmp_line_8_dbl;
  draw_bitmap = draw_bitmap_8_dbl;
#if 1
  draw_bitmap_darken = draw_bitmap_darken8_8_dbl;
  draw_bitmap_lighten = draw_bitmap_lighten8_8_dbl;
  draw_bitmap_alpha25 = draw_bitmap_alpha8_25_8_dbl;
  draw_bitmap_alpha50 = draw_bitmap_alpha8_50_8_dbl;
  draw_bitmap_alpha75 = draw_bitmap_alpha8_75_8_dbl;
#else
  draw_bitmap_darken = draw_bitmap_darken8_8_dbl;
  draw_bitmap_lighten = draw_bitmap_lighten8_8_dbl;
  draw_bitmap_alpha25 = draw_bitmap_8_dbl;
  draw_bitmap_alpha50 = draw_bitmap_8_dbl;
  draw_bitmap_alpha75 = draw_bitmap_8_dbl;
#endif
  draw_bitmap_tr = draw_bitmap_tr_8_dbl;
  draw_bitmap_tr_blink = draw_bitmap_tr_blink8_8_dbl;
  draw_bitmap_tr_displace = draw_bitmap_tr_displace_8_dbl;
#if 1
  draw_bitmap_tr_darken = draw_bitmap_tr_darken8_8_dbl;
  draw_bitmap_tr_lighten = draw_bitmap_tr_lighten8_8_dbl;
  draw_bitmap_tr_alpha25 = draw_bitmap_tr_alpha8_25_8_dbl;
  draw_bitmap_tr_alpha50 = draw_bitmap_tr_alpha8_50_8_dbl;
  draw_bitmap_tr_alpha75 = draw_bitmap_tr_alpha8_75_8_dbl;
#else
  draw_bitmap_tr_darken = draw_bitmap_tr_darken8_8_dbl;
  draw_bitmap_tr_lighten = draw_bitmap_tr_lighten8_8_dbl;
  draw_bitmap_tr_alpha25 = draw_bitmap_tr_8_dbl;
  draw_bitmap_tr_alpha50 = draw_bitmap_tr_8_dbl;
  draw_bitmap_tr_alpha75 = draw_bitmap_tr_8_dbl;
#endif
  draw_energy_bar = draw_energy_bar_8_dbl;
}

void use_8bpp(void)
{
  draw_pixel = draw_pixel_8;
  draw_bmp_line = draw_bmp_line_8;
  draw_bitmap = draw_bitmap_8;
  
#if 1
  draw_bitmap_darken = draw_bitmap_darken8_8;
  draw_bitmap_lighten = draw_bitmap_lighten8_8;
  draw_bitmap_alpha25 = draw_bitmap_alpha8_25_8;
  draw_bitmap_alpha50 = draw_bitmap_alpha8_50_8;
  draw_bitmap_alpha75 = draw_bitmap_alpha8_75_8;
#else
  draw_bitmap_darken = draw_bitmap_darken8_8;
  draw_bitmap_lighten = draw_bitmap_lighten8_8;
  draw_bitmap_alpha25 = draw_bitmap_8;
  draw_bitmap_alpha50 = draw_bitmap_8;
  draw_bitmap_alpha75 = draw_bitmap_8;
#endif
  draw_bitmap_tr = draw_bitmap_tr_8;
  draw_bitmap_tr_blink = draw_bitmap_tr_blink8_8;
  draw_bitmap_tr_displace = draw_bitmap_tr_displace_8;
#if 1
  draw_bitmap_tr_darken = draw_bitmap_tr_darken8_8;
  draw_bitmap_tr_lighten = draw_bitmap_tr_lighten8_8;
  draw_bitmap_tr_alpha25 = draw_bitmap_tr_alpha8_25_8;
  draw_bitmap_tr_alpha50 = draw_bitmap_tr_alpha8_50_8;
  draw_bitmap_tr_alpha75 = draw_bitmap_tr_alpha8_75_8;
#else
  draw_bitmap_tr_darken = draw_bitmap_tr_darken8_8;
  draw_bitmap_tr_lighten = draw_bitmap_tr_lighten8_8;
  draw_bitmap_tr_alpha25 = draw_bitmap_tr_8;
  draw_bitmap_tr_alpha50 = draw_bitmap_tr_8;
  draw_bitmap_tr_alpha75 = draw_bitmap_tr_8;
#endif
  draw_energy_bar = draw_energy_bar_8;
}
