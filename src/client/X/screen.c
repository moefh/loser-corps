/* screen.c */

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define USE_ICON_PIXMAP

#ifdef USE_ICON_PIXMAP
#include "loser.xpm"
#endif /* USE_ICON_PIXMAP */

#include "../game.h"

#include <X11/cursorfont.h>
#include <X11/xpm.h>
#include <X11/XKBlib.h>

/* USE_ASM is now arranged by the configure scripts and Makefiles!
#ifndef sparc
#define USE_ASM
#endif
*/

#ifdef USE_ASM
#include "inline_asm.h"
#else /* USE_ASM */
#define single_memcpy(dest, src, n)     memcpy((dest), (src), (n))
#endif /* USE_ASM */

#ifdef USE_SHM
#include <X11/extensions/XShm.h>
static int use_shm;
static XShmSegmentInfo shm_info;
#endif

static int dbl_screen;       /* 1 if screen is doubled */
static int no_scanlines;     /* 1 to use scanlines */
static XBITMAP *tmp;         /* Used only if screen is doubled */
static Display *disp;
static XImage *img;
static Window win;
static XColor palette[256];
static Colormap cmap;
static Visual *visual;
static int depth;
static GC gc;

int x11_screen_w, x11_screen_h, x11_screen_bpp;

void (*draw_bitmap)(XBITMAP *bmp, int x, int y);
void (*draw_bitmap_darken)(XBITMAP *bmp, int x, int y);
void (*draw_bitmap_lighten)(XBITMAP *bmp, int x, int y);
void (*draw_bitmap_alpha25)(XBITMAP *bmp, int x, int y);
void (*draw_bitmap_alpha50)(XBITMAP *bmp, int x, int y);
void (*draw_bitmap_alpha75)(XBITMAP *bmp, int x, int y);
void (*draw_bitmap_tr)(XBITMAP *bmp, int x, int y);
void (*draw_bitmap_tr_blink)(XBITMAP *bmp, int x, int y);
void (*draw_bitmap_tr_displace)(XBITMAP *bmp, int x, int y);
void (*draw_bitmap_tr_darken)(XBITMAP *bmp, int x, int y);
void (*draw_bitmap_tr_lighten)(XBITMAP *bmp, int x, int y);
void (*draw_bitmap_tr_alpha25)(XBITMAP *bmp, int x, int y);
void (*draw_bitmap_tr_alpha50)(XBITMAP *bmp, int x, int y);
void (*draw_bitmap_tr_alpha75)(XBITMAP *bmp, int x, int y);
void (*draw_pixel)(int x, int y, int color);
void (*draw_bmp_line)(XBITMAP *bmp, int bx, int by, int x, int y,int w);
void (*draw_energy_bar)(int x, int y, int w, int h, int energy, int color);

/**********************************************************/
/****** Drawing functions *********************************/

#include "8bpp.h"
#include "15bpp.h"
#include "16bpp.h"
#include "32bpp.h"

/* #include screen.h seemed missing, so I added it. Duh */
#include "screen.h"

/**********************************************************/
/**********************************************************/

/* Compile a bitmap.  This function does nothing in X */
void compile_bitmap(XBITMAP *bmp)
{
}

static int image_depth(int bpp)
{
#if 0
  switch (bpp) {
  case 1: return 8;
  case 2: return convert_16bpp_to;
  case 4: return 24;
  }
  return 0;
#else
  return depth;
#endif
}

