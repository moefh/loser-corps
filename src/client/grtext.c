/* grtext.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "game.h"


/* Copy a bitmap to another, drawing at (x,y).  This is used mainly
 * for writting text (copying font bitmaps) to other bitmaps. */
void copy_xbitmap(XBITMAP *d_bmp, int x, int y, XBITMAP *s_bmp)
{
  int i;

  if (s_bmp->bpp == 1) {
    unsigned char *dest, *src;

    src = s_bmp->line[0];
    dest = d_bmp->line[y];
    for (i = s_bmp->h; i > 0; i--) {
      memcpy(dest + x, src, s_bmp->line_w);
      dest += d_bmp->w;
      src += s_bmp->w;
    }
  } else if (s_bmp->bpp == 2) {
    int2bpp *dest, *src;

    src = (int2bpp *) s_bmp->line[0];
    dest = (int2bpp *) d_bmp->line[y];
    for (i = s_bmp->h; i > 0; i--) {
      memcpy(dest + x, src, s_bmp->line_w);
      dest += d_bmp->w;
      src += s_bmp->w;
    }
  } else {
    int4bpp *dest, *src;

    src = (int4bpp *) s_bmp->line[0];
    dest = (int4bpp *) d_bmp->line[y];
    for (i = s_bmp->h; i > 0; i--) {
      memcpy(dest + x, src, s_bmp->line_w);
      dest += d_bmp->w;
      src += s_bmp->w;
    }
  }
}


/* Print a text with the given font at the given position of the screen */
void gr_printf(int x, int y, BMP_FONT *font, const char *fmt, ...)
{
  char str[256], *p;
  int pos;
  va_list ap;

  va_start(ap, fmt);
  vsprintf(str, fmt, ap);
  va_end(ap);

  pos = 0;
  for (p = str; *p != '\0'; p++) {
    draw_bitmap_tr(font->bmp[*(unsigned char *)p], x + pos, y);
    pos += font->bmp[*(unsigned char *)p]->w;
  }
}

/* Print a selected text with the given font at the given position of
 * the screen */
void gr_printf_selected(int sel, int x, int y, BMP_FONT *font,
			const char *fmt, ...)
{
  static XBITMAP *bmp;
  char str[256];
  unsigned char *p;
  int pos;
  va_list ap;

  va_start(ap, fmt);
  vsprintf(str, fmt, ap);
  va_end(ap);

  if (bmp == NULL) {
    bmp = create_xbitmap(20, 20, SCREEN_BPP);
    if (bmp == NULL)
      return;
  }

  pos = 0;
  for (p = (unsigned char *) str; *p != '\0'; p++) {
    if (SCREEN_BPP == 1) {
      clear_xbitmap(bmp, XBMP8_TRANSP);
      copy_xbitmap(bmp, 0, 0, font->bmp[*p]);
      if (sel)
	color_bitmap(bmp, MAKECOLOR8(255,255,255) + add_color);
      else
	color_bitmap(bmp, MAKECOLOR8(196,196,196) + add_color);
      draw_bitmap_tr(bmp, x + pos, y);
    } else {
      if (sel)
	draw_bitmap_tr(font->bmp[*p], x + pos, y);
      else
	draw_bitmap_tr_alpha(75, font->bmp[*p], x + pos, y);
    }
    pos += font->bmp[*p]->w;
  }
}

/* Draw a text with the specified font in a bitmap at the given
 * location. */
void draw_text(XBITMAP *dest, BMP_FONT *font, int x, int y, char *txt)
{
  int pos;
  char *p;

  pos = x;
  for (p = txt; *p != '\0'; p++) {
    if (pos + font->bmp[*(unsigned char *)p]->w > dest->w)
      break;
    copy_xbitmap(dest, pos, y, font->bmp[*(unsigned char *)p]);
    pos += font->bmp[*(unsigned char *)p]->w;
  }
}


static int has_key(void)
{
  game_key_trans[SCANCODE_SPACE] = 0;
  if (poll_keyboard())
    return -1;
  if (game_key_trans[SCANCODE_SPACE] > 0)
    return 1;
  return 0;
}

static void wait_for_key(void)
{
  int r;

  while (1) {
    if ((r = has_key()) < 0)
      return;
    if (r == 0)
      break;
  }

  while (1) {
    if ((r = has_key()) < 0)
      return;
    if (r > 0)
      break;
  }
}


static void gr_print_center(int y, BMP_FONT *font, char *str)
{
  int x;

  x = (SCREEN_W - font->bmp[' ']->w * strlen(str)) / 2;
  gr_printf(DISP_X + x, y, font, "%s", str);
}

static void gr_print_break(int y, BMP_FONT *font, char *orig_str)
{
  char str[1024];
  char *last, *p;
  int last_space, len;
  int lines;

  strcpy(str, orig_str);
  last = str;
  last_space = 0;
  len = strlen(str);
  lines = 0;

  for (p = str; p < str + len; p++)
    if (*p == ' ' || *p == '\n') {
      if ((p - str) / 34 != last_space / 34) {
	char *old_p;

	old_p = p;
	p = str + last_space;
	*p = '\0';
	gr_print_center(y + 10 * lines, font, last);
	last = p + 1;
	lines++;
	last_space = old_p - str;
      } else
	last_space = p - str;
    }
  gr_print_center(y + 10 * lines, font, last);
}


/* Print an error message for startup errors and wait for any key */
void start_error_msg(char *fmt, ...)
{
  static char str[1024];
  va_list ap;

  va_start(ap, fmt);
  vsprintf(str, fmt, ap);
  va_end(ap);

  if (font_8x8 != NULL) {
    clear_screen();
    gr_print_break(DISP_Y + SCREEN_H / 3 - font_8x8->bmp[' ']->h,
		   font_8x8, str);
    display_screen();
    wait_for_key();
  } else {
    printf("%s", str);
    fflush(stdout);
  }
}

/* Ask to the user if he/she wants to create the 8bpp files.  If no,
 * just fail (return 0).  Otherwise, run the command "./quantize" and
 * return 1 if OK or 0 if it failed. */
int create_8bpp_files(void)
{
  int create_images = 0;

  clear_screen();
  if (SCREEN_BPP == 1)
    color_font(font_8x8, MAKECOLOR8(255,255,255));
  gr_print_break(DISP_Y + SCREEN_H / 3 - 10, font_8x8,
		 "Hmmm... Looks like you don't have the 8bpp version of the "
		 "images on your system.\n");
  gr_print_break(DISP_Y + SCREEN_H / 3 + 40, font_8x8,
		 "Would you like to create them now? (y/n)");
  display_screen();

  game_key_trans[SCANCODE_Y] = 0;
  game_key_trans[SCANCODE_N] = 0;
  while (1) {
    if (poll_keyboard() || game_key_trans[SCANCODE_N] > 0)
      break;
    if (game_key_trans[SCANCODE_Y] > 0) {
      create_images = 1;
      break;
    }
  }

  if (create_images) {
    clear_screen();
    gr_print_break(DISP_Y + SCREEN_H / 3, font_8x8,
		   "Quantizing sprites for 8bpp");
    gr_print_break(DISP_Y + SCREEN_H / 3 + 10, font_8x8,
		   "(this may take a while...)");
    display_screen();
    poll_keyboard();
    usleep(10000);
    /* call quantize as follows:
     * quantize <location-of-quant-executable> <location-of-data>
     */
    if (system(DATADIR "/quantize " \
               LIBDIR "/quant " \
               DATADIR "/data #> /dev/null"))
      return 0;
    return 1;
  }

  return 0;
}
