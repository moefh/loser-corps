/* credits.c */

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "common.h"
#include "game.h"

#define INLINE_FILE_PREFIX  "!file: "

#define CREDIT_ALPHA       100

#define INLINE_SPR(file)   (INLINE_FILE_PREFIX file)

typedef struct CREDIT_LINE {
  int delta;               /* Space from upper line */
  int alpha;               /* Alpha for this bitmap (25, 50, 75 or 100) */
  int color[2];            /* Text color index (-1 to blink)
			    * color[0] = 8 bit value,
			    * color[1] = 16 bit value */
  char *text;              /* Line text */
  int x, y;                /* Line position (calculated at run-time) */
  XBITMAP *bmp;            /* Bitmap */
  XBITMAP **ani;           /* Animation */
  int frame;               /* Current frame (if an animation) */
  int n_frames;            /* Number of frames (if an animation) */
} CREDIT_LINE;


static int initialized = 0, frame, end_frame;

#define C16(r,g,b)   MAKECOLOR16(r,g,b)


#define CREDIT_TITLE(before, title)				   	\
  { before + 10, CREDIT_ALPHA, { 14, C16(255,255,0) },			\
				 "\x81\x81\x81\x81\x81\x81\x81" }, 	\
  { 10,          CREDIT_ALPHA, { 14, C16(255,255,0) },			\
				 title },				\
  { 10,          CREDIT_ALPHA, { 14, C16(255,255,0) },			\
				 "\x80\x80\x80\x80\x80\x80\x80" }

#define CREDIT_SUBTITLE(subtitle)		\
  { 40, CREDIT_ALPHA, { 14, C16(255,255,255) }, subtitle }

#define CREDIT_AUTHOR(name)			\
  { 15, CREDIT_ALPHA, { -1, -1 }, name }

#define CREDIT_IMAGE(before, file)		\
  { before, CREDIT_ALPHA, { 0, 0 }, INLINE_SPR(file) }

#define CREDIT_IMAGE_ALPHA(before, alpha, file)	\
  { before, alpha, { 0, 0 }, INLINE_SPR(file) }

#define CREDIT_IMAGE_AUTHOR(before, name)	\
  { before, CREDIT_ALPHA, { -1, -1 }, name }

#define CREDIT_TEXT(before, text)		\
  { before, CREDIT_ALPHA, { 23, C16(200,200,200) }, text }

#define CREDIT_END()				\
  { -1, 0, { 0, 0 }, NULL }


#if 1

static CREDIT_LINE credit[] = {
  CREDIT_IMAGE(0, "logo"),

  CREDIT_IMAGE(120, "credits"),

  CREDIT_IMAGE(40,  "main"),
  CREDIT_IMAGE(40,  "fx"),
  CREDIT_IMAGE(40,  "enemy"),
  CREDIT_IMAGE(40,  "mapdsg"),
  CREDIT_IMAGE(40,  "chara"),
  CREDIT_IMAGE(40,  "music"),
  CREDIT_IMAGE(40,  "sound"),

  CREDIT_END(),
};

#else