static void create_image(int w, int h, int bpp)
{
  if (dbl_screen) {
    tmp = create_xbitmap(w, h, bpp);
    if (tmp == NULL) {
      printf("Can't create intermediate bitmap for doubling screen\n");
      exit(1);
    }
    clear_xbitmap(tmp, 0);
    w = 2 * (w - (2 * DISP_X));
    h = 2 * (h - (2 * DISP_Y));
  }

#ifdef USE_SHM
  if (use_shm) {
    if (XShmQueryExtension(disp) == 0) {
      fprintf(stderr, "Error: MIT shared memory extensions not present.\n");
      exit(1);
    }
    img = XShmCreateImage(disp, visual, image_depth(SCREEN_BPP),
			  ZPixmap, NULL, &shm_info, w, h);
    if (img == NULL) {
      fprintf(stderr, "Can't create shared memory image.\n");
      exit(1);
    }
    shm_info.shmid = shmget(IPC_PRIVATE, img->bytes_per_line * img->height,
			    IPC_CREAT | 0777);
    if (shm_info.shmid < 0) {
      printf("Can't allocate shared memory\n");
      exit(1);
    }
    img->data = shm_info.shmaddr = shmat(shm_info.shmid, 0, 0);
    if (img->data == NULL) {
      printf("Can't attach shared memory segment\n");
      exit(1);
    }
    memset(img->data, 0, img->bytes_per_line * img->height);
    shm_info.readOnly = False;
    XShmAttach(disp, &shm_info);
  } else
#endif

  {
    char *data;

    data = (char *) malloc((w*bpp) * h);
    if (data == NULL) {
      printf("Insufficient memory to create image\n");
      exit(1);
    }
    img = XCreateImage(disp, visual, image_depth(SCREEN_BPP),
		       ZPixmap, 0, data, w, h, 8*bpp, 0);
    if (img == NULL) {
      printf("Can't create image\n");
      exit(1);
    }
  }
}

static void create_window(int w, int h)
{
  XEvent ev;
  unsigned long val_mask;
  XSetWindowAttributes swa;

  if (dbl_screen) {
    w *= 2;
    h *= 2;
  }

  val_mask = CWBackPixel /* | CWBorderPixel*/;
  swa.background_pixel = 0;
  swa.border_pixel = 1;

  if (SCREEN_BPP == 1) {
    /*
     * hack: make a colormap for the sole purpose of creating the window. We'll
     * create and set the colormap we want later.
     */
    val_mask |= CWColormap;
    swa.colormap = XCreateColormap(disp, DefaultRootWindow(disp),
				   visual, AllocAll);
  }
  
  win = XCreateWindow(disp, DefaultRootWindow(disp),
		      0, 0, w, h, 1, depth, InputOutput, visual,
		      val_mask, &swa);
  if (SCREEN_BPP == 1)
    XFreeColormap(disp, swa.colormap);
  if (win == None) {
    printf("Can't create window\n");
    exit(1);
  }
  {
    /* Set window name, min and max sizes and accept the DELETE messages */
    XSizeHints *hints;
    XClassHint *class;
    XWMHints *wm_hints;
    Atom delete_window;
    Pixmap icon = None, mask = None;
    int use_icon_pixmap = 0;

#ifdef USE_ICON_PIXMAP
    if (XCreatePixmapFromData(disp, win, loser_xpm, &icon,&mask, NULL) == XpmSuccess)
      use_icon_pixmap = 1;
#endif /* USE_ICON_PIXMAP */

    /* Set the window class, name and hints */
    if ((class = XAllocClassHint()) != NULL) {
      class->res_name = "XLoser";
      class->res_class = "xloser";
    }
    hints = XAllocSizeHints();
    if (hints != NULL) {
      hints->flags = PSize | PMinSize | PMaxSize;
      hints->min_width = hints->max_width = hints->base_width = w;
      hints->min_height = hints->max_height = hints->base_height = h;
    }
    if ((wm_hints = XAllocWMHints()) != NULL) {
      wm_hints->flags = (InputHint | StateHint | WindowGroupHint |
			 ((use_icon_pixmap)?(IconPixmapHint|IconMaskHint):0));
      wm_hints->input = True;
      wm_hints->initial_state = NormalState;
      wm_hints->icon_pixmap = icon;
      wm_hints->icon_mask = mask;
      wm_hints->window_group = win;
    }
    XmbSetWMProperties(disp, win, "XLoser", "XLoser",
		       program_argv, program_argc,
		       hints, wm_hints, class);

    delete_window = XInternAtom(disp, "WM_DELETE_WINDOW", False);
    if (delete_window != None)
      XSetWMProtocols(disp, win, &delete_window, 1);
  }

  gc = XCreateGC(disp, win, 0, NULL);

  /* XDefineCursor(disp, win, XCreateFontCursor(disp, XC_X_cursor)); */
  XSelectInput(disp, win, FocusChangeMask | ExposureMask);
  XMapRaised(disp, win);
  XClearWindow(disp, win);
  XWindowEvent(disp, win, ExposureMask, &ev);
}

