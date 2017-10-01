/* window.c - XDialogs main file.
 *
 *  Copyright (C) 1997  Ricardo R. Massaro
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>

#include "xdialogs.h"
#include "internal.h"

char xd_version[] = "XDialogs 1.0, by Ricardo R. Massaro";
WINDOW *active_window = NULL;
NOTIFY_WIN *notify_list = NULL;
FONT *xd_default_font, *xd_fixed_font;
char xd_font_name[256] = "fixed";
Colormap xd_colormap;

char xd_def_window_name[] = "XDialogs";
char xd_def_window_class[] = "XDialogs";

char *xd_window_name = xd_def_window_name;
char *xd_window_class = xd_def_window_class;

int xd_no_sent_events;
int xd_install_colormap;
int gui_fg_color, gui_bg_color, gui_mg_color;
int xd_stddlg_3d;
int xd_light_3d, xd_shadow_3d, xd_back_3d;

Display *xd_display = NULL;
Cursor xd_arrow_cursor, xd_text_cursor;

static WINDOW root_window[1];

#define INI_PAL_SIZE  16    /* Number of colors initially in the pallete */

PALETTE xd_pal[PAL_SIZE] = {
  { 0, 0, { 0,     0,     0     } },    /* Black       */
  { 0, 0, { 0,     0,     32767 } },    /* Blue        */
  { 0, 0, { 0,     32767, 0     } },    /* Green       */
  { 0, 0, { 0,     32767, 32767 } },    /* Cyan        */
  { 0, 0, { 32767, 0,     0     } },    /* Red         */
  { 0, 0, { 32767, 0,     32767 } },    /* Magenta     */
  { 0, 0, { 32767, 32767, 0     } },    /* Yellow      */
  { 0, 0, { 50176, 50176, 50176 } },    /* Light Gray  */
  { 0, 0, { 32767, 32767, 32767 } },    /* Dark gray   */
  { 0, 0, { 0,     0,     65535 } },    /* Blue        */
  { 0, 0, { 0,     65535, 0     } },    /* Green       */
  { 0, 0, { 0,     65535, 65535 } },    /* Cyan        */
  { 0, 0, { 65535, 0,     0     } },    /* Red         */
  { 0, 0, { 65535, 0,     65535 } },    /* Magenta     */
  { 0, 0, { 65535, 65535, 0     } },    /* Yellow      */
  { 0, 0, { 65535, 65535, 65535 } },    /* White       */

  /* All other colors are initially black */
};

/* Add `win' to the list of `win->parent' child windows */
static void add_window(WINDOW *win)
{
  WINDOW *w, *parent;

  parent = (win->parent != NULL) ? win->parent : root_window;

  if (parent->child == NULL) {
    parent->child = win;
    return;
  }

  for (w = parent->child; w->sibling != NULL; w = w->sibling)
    ;
  w->sibling = win;
}

/* Remove `win' from the list of `win->parent' child windows */
static void remove_window(WINDOW *win)
{
  WINDOW *parent, *w, *tmp;

  /* Remove the window from the parent list of child windows. */
  if (win != root_window) {
    parent = (win->parent != NULL) ? win->parent : root_window;

    tmp = NULL;
    for (w = parent->child; w != NULL; tmp = w, w = w->sibling)
      if (w == win)
	break;
    if (w == NULL) {
#if DEBUG
      fprintf(stderr,
	      "remove_window(): can't find child `%p' among `%p' childs\n",
	      win, parent);
#endif
      return;
    }
    if (tmp == NULL)
      parent->child = win->sibling;
    else
      tmp->sibling = win->sibling;
  }

  /* Remove all childs */
  if (win->child != NULL) {
#if DEBUG
    fprintf(stderr, "remove_window(): trying to remove non-leaf window\n");
#endif

    /* This makes `root_window' adopt `win' childs: */
    for (w = win->child; w != NULL; w = w->sibling)
      w->parent = NULL;
  }
}