/***** Old credits *****/
static CREDIT_LINE credit[] = {
  CREDIT_IMAGE_ALPHA(0, 75, "game-logo"),

  CREDIT_TITLE(140, "Credits"),

  CREDIT_SUBTITLE("Programming"),
  CREDIT_AUTHOR("Moe"),

  CREDIT_SUBTITLE("Additional code"),
  CREDIT_AUTHOR("Guto"),
  CREDIT_AUTHOR("Cassio"),

  CREDIT_SUBTITLE("Graphics"),
  CREDIT_AUTHOR("Meio"),

  CREDIT_SUBTITLE("Music"),
  CREDIT_AUTHOR("Pierce"),

  CREDIT_SUBTITLE("Distribution"),
  CREDIT_AUTHOR("Guto"),

  CREDIT_SUBTITLE("Translation"),
  CREDIT_AUTHOR("Shadow"),

  CREDIT_SUBTITLE("Loserboy"),
  CREDIT_IMAGE_ALPHA(10, 50, "loserboy-ani"),
  CREDIT_IMAGE_AUTHOR(50, "by Meio"),

  CREDIT_SUBTITLE("Punkman"),
  CREDIT_IMAGE_ALPHA(10, 50, "punkman-ani"),
  CREDIT_IMAGE_AUTHOR(74, "by Bota"),

  CREDIT_SUBTITLE("Stickman"),
  CREDIT_IMAGE_ALPHA(10, 50, "stickman-ani"),
  CREDIT_IMAGE_AUTHOR(50, "by Moe"),

  CREDIT_TEXT(80, "WAVE server adapted from SoundIt,"),
  CREDIT_TEXT(15, "Copyright 1994 Brad Pitzel"),

  CREDIT_TEXT(30, "MIDI server adapted from Playmidi,"),
  CREDIT_TEXT(15, "Copyright 1994-1996 Nathan I. Laredo"),

  CREDIT_TEXT(30, "Linux Penguin by"),
  CREDIT_TEXT(15, "Larry Ewing"),

  CREDIT_TITLE(70, "Thanks!"),

  CREDIT_TEXT(40, "Cezar"),
  CREDIT_TEXT(15, "for maintaining the Linux network"),
  CREDIT_TEXT(15, "on CEC/IME. It would be very hard to"),
  CREDIT_TEXT(15, "implement network play without it."),

  CREDIT_TEXT(40, "Everyone that tested the game and"),
  CREDIT_TEXT(15, "made comments and suggestions."),

  CREDIT_TEXT(40, "Pink Floyd,"),
  CREDIT_TEXT(15, "Beatles and Rolling Stones"),
  CREDIT_TEXT(15, "for the great musics I listen"),
  CREDIT_TEXT(15, "while I'm programming."),

  CREDIT_TEXT(40, "Special thanks to"),
  CREDIT_TEXT(15, "Mary Joe McKinsky"),

  CREDIT_END()
};
#endif /* 1 */


static void setup_credit_line(CREDIT_LINE *cl)
{
  draw_text(cl->bmp, font_8x8, 0, 0, cl->text);
  if (cl->color >= 0)
    color_bitmap(cl->bmp, add_color + cl->color[(SCREEN_BPP==1) ? 0 : 1]);
  cl->x = (SCREEN_W - cl->bmp->w) / 2;
}

static char *credit_filename(char *text)
{
  static char p[] = INLINE_FILE_PREFIX;

  if (memcmp(text, p, sizeof(p) - 1) == 0)
    return text + sizeof(p) - 1;
  return NULL;
}


static int read_credit_sprite(char *file, CREDIT_LINE *cl)
{
  int i;
  XBITMAP *ani[256];
  char filename[256];

  snprintf(filename, sizeof(filename), "credits|%s.spr", file);
  strcpy(filename, img_file_name(filename));

  cl->n_frames = read_xbitmaps(filename, 256, ani);
  if (cl->n_frames <= 0) {
    cl->bmp = NULL;
    return 1;
  }

  if (cl->n_frames == 1) {
    cl->bmp = ani[0];
    add_color_bmp(cl->bmp, add_color);
    cl->x = (SCREEN_W - cl->bmp->w) / 2;
    return 0;
  }

  cl->ani = (XBITMAP **) malloc(sizeof(XBITMAP *) * cl->n_frames);
  if (cl->ani == NULL) {
    for (i = 0; i < cl->n_frames; i++)
      destroy_xbitmap(ani[i]);
    cl->bmp = NULL;
    return 1;
  }
  for (i = 0; i < cl->n_frames; i++) {
    cl->ani[i] = ani[i];
    add_color_bmp(cl->ani[i], add_color);
  }
  cl->bmp = cl->ani[0];
  cl->x = (SCREEN_W - cl->bmp->w) / 2;
  return 0;
}


static void destroy_credit_line(CREDIT_LINE *cl)
{
  if (cl->ani != NULL) {
    int i;

    for (i = 0; i < cl->n_frames; i++)
      destroy_xbitmap(cl->ani[i]);
    free(cl->ani);
    cl->ani = NULL;
  } else if (cl->bmp != NULL) {
    destroy_xbitmap(cl->bmp);
    cl->bmp = NULL;
  }
}