/* Set the game palette to `palette_data', an array of 256 triples of
 * unsigned chars with values for red, green and blue (in this order) */
int set_game_palette(unsigned char *palette_data)
{
  int i;

  if (SCREEN_BPP != 1)
    return 0xffffff;    /* white color */

#if 0
  for (i = 255; i >= 1; i--)
    if (palette_data[3 * i] == 0 &&
	palette_data[3 * i + 1] == 0 &&
	palette_data[3 * i + 2] == 0 &&
	palette_data[3 * (i-1)] == 0 &&
	palette_data[3 * (i-1) + 1] == 0 &&
	palette_data[3 * (i-1) + 2] == 0)
      break;
  add_color = 256 - (i - 1);
#else
  add_color = 0;
#endif
  
  for (i = add_color; i < 256; i++) {
    palette[i].pixel = i;
    palette[i].flags = DoRed | DoGreen | DoBlue;
    palette[i].red = ((int)palette_data[3 * (i - add_color)]) << 8;
    palette[i].green = ((int)palette_data[3 * (i - add_color) + 1]) << 8;
    palette[i].blue = ((int)palette_data[3 * (i - add_color) + 2]) << 8;
  }

  XStoreColors(disp, cmap, palette, 256);
  XSetWindowColormap(disp, win, cmap);
  return 255;
}

/* Install the game palette.  This only creates a palette for X, it
 * doesn't yet change it.  This is done by set_game_palette() */
int install_game_palette(unsigned char *game_palette)
{
  Colormap def_colormap;
  int i;

  add_color = 0;

  if (SCREEN_BPP != 1)
    return 0xffffff;    /* white color */

  def_colormap = DefaultColormap(disp, DefaultScreen(disp));
  cmap = XCreateColormap(disp, win, visual, AllocAll);

  for (i = 0; i < 256; i++)
    palette[i].pixel = i;
  XQueryColors(disp, def_colormap, palette, 256);
  for (i = 0; i < 256; i++) {
    game_palette[3 * i] = palette[i].red >> 8;
    game_palette[3 * i + 1] = palette[i].green >> 8;
    game_palette[3 * i + 2] = palette[i].blue >> 8;
  }

  XStoreColors(disp, cmap, palette, 256);
  XSetWindowColormap(disp, win, cmap);
  return 15;
}

/* Get the screen depth.  This only sets SCREEN_BPP to 1, 2 or 4, or
 * exits with an error message if X doesn't support a depth we
 * like. */
void graph_query_bpp(int width, int height, int dbl, int no_scan, int shm)
{
  char *disp_name;
  int screen, found_good_visual = 0;
  static XVisualInfo vinfo;

  disp_name = getenv("DISPLAY");
  disp = XOpenDisplay(disp_name);
  if (disp == NULL) {
    if (disp_name == NULL)
      printf("Can't open display. Looks like you are not running X.\n");
    else
      printf("Can't open display `%s'\n", disp_name);
    exit(1);
  }

  screen = DefaultScreen(disp);
  if (XMatchVisualInfo(disp, screen, 24, TrueColor, &vinfo)) {
    XPixmapFormatValues *formats;
    int count, i;

    if ((formats = XListPixmapFormats(disp, &count)) != NULL)
      for (i = 0; i < count; i++)
	if (formats[i].bits_per_pixel == 32) {
	  found_good_visual = 1;
	  break;
	}
    if (found_good_visual) {
      convert_16bpp_to = 32;    /* convert to 32bpp when reading from file */
      SCREEN_BPP = 4;
    }
  } else if (XMatchVisualInfo(disp, screen, 16, TrueColor, &vinfo)) {
    SCREEN_BPP = 2;
    found_good_visual = 1;

    /* Some video cards really accept only 15bpp even when X is in 16bpp */
    if (vinfo.red_mask == 0x7c00
	&& vinfo.green_mask == 0x3e0
	&& vinfo.blue_mask == 0x1f)
      convert_16bpp_to = 15;  /* convert to 15bpp when reading from file */
  } else if (XMatchVisualInfo(disp, screen, 15, TrueColor, &vinfo)) {
    SCREEN_BPP = 2;
    convert_16bpp_to = 15;    /* convert to 15bpp when reading from file */
    found_good_visual = 1;
  } else if (XMatchVisualInfo(disp, screen, 8, PseudoColor, &vinfo)) {
    SCREEN_BPP = 1;
    found_good_visual = 1;
  }

  if (! found_good_visual) {
    printf("Sorry, this game needs either 15, 16 or 32 bit TrueColor\n"
	   "or 8 bit PseudoColor to run, and no such visual can be found.\n"
	   "Change your display depth if possible and try again.\n");
    exit(1);
  }
  visual = vinfo.visual;
  depth = vinfo.depth;
}