/* Create the default font */
static FONT *create_default_font(void)
{
  FONT *f;

  f = (FONT *) malloc(sizeof(FONT));
  if (f == NULL)
    return NULL;
  f->font_struct = XLoadQueryFont(xd_display, xd_font_name);
  if (f->font_struct == NULL) {
    f->font_struct = XLoadQueryFont(xd_display, "fixed");
    if (f->font_struct == NULL) {
      free(f);
      return NULL;
    }
  }
  f->font_x = f->font_struct->min_bounds.lbearing;
  f->font_y = f->font_struct->max_bounds.ascent;
  f->num_refs = 0;
  return f;
}

/* Create the default font */
static FONT *create_fixed_font(void)
{
  FONT *f;

  f = (FONT *) malloc(sizeof(FONT));
  if (f == NULL)
    return NULL;
  f->font_struct = XLoadQueryFont(xd_display, "fixed");
  if (f->font_struct == NULL) {
    free(f);
    return NULL;
  }
  f->font_x = f->font_struct->min_bounds.lbearing;
  f->font_y = f->font_struct->max_bounds.ascent;
  f->num_refs = 0;
  return f;
}

void xd_install_palette(void)
{
  int i;
  XColor colors[PAL_SIZE];

  xd_install_colormap = 1;
  xd_colormap = XCreateColormap(xd_display,
				DefaultRootWindow(xd_display),
				DefaultVisual(xd_display,
					      DefaultScreen(xd_display)),
				AllocAll);

  for (i = 0; i < PAL_SIZE; i++) {
    colors[i].pixel = i;
    colors[i].flags = DoRed | DoGreen | DoBlue;
    colors[i].red   = xd_pal[i].val[0];
    colors[i].green = xd_pal[i].val[1];
    colors[i].blue  = xd_pal[i].val[2];
    xd_pal[i].pixel = i;
  }
  XStoreColors(xd_display, xd_colormap, colors, PAL_SIZE);
  XInstallColormap(xd_display, xd_colormap);
}

void xd_set_palette(unsigned char *pal, int from, int to)
{
  int i;
  XColor colors[PAL_SIZE];

  for (i = 0; i <= to - from; i++) {
    colors[i].pixel = i;
    colors[i].flags = DoRed | DoGreen | DoBlue;
    colors[i].red   = (unsigned int) pal[3 * (i + from)] << 8;
    colors[i].green = (unsigned int) pal[3 * (i + from) + 1] << 8;
    colors[i].blue  = (unsigned int) pal[3 * (i + from) + 2] << 8;
  }
  XStoreColors(xd_display, xd_colormap, colors, to - from + 1);
}

static void setup_colors(void)
{
  int i;

  if (xd_install_colormap
      && DefaultDepth(xd_display, DefaultScreen(xd_display)) == 8) {
    xd_install_palette();
  } else {
    XColor color;

    xd_colormap = DefaultColormap(xd_display, DefaultScreen(xd_display));
    color.flags = DoRed|DoGreen|DoBlue;
    for (i = 0; i < INI_PAL_SIZE; i++) {
      color.red   = xd_pal[i].val[0];
      color.green = xd_pal[i].val[1];
      color.blue  = xd_pal[i].val[2];
      XAllocColor(xd_display, xd_colormap, &color);
      xd_pal[i].val[0] = color.red;
      xd_pal[i].val[1] = color.green;
      xd_pal[i].val[2] = color.blue;
      xd_pal[i].pixel = color.pixel;
      xd_pal[i].allocated = 1;
    }

    if (xd_pal[COLOR_GRAY].pixel == xd_pal[COLOR_BLACK].pixel)
      xd_pal[COLOR_GRAY].pixel = xd_pal[COLOR_WHITE].pixel;
    if (xd_pal[COLOR_GRAY1].pixel == xd_pal[COLOR_GRAY].pixel)
      xd_pal[COLOR_GRAY1].pixel = xd_pal[COLOR_BLACK].pixel;
  }
}