/* Create the bitmaps for displaying the credits */
int init_credits(void)
{
  int i, y;
  char *file;

  frame = 0;
  if (initialized)
    return 0;

  y = 0;
  for (i = 0; credit[i].text != NULL; i++) {
    credit[i].ani = NULL;

    if ((file = credit_filename(credit[i].text)) != NULL)
      read_credit_sprite(file, credit + i);

    if (file == NULL || credit[i].bmp == NULL) {
      credit[i].bmp = create_xbitmap(strlen(credit[i].text) * 8, 8,
				     SCREEN_BPP);
      if (credit[i].bmp == NULL) {
	for (i--; i >= 0; i--)
	  destroy_credit_line(credit + i);
	return 1;
      }
      clear_xbitmap(credit[i].bmp, 0);
      setup_credit_line(credit + i);
    }
    y += credit[i].delta;
    if (i > 0)
      y += credit[i].bmp->h;
    credit[i].y = y;
  }

  end_frame = 10 + credit[0].bmp->h;
  for (i = 1; credit[i].text != NULL; i++)
    end_frame += credit[i].delta + credit[i-1].bmp->h;
  end_frame += SCREEN_H;
  initialized = 1;
  return 0;
}


static void draw_credit_line(CREDIT_LINE *cl, int pos)
{
  static int color8[] = {
    21, 22,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 
    22, 21, 20
  };
#if 0
  static int color16[] = {
    MAKECOLOR16(128,128,128),
    MAKECOLOR16(144,144,144),
    MAKECOLOR16(160,160,160),
    MAKECOLOR16(176,176,176),
    MAKECOLOR16(192,192,192),
    MAKECOLOR16(208,208,208),
    MAKECOLOR16(224,224,224),
    MAKECOLOR16(240,240,240),

    MAKECOLOR16(255,255,255),
    MAKECOLOR16(255,255,255),
    MAKECOLOR16(255,255,255),
    MAKECOLOR16(255,255,255),
    MAKECOLOR16(255,255,255),
    MAKECOLOR16(255,255,255),
    MAKECOLOR16(255,255,255),
    MAKECOLOR16(255,255,255),
    MAKECOLOR16(255,255,255),

    MAKECOLOR16(240,240,240),
    MAKECOLOR16(224,224,224),
    MAKECOLOR16(208,208,208),
    MAKECOLOR16(192,192,192),
    MAKECOLOR16(176,176,176),
    MAKECOLOR16(160,160,160),
    MAKECOLOR16(144,144,144),
    MAKECOLOR16(128,128,128),
  };
#else
  static int color16[24];
#endif
  XBITMAP *bmp;
  int *color, n_colors;

  if (SCREEN_BPP == 1) {
    color = color8;
    n_colors = sizeof(color8) / sizeof(color8[0]);
  } else {
    color = color16;
    n_colors = sizeof(color16) / sizeof(color16[0]);
  }

  if (pos < -DISP_Y || pos + cl->bmp->h + 1 < 0 || pos >= SCREEN_H
      || cl->x < 0 || cl->x + cl->bmp->w >= SCREEN_W)
    return;

  if (cl->ani != NULL) {
    cl->frame = (cl->frame + 1) % cl->n_frames;
    bmp = cl->ani[cl->frame];
  } else
    bmp = cl->bmp;

  if (cl->color[(SCREEN_BPP==1) ? 0 : 1] < 0)
    color_bitmap(bmp, add_color + color[((pos/3)+n_colors) % n_colors]);
  draw_bitmap_tr_alpha(cl->alpha, bmp, DISP_X + cl->x, DISP_Y + pos);
}

/* Draw the credits, return 1 if done */
int draw_credits(int advance)
{
  int i;

  if (! initialized)
    return 1;

  for (i = 0; credit[i].text != NULL; i++)
    ;

  for (i--; i >= 0; i--)
    draw_credit_line(credit + i, SCREEN_H - frame + credit[i].y);

  if (advance && ++frame > end_frame) {
    frame = 0;
    return 1;
  }
  return 0;
}