/* Initialize the graphics (create a window, image, etc).  This
 * function will exit with an error message if it can't do that. */
void install_graph(int width, int height, int dbl, int no_scan, int shm)
{
  if (disp == NULL)
    graph_query_bpp(width, height, dbl, no_scan, shm);

#ifdef USE_SHM
  use_shm = shm;
#endif
  dbl_screen = dbl;
  no_scanlines = no_scan;
  SCREEN_W = width;
  SCREEN_H = height;

  if (SCREEN_BPP == 1) {
    if (dbl_screen)
      use_8bpp_dbl();
    else
      use_8bpp();
  } else if (SCREEN_BPP == 2) {
    if (convert_16bpp_to == 15) {
      if (dbl_screen)
	use_15bpp_dbl();
      else
	use_15bpp();
    } else {
      if (dbl_screen)
	use_16bpp_dbl();
      else
	use_16bpp();
    }
  } else {
    if (dbl_screen)
      use_32bpp_dbl();
    else
      use_32bpp();
  }
  create_window(SCREEN_W, SCREEN_H);
  create_image(SCREEN_W + 2 * DISP_X, SCREEN_H + 2 * DISP_Y, SCREEN_BPP);
  atexit(close_graph);
  XSync(disp, False);
}

/* Shutdown the graphics system. */
void close_graph(void)
{
  if (disp == NULL)
    return;

#ifdef USE_SHM
  if (use_shm) {
    XShmDetach(disp, &shm_info);
    if (shm_info.shmaddr != 0)
      shmdt(shm_info.shmaddr);
    if (shm_info.shmid >= 0)
      shmctl(shm_info.shmid, IPC_RMID, 0);
  } else
#endif /* USE_SHM */
  if (img != NULL)
    XDestroyImage(img);

  XCloseDisplay(disp);
  disp = NULL;
}