/* Initializes the graphics system */
int init_graph(char *disp_name)
{
  xd_display = XOpenDisplay(disp_name);
  if (xd_display == NULL)
    return -1;

  xd_no_sent_events = 0;
  root_window->child = root_window->sibling = NULL;
  active_window = NULL;

  xd_light_3d = COLOR_WHITE;
  xd_shadow_3d = COLOR_GRAY1;
  xd_back_3d = COLOR_GRAY;
  gui_fg_color = COLOR_BLACK;
  gui_bg_color = COLOR_WHITE;
  xd_stddlg_3d = 0;

  /* Read the fonts */
  xd_default_font = create_default_font();
  if (xd_default_font == NULL) {
    closegraph();
    return -1;
  }
  xd_fixed_font = create_fixed_font();
  if (xd_fixed_font == NULL) {
    closegraph();
    return -1;
  }

  /* Load the cursors */
  xd_arrow_cursor = XCreateFontCursor(xd_display, XC_left_ptr);
  xd_text_cursor = XCreateFontCursor(xd_display, XC_xterm);

  /* Load the colors */
  setup_colors();

  return 0;
}

/* Shuts down the graphics system */
void closegraph(void)
{
  Colormap cmap;

  if (xd_display == NULL)
    return;

  /* Closes all windows */
  close_window(root_window);

  /* Free the allocated colors */
  cmap = DefaultColormap(xd_display, DefaultScreen(xd_display));
  XFreeColors(xd_display, cmap, NULL, 0, AllPlanes);

  XCloseDisplay(xd_display);
}

/* Create a dialog window belonging to `parent' */
void create_dialog_window(WINDOW *parent, DIALOG *dlg)
{
  dlg->win = create_window_x(NULL, dlg->x, dlg->y, dlg->w, dlg->h,
			     0, parent, NULL);
  dlg->win->dialog = dlg;
  XSetWindowBackground(dlg->win->disp, dlg->win->window, COLOR(dlg->bg));

  /* The window will keep hidden while the parent is hidden */
  win_showwindow(dlg->win);
}

/* Creates a new window of size w, h */
WINDOW *create_window(char *title, int x, int y, int w, int h,
		      WINDOW *parent, DIALOG *dlg)
{
  WINDOW *win;
  if (xd_display == NULL)
    return NULL;

  win = create_window_x(title, x, y, w, h, 1, parent, dlg);
  if (win != NULL)
    win->is_child = 0;
  return win;
}

/* Create a window */
WINDOW *create_window_x(char *title, int x, int y, int w, int h,
			int managed, WINDOW *parent, DIALOG *dlg)
{
  WINDOW *win;
  XSizeHints *size_hints;
  unsigned long fg, bg;
  char *name = NULL;
  XGCValues gc_values;

  if (xd_display == NULL)
    return NULL;

  if (dlg != NULL && dlg->d4 != 0)
    return NULL;    /* This dialog is being used by another window */

  if ((size_hints = XAllocSizeHints()) == NULL)
    return NULL;

  if ((win = malloc(sizeof(WINDOW))) == NULL) {
    XFree(size_hints);
    return NULL;
  }
  memset(win, 0, sizeof(WINDOW));

  name = title;
  size_hints->flags = PSize | PPosition;
  if (dlg != NULL) {
    win->color = dlg[0].fg;
    win->bkcolor = dlg[0].bg;
    win->x = size_hints->x = dlg[0].x;
    win->y = size_hints->y = dlg[0].y;
    win->w = size_hints->width = dlg[0].w;
    win->h = size_hints->height = dlg[0].h;
    if (dlg->dp != NULL)
      name = dlg->dp;
  } else {
    win->color = COLOR_BLACK;
    win->bkcolor = COLOR_WHITE;
    win->x = size_hints->x = x;
    win->y = size_hints->y = y;
    win->w = size_hints->width = w;
    win->h = size_hints->height = h;
  }
  fg = COLOR(win->color);
  bg = COLOR(win->bkcolor);

  win->dialog = dlg;
  win->parent = parent;
  win->child = win->sibling = NULL;
  win->pointer_grabbed = 0;
  win->is_child = 1;
  win->selected = 1;

  win->disp = xd_display;
  win->depth = DefaultDepth(xd_display, DefaultScreen(xd_display));

  /* Create the window and set its properties */
  win->window = XCreateSimpleWindow(xd_display,
				    ((managed || parent == NULL) ?
				     DefaultRootWindow(xd_display) :
				     parent->window),
				    size_hints->x, size_hints->y,
				    size_hints->width, size_hints->height,
				    1, fg, bg);
  if (managed) {
    XClassHint *class;
    XWMHints *wm_hints;
    Atom delete_window;

    /* Set the window class */
    class = XAllocClassHint();
    if (class != NULL) {
      class->res_name = xd_window_name;
      class->res_class = xd_window_class;
    }
    if ((wm_hints = XAllocWMHints()) != NULL) {
      wm_hints->flags = (InputHint | StateHint);
      wm_hints->input = True;
      wm_hints->initial_state = NormalState;
    }

    XmbSetWMProperties(xd_display, win->window, name, name,
		       NULL, 0, size_hints, wm_hints, class);

    /* Accept the DELETE_WINDOW protocol */
    delete_window = XInternAtom(xd_display, "WM_DELETE_WINDOW", False);
    if (delete_window != None)
      XSetWMProtocols(xd_display, win->window, &delete_window, 1);
  }

  XSelectInput(xd_display, win->window, EV_MASK);
  if (! managed) {
    XSetWindowAttributes wa;

    wa.override_redirect = True;
    XChangeWindowAttributes(xd_display, win->window,
			    CWOverrideRedirect, &wa);
  }

  /* Create the gc that we will  use to draw */
  gc_values.graphics_exposures = False;
  win->gc = XCreateGC(xd_display, win->window, GCGraphicsExposures,
		      &gc_values);
  XSetBackground(xd_display, win->gc, bg);
  XSetForeground(xd_display, win->gc, fg);
  win->image = None;
  win->font = xd_default_font;
  XSetFont(xd_display, win->gc, win->font->font_struct->fid);

  /* Clear the keyboar buffer */
  win->pkb_i = win->pkb_f = 0;

  /* Create the dialog window controls */
  if (dlg != NULL) {
    int i;

    dlg->win = win;
    dlg->d4 = 1;     /* Mark this dialog as `with window' */
    for (i = 1; dlg[i].proc != NULL; i++)
      create_dialog_window(win, dlg + i);
  }

  XSetWindowColormap(xd_display, win->window, xd_colormap);

  add_window(win);
  if (parent != NULL && managed)
    XSetTransientForHint(xd_display, win->window, parent->window);
  XDefineCursor(xd_display, win->window, xd_arrow_cursor);

  if (active_window == NULL)
    active_window = win;

  XFree(size_hints);
  return win;
}

/* Show a window */
void win_showwindow(WINDOW *win)
{
  XMapRaised(win->disp, win->window);
  XSync(win->disp, 0);
}

/* Hide a window */
void win_hidewindow(WINDOW *win)
{
  XUnmapWindow(win->disp, win->window);
  XSync(win->disp, 0);
}

/* Set the position of a window */
void win_movewindow(WINDOW *win, int x, int y)
{
  win->x = x;
  win->y = y;
  XMoveWindow(win->disp, win->window, x, y);
}

/* Set the minimum and maximum sizes of a window */
void win_set_min_max(WINDOW *win, int min_w, int min_h, int max_w, int max_h)
{
  XSizeHints *sh;

  if ((sh = XAllocSizeHints()) == NULL)
    return;

  sh->min_width = min_w;
  sh->min_height = min_h;
  sh->max_width = max_w;
  sh->max_height = max_h;
  sh->flags = PMinSize | PMaxSize;
  XSetWMNormalHints(win->disp, win->window, sh);
  XFree(sh);
}

/* Set the size of a window */
void win_resizewindow(WINDOW *win, int w, int h)
{
  win->w = w;
  win->h = h;
  XResizeWindow(win->disp, win->window, w, h);
}

/* Closes and destroys the window `win' */
void close_window(WINDOW *win)
{
  WINDOW *ch, *tmp;

  if (win == NULL)
    return;

  /* Close the childs */
  ch = win->child;
  while (ch != NULL) {
    tmp = ch->sibling;
    close_window(ch);
    ch = tmp;
  }
  remove_window(win);

  /* Now really close the window */
  if (win != root_window) {
    if (win->image != None)
      XFreePixmap(win->disp, win->image);
    free_font(win->font);
    XFreeGC(win->disp, win->gc);
    XDestroyWindow(win->disp, win->window);
    XSync(win->disp, 0);
    if (win->dialog != NULL)
      win->dialog->d4 = 0;    /* Mark it's not being used anymore */
    free(win);
  }
}