/* Copy the bitmap `tmp' to `img', doubling the pixels */
static void double_screen(void)
{
  int y;

#ifdef USE_ASM

  if (SCREEN_BPP == 1) {
    char *dest;

    dest = img->data;
    if (no_scanlines)
      for (y = DISP_Y; y < SCREEN_H + DISP_Y; y++) {
	double_memcpy(dest, tmp->line[y] + DISP_X, SCREEN_W);
	single_memcpy(dest + img->bytes_per_line, dest, 2 * SCREEN_W);
	dest += 2 * img->bytes_per_line;
      }
    else
      for (y = DISP_Y; y < SCREEN_H + DISP_Y; y++) {
	double_memcpy(dest, tmp->line[y] + DISP_X, SCREEN_W);
	dest += 2 * img->bytes_per_line;
      }
  } else if (SCREEN_BPP == 2) {     /* 2 bpp */
    unsigned short *dest;

    dest = (unsigned short *) img->data;
    if (no_scanlines)
      for (y = DISP_Y; y < SCREEN_H + DISP_Y; y++) {
	double_memcpy16(dest, ((unsigned short *) tmp->line[y]) + DISP_X,
			SCREEN_W);
	single_memcpy(((char *) dest) + img->bytes_per_line,
		      dest, SCREEN_W<<2);
	dest += img->bytes_per_line;
      }
    else {
      for (y = DISP_Y; y < SCREEN_H + DISP_Y; y++) {
	double_memcpy16(dest, ((unsigned short *) tmp->line[y]) + DISP_X,
			SCREEN_W);
	dest += img->bytes_per_line;
      }
    }
  } else {     /* assume 4 bpp */
    int2bpp *dest;

    dest = (int2bpp *) img->data;
    if (no_scanlines)
      for (y = DISP_Y; y < SCREEN_H + DISP_Y; y++) {
	double_memcpy32(dest, ((int4bpp *) tmp->line[y]) + DISP_X,
			SCREEN_W);
	single_memcpy(((char *) dest) + img->bytes_per_line,
		      dest, SCREEN_W<<4);
	dest += img->bytes_per_line;
      }
    else {
      for (y = DISP_Y; y < SCREEN_H + DISP_Y; y++) {
	double_memcpy32(dest, ((int4bpp *) tmp->line[y]) + DISP_X,
			SCREEN_W);
	dest += img->bytes_per_line;
      }
    }
  }

#else /* USE_ASM */

  if (SCREEN_BPP == 1) {
    int x;
    unsigned char *dest, *src, *line;

    if (no_scanlines)
      for (y = 0; y < SCREEN_H; y++) {
	src = tmp->line[y + DISP_Y] + DISP_X;
	line = dest = (unsigned char *)img->data + 2 * (img->bytes_per_line * y);
	for (x = 0; x < SCREEN_W; x++) {
	  register char c;
	  
	  c = *src++;
	  *dest++ = c;
	  *dest++ = c;
	}
	single_memcpy(line + img->bytes_per_line, line, 2 * SCREEN_W);
      }
    else
      for (y = 0; y < SCREEN_H; y++) {
	src = tmp->line[y + DISP_Y] + DISP_X;
	line = dest = (unsigned char *)img->data + 2 * (img->bytes_per_line * y);
	for (x = 0; x < SCREEN_W; x++) {
	  register char c;

	  c = *src++;
	  *dest++ = c;
	  *dest++ = c;
	}
      }
  } else if (SCREEN_BPP == 2) {    /* 2 bpp */
    int x;
    unsigned short *dest, *src, *line;

    if (no_scanlines)
      for (y = 0; y < SCREEN_H; y++) {
	src = ((unsigned short *) (tmp->line[y + DISP_Y])) + DISP_X;
	line = dest = ((unsigned short *) img->data
		       + (img->bytes_per_line * y));
	for (x = 0; x < SCREEN_W; x++) {
	  register unsigned short c;
	  
	  c = *src++;
	  *dest++ = c;
	  *dest++ = c;
	}
	single_memcpy(line + img->bytes_per_line, line, 4 * SCREEN_W);
      }
    else
      for (y = 0; y < SCREEN_H; y++) {
	src = ((unsigned short *) tmp->line[y + DISP_Y]) + DISP_X;
	line = dest = (((unsigned short *) img->data)
		       + (img->bytes_per_line * y));
	for (x = 0; x < SCREEN_W; x++) {
	  register unsigned short c;

	  c = *src++;
	  *dest++ = c;
	  *dest++ = c;
	}
      }
  } else {     /* assume 4 bpp */
    int x;
    int4bpp *dest, *src, *line;

    if (no_scanlines)
      for (y = 0; y < SCREEN_H; y++) {
	src = ((int4bpp *) (tmp->line[y + DISP_Y])) + DISP_X;
	line = dest = (int4bpp *) ((int2bpp *) img->data
				   + (img->bytes_per_line * y));
	for (x = 0; x < SCREEN_W; x++) {
	  register int4bpp c;
	  
	  c = *src++;
	  *dest++ = c;
	  *dest++ = c;
	}
	single_memcpy((char *) line + img->bytes_per_line, line, 8*SCREEN_W);
      }
    else
      for (y = 0; y < SCREEN_H; y++) {
	src = ((int4bpp *) tmp->line[y + DISP_Y]) + DISP_X;
	line = dest = (int4bpp *) (((int2bpp *) img->data)
				   + (img->bytes_per_line * y));
	for (x = 0; x < SCREEN_W; x++) {
	  register int4bpp c;

	  c = *src++;
	  *dest++ = c;
	  *dest++ = c;
	}
      }
  }

#endif  /* USE_ASM */
}

/* Clear the screen memory to black.  Note that you must call
 * display_screen*() to really clear what the user is seeing; this
 * function only clears the screen buffer. */
int clear_screen(void)
{
  if (dbl_screen)
    memset(tmp->data, 0, tmp->line_w * tmp->h);
  else
    memset(img->data, 0, img->bytes_per_line * img->height);
  return WhitePixelOfScreen(DefaultScreenOfDisplay(disp));
}