#if DEBUG
int dump_win_list(WINDOW *root, int start)
{
  WINDOW *w;

  printf("%*s%p\n", start, "", root);
  for (w = root->child; w != NULL; w = w->sibling)
    dump_win_list(w, start + 2);
  return 0;
}
#endif

static WINDOW *do_search_window_handle(WINDOW *parent, Window win)
{
  WINDOW *w, *tmp;

  if (parent == NULL)
    return NULL;

  if (parent->window == win)
    return parent;

  for (w = parent->child; w != NULL; w = w->sibling)
    if ((tmp = do_search_window_handle(w, win)) != NULL)
      return tmp;
  return NULL;
}

/* Return the WINDOW structure that corresponds to w */
WINDOW *search_window_handle(Window w)
{
  return do_search_window_handle(root_window, w);
}

/* Return the screen width */
int screen_width(void)
{
  return WidthOfScreen(DefaultScreenOfDisplay(xd_display));
}

/* Return the screen height */
int screen_height(void)
{
  return HeightOfScreen(DefaultScreenOfDisplay(xd_display));
}

/* Makes `win' the active window */
void set_active_window(WINDOW *win)
{
  active_window = win;
}

/* Create a new font */
FONT *create_font(char *font_name)
{
  FONT *f;

  /* FIXME: this function should keep a list of font names (perhaps
   * add a `name' field in the FONT structure) and return the existing
   * font if it was loaded before.
   */
  f = (FONT *) malloc(sizeof(FONT));
  f->num_refs = 0;
  if (f == NULL)
    return NULL;
  f->font_struct = XLoadQueryFont(xd_display, font_name);
  if (f->font_struct == NULL) {
    free(f);
    return NULL;
  }
  f->font_x = f->font_struct->min_bounds.lbearing;
  f->font_y = f->font_struct->max_bounds.ascent;
  f->num_refs = 0;
  return f;
}

/* Free a font */
void free_font(FONT *f)
{
  if (f->num_refs-- > 0 || f == xd_default_font)
    return;
  if (f->font_struct != NULL)
    XFreeFont(xd_display, f->font_struct);
#if DEBUG
  else
    fprintf(stderr, "free_font(): empty font!\n");
#endif
  f->font_struct = NULL;
  free(f);
}

/* Store the selection in the display buffer */
void win_set_selection(WINDOW *win, char *sel, int n)
{
  Atom selection;

  /* Get or create the primary selection atom */
  selection = XInternAtom(win->disp, "PRIMARY", False);
  /* Set the selection owner to `none': I wonder if it always works? */
  XSetSelectionOwner(win->disp, selection, None, CurrentTime);
  /* Now, store the selection in the cut buffer */
  XStoreBytes(win->disp, sel, n);
}

/* Get the selection from the display buffer */
int win_get_selection(WINDOW *win, char *sel, int max)
{
  int n;
  char *p;
  
  p = XFetchBytes(win->disp, &n);
  if (p != NULL) {
    strncpy(sel, p, MIN(max, n));
    XFree(p);
  }
  return MIN(max, n);
}

#if DEBUG
/* Display information on the selection */
void selection_info(void)
{
  Window w;
  int i, n;
  char *p;

  w = XGetSelectionOwner(xd_display, 1);
  printf("Selection owner window: 0x%x\n", (unsigned) w);
  p = XFetchBytes(xd_display, &n);
  if (p != NULL)
    printf("XFetchBytes gives `%.*s'\n", n, p);
  else
    printf("XFetchBytes gives NULL\n");
  for (i = 0; i < 8; i++) {
    p = XFetchBuffer(xd_display, &n, i);
    if (p != NULL)
      printf("buffer[%d] = `%.*s'\n", i, n, p);
    else
      printf("buffer[%d] = NULL\n", i);
  }
}
#endif