static INLINE void put_shm_image(Display *disp, Drawable d, GC gc,
				 XImage *img, int delta)
{
  if (delta < 0)
    XShmPutImage(disp, win, gc,
		 img, 0, 0, 0, -delta, 2 * SCREEN_W, 2 * SCREEN_H + delta,
		 False);
  else
    XShmPutImage(disp, win, gc,
		 img, 0, delta, 0, 0, 2 * SCREEN_W, 2 * SCREEN_H - delta,
		 False);
}

static INLINE void put_small_shm_image(Display *disp, Drawable d, GC gc,
				       XImage *img, int delta)
{
  if (delta < 0)
    XShmPutImage(disp, win, gc,
		 img, DISP_X, DISP_Y, 0, -delta, SCREEN_W, SCREEN_H + delta,
		 False);
  else
    XShmPutImage(disp, win, gc,
		 img, DISP_X, DISP_Y + delta, 0, 0, SCREEN_W, SCREEN_H - delta,
		 False);
}

static INLINE void put_image(Display *disp, Drawable d, GC gc,
			     XImage *img, int delta)
{
  if (delta < 0)
    XPutImage(disp, win, gc,
	      img, 0, 0, 0, -delta, 2 * SCREEN_W, 2 * SCREEN_H + delta);
  else
    XPutImage(disp, win, gc,
	      img, 0, delta, 0, 0, 2 * SCREEN_W, 2 * SCREEN_H - delta);
}

static INLINE void put_small_image(Display *disp, Drawable d, GC gc,
				   XImage *img, int delta)
{
  if (delta < 0)
    XPutImage(disp, win, gc,
	      img, DISP_X, DISP_Y, 0, -delta, SCREEN_W, SCREEN_H + delta);
  else
    XPutImage(disp, win, gc,
	      img, DISP_X, DISP_Y + delta, 0, 0, SCREEN_W, SCREEN_H - delta);
}

#if 0
XPutImage(disp, win, gc,
	  img, 0, 0, 0, 0, 2 * SCREEN_W, 2 * SCREEN_H);
#endif


/* Copy the contents of the screen buffer to the display window.  You
 * may set `delta' to something different of 0 to make the "tremor
 * effect". */
void display_screen_delta(int delta)
{
  int i;

  if (dbl_screen) {
    double_screen();

#ifdef USE_SHM
    if (use_shm)
      put_shm_image(disp, win, gc, img, delta);
    else
#endif
      put_image(disp, win, gc, img, delta);
  } else {
#ifdef USE_SHM
    if (use_shm)
      put_small_shm_image(disp, win, gc, img, delta);
    else
#endif
      put_small_image(disp, win, gc, img, delta);
  }

  for (i = 0; i < n_retraces; i++)
    XSync(disp, False);
}


#if 0
static void print_keys(void)
{
  char kb_state[32];
  int i;

  XQueryKeymap(disp, kb_state);
#if 1
  for (i = 0; i < 256; i++)
    if (kb_state[i / 8] & (1 << (i % 8)))
      printf("%s pressed\n", XKeysymToString(XKeycodeToKeysym(disp, i, 0)));
#else
  for (i = 0; i < 32; i++)
    printf("%02X", (unsigned char) kb_state[i]);
  printf("\n");
#endif
}
#endif

#define MAP_XS(x,s)    case XK_##x: scancode = SCANCODE_##s; break

static void handle_keys(void)
{
  static char kb_state[32], old_kb_state[32];
  unsigned int w;
  int i, scancode, press, old_press;

  /* print_keys(); */
  XQueryKeymap(disp, kb_state);

  for (i = 0; i < 256; i++) {
    press = (((kb_state[i / 8] & (1 << (i % 8))) != 0) ? 1 : 0);
    old_press = (((old_kb_state[i / 8] & (1 << (i % 8))) != 0) ? 1 : 0);

#if 0
    if (press) {
      printf("Backspace is %d, Delete is %d\n",
	     (kb_state[22/8] & (1 << (22%8))) ? 1 : 0,
	     (kb_state[107/8] & (1 << (107%8))) ? 1 : 0);
    }
#endif

    if (press == old_press)
      continue;

    switch (w = XkbKeycodeToKeysym(disp, i, 0, 0)) {
      MAP_XS(0, 0); MAP_XS(1, 1); MAP_XS(2, 2); MAP_XS(3, 3); MAP_XS(4, 4);
      MAP_XS(5, 5); MAP_XS(6, 6); MAP_XS(7, 7); MAP_XS(8, 8); MAP_XS(9, 9);

      MAP_XS(q, Q); MAP_XS(w, W); MAP_XS(e, E); MAP_XS(r, R); MAP_XS(t, T);
      MAP_XS(y, Y); MAP_XS(u, U); MAP_XS(i, I); MAP_XS(o, O); MAP_XS(p, P);

      MAP_XS(a, A); MAP_XS(s, S); MAP_XS(d, D); MAP_XS(f, F); MAP_XS(g, G);
      MAP_XS(h, H); MAP_XS(j, J); MAP_XS(k, K); MAP_XS(l, L);

      MAP_XS(z, Z); MAP_XS(x, X); MAP_XS(c, C); MAP_XS(v, V); MAP_XS(b, B);
      MAP_XS(n, N); MAP_XS(m, M);

      MAP_XS(Up, CURSORBLOCKUP);
      MAP_XS(Down, CURSORBLOCKDOWN);
      MAP_XS(Left, CURSORBLOCKLEFT);
      MAP_XS(Right, CURSORBLOCKRIGHT);

      MAP_XS(space, SPACE);
      MAP_XS(slash, SLASH);
      MAP_XS(comma, COMMA);
      MAP_XS(period, PERIOD);
      MAP_XS(minus, MINUS);
      MAP_XS(equal, EQUAL);
      MAP_XS(bracketleft, BRACKET_LEFT);
      MAP_XS(bracketright, BRACKET_RIGHT);
      MAP_XS(backslash, BACKSLASH);
      MAP_XS(semicolon, SEMICOLON);
      MAP_XS(apostrophe, APOSTROPHE);
      MAP_XS(grave, GRAVE);

      MAP_XS(F1, F1); MAP_XS(F2, F2); MAP_XS(F3, F3); MAP_XS(F4, F4);
      MAP_XS(F5, F5); MAP_XS(F6, F6); MAP_XS(F7, F7); MAP_XS(F8, F8);
      MAP_XS(F9, F9); MAP_XS(F10, F10); MAP_XS(F11, F11); MAP_XS(F12, F12);

      MAP_XS(Escape, ESCAPE);
      MAP_XS(BackSpace, BACKSPACE);
      MAP_XS(Delete, REMOVE);
      MAP_XS(Return, ENTER);
      MAP_XS(Tab, TAB);

      MAP_XS(Shift_L, LEFTSHIFT);
      MAP_XS(Shift_R, RIGHTSHIFT);

    default:
      scancode = -1;
    }
    if (scancode >= 0) {
      if (press) {
	if (game_key[scancode] != 1) {
	  game_key[scancode] = 1;
	  game_key_trans[scancode] = 1;
	  client_send_key(server, scancode, 1);
	}
      } else {
	if (game_key[scancode] != 0) {
	  game_key[scancode] = 0;
	  game_key_trans[scancode] = -1;
	  client_send_key(server, scancode, -1);
	}
      }
    }
  }

  for (i = 0; i < 32; i++)
    old_kb_state[i] = kb_state[i];
}


/* Poll the keyboard, setting the vectors game_key[] and
 * game_key_trans[] according to the keyboard state.  This function
 * returns 1 to terminate the game (e.g., when the user asks to close
 * the window). */
int poll_keyboard(void)
{
  static int got_focus;
  XEvent ev;

  if (got_focus)
    handle_keys();

  while (XCheckWindowEvent(disp, win, FocusChangeMask, &ev))
    if (ev.type == FocusOut)
      got_focus = 0;
    else if (ev.type == FocusIn)
      got_focus = 1;

  while (XCheckWindowEvent(disp, win, ExposureMask, &ev))
    if (ev.xexpose.count == 0)
      display_screen();

  while (XCheckTypedWindowEvent(disp, win, ClientMessage, &ev))
    if ((ev.xclient.message_type == XInternAtom(disp, "WM_PROTOCOLS", True))
	&& (ev.xclient.data.l[0]
	    == XInternAtom(disp, "WM_DELETE_WINDOW", True)))
      return 1;        /* Window deleted */
  return 0;
}
