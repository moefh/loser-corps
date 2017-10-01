/* win_io.c - Window input/output functions.
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
 *
 *
 * You may #define NO_DCLICK to disable double click notification messages.
 * You will want to do this if your standard library doesn't provide
 * `gettimeofday()' or if the returned microsseconds field is not
 * accurate enough. If someone knows a more general way of doing this,
 * please implement it or tell me about it so I can implement it.
 *
 *
 * BUGS/NOTES:
 *
 * - win_read_key() should NEVER be called to a window with a dialog
 * associated to it, any keypress will generate a MSG_KEY message to the
 * dialog instead of sending the key to the buffer. For the same reason,
 * win_has_key() will always return 0 to such a window. If a dialog
 * procedure returns D_O_K in response to a MSG_KEY message, the key will
 * go to the main dialog window buffer, not to the control window. The key,
 * then, can be used by the dialog manager function to change the focus,
 * etc.
 *
 * Ricardo R. Massaro (massaro@ime.usp.br)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#ifndef NO_DCILCK
#include <sys/time.h>
#endif

#include "xdialogs.h"
#include "internal.h"

/* Use this function to avoid CPU burn while you're doing nothing
 * (tipically, waiting for some event). Increase the number in `usleep'
 * if the system is too slow, or decrease it if you want your program to
 * run faster.
 */
void kill_time(void)
{
  usleep(1000);
}

#ifndef XK_ISO_Left_Tab
#define XK_ISO_Left_Tab 0xFE20
#endif

/* Move the dialog focus due to a keypress */
static void move_dialog_focus(DIALOG *dialog, int key)
{
  int old, n_controls, inc, focus;

  if (KEYSYM(key) == XK_Tab || KEYSYM(key) == XK_ISO_Left_Tab) {
    if (KEYSYM(key) == XK_ISO_Left_Tab || (KEYMOD(key) & ShiftMask))
      inc = -1;
    else
      inc = 1;

    /* Count the number of controls */
    for (n_controls = 0; dialog[n_controls].proc != NULL; n_controls++)
      ;

    old = dialog->d1;
    /* Pick the first control that wants the focus */
    focus = old + inc;
    if (focus < 0)
      focus = n_controls - 1;
    else
      focus = focus % n_controls;
    while (focus != old) {
      if (SEND_MESSAGE(MSG_WANTFOCUS, dialog + focus, 0) & D_WANTFOCUS) {
	if (old >= 0) {
	  dialog[old].flags &= ~D_GOTFOCUS;
	  SEND_MESSAGE(MSG_LOSTFOCUS, dialog + old, 0);
	}
	dialog[focus].flags |= D_GOTFOCUS;
	SEND_MESSAGE(MSG_GOTFOCUS, dialog + focus, 0);
	break;
      }
      focus = focus + inc;
      if (focus < 0)
	focus = n_controls - 1;
      else
	focus = focus % n_controls;
    }
    dialog->d1 = focus;
  }
}

/* Process the specified event on the specifyed window */
static void process_event(WINDOW *w, XEvent *ev)
{
  DIALOG *main_dlg;
  int ret_flags;

  if (w->dialog != NULL) {
    if (w->dialog->type == CTL_WINDOW)
      main_dlg = w->dialog;
    else
      main_dlg = w->parent->dialog;
    if (main_dlg->d5 == 0)
      return;  /* The dialog hasn't been initialized or has been tarminated */
  } else
    main_dlg = NULL;

  switch(ev->type) {
  case ClientMessage:
    if (main_dlg != NULL && main_dlg->d2 == 0) {
      if ((ev->xclient.message_type
	   == XInternAtom(w->disp, "WM_PROTOCOLS", True))
	  && (ev->xclient.data.l[0]
	      == XInternAtom(w->disp, "WM_DELETE_WINDOW", True))) {
	main_dlg->d1 = -1;       /* No control selected */
	main_dlg->flags |= D_CLOSE;
	win_hidewindow(main_dlg->win);
      }
    }
    break;

  case KeyPress:             /* A key was pressed */
    if (main_dlg == NULL || main_dlg->d2 == 0) {
      char buf[64];
      unsigned long k;

      XLookupString(&ev->xkey, buf, 64, &k, NULL);

      /* Put the key code and the modifiers in k */
      k |= (((unsigned) ev->xkey.state << 16) & 0xffff0000u);

      if (w->dialog != NULL) {
	ret_flags = SEND_MESSAGE(MSG_KEY, main_dlg + main_dlg->d1, (int) k);

	if (ret_flags & D_CLOSE) {
	  main_dlg->flags |= D_CLOSE;
	  win_hidewindow(main_dlg->win);
	} else if ((ret_flags & D_USED_CHAR) == 0) {
	  if (main_dlg != NULL)
	    w = main_dlg->win;
	  move_dialog_focus(main_dlg, (int) k);
	}
	if (ret_flags & D_REDRAW)
	  broadcast_msg(MSG_DRAW, main_dlg->win, 0);
      } else {
	/* Window without a dialog */
	w->kb_buf[w->pkb_f] = k;
	w->pkb_f = (w->pkb_f + 1) % MAX_KB_BUF;
      } 
    }
    break;

  case ButtonPress:
    if (main_dlg != NULL && main_dlg->d2 != 0)
      break;

    if (w->dialog != NULL) {
      XButtonPressedEvent *bev;
      int msg = MSG_CLICK, key_mod;
#ifndef NO_DCLICK
      struct timeval tv;
      unsigned long delta;
#endif

      bev = (XButtonPressedEvent *) ev;
#if 0
      {
	Window a, b;
	int x1, y1, x2, y2;

	XQueryPointer(w->disp, w->window, &a, &b,
		      &x1, &y1, &x2, &y2, &key_mod);
      }
#else
      key_mod = bev->state;
#endif

      if (SEND_MESSAGE(MSG_WANTFOCUS, w->dialog, 0) == D_WANTFOCUS) {
	if (main_dlg->d1 >= 0) {
	  main_dlg[main_dlg->d1].flags &= ~D_GOTFOCUS;
	  SEND_MESSAGE(MSG_LOSTFOCUS, &main_dlg[main_dlg->d1], 0);
	}
	main_dlg->d1 = w->dialog - main_dlg;
	w->dialog->flags |= D_GOTFOCUS;
	SEND_MESSAGE(MSG_GOTFOCUS, w->dialog, 0);
      }

#ifndef NO_DCLICK
      gettimeofday(&tv, NULL);
      if (tv.tv_sec == w->last_click_sec)
	delta = tv.tv_usec - w->last_click_usec;
      else if (tv.tv_sec == w->last_click_sec - 1)
	delta = ~(0ul) - (tv.tv_usec - w->last_click_usec);
      else
	delta = DCLICK_TIME;
      if (delta < DCLICK_TIME)
	msg = MSG_DCLICK;
      else {
	w->last_click_usec = tv.tv_usec;
	w->last_click_sec = tv.tv_sec;
      }
#endif

      w->mouse_x = bev->x;
      w->mouse_y = bev->y;
      w->mouse_b = bev->state & (MOUSE_L | MOUSE_M | MOUSE_R);
      if (bev->button == Button1)
	w->mouse_b |= MOUSE_L;
      else if (bev->button == Button2)
	w->mouse_b |= MOUSE_M;
      else if (bev->button == Button3)
	w->mouse_b |= MOUSE_R;
      ret_flags = SEND_MESSAGE(msg, w->dialog, key_mod);

      if (ret_flags & D_CLOSE) {
	main_dlg->flags |= D_CLOSE;
	win_hidewindow(main_dlg->win);
      }
      if (ret_flags & D_REDRAW)
	broadcast_msg(MSG_DRAW, main_dlg->win, 0);
    }
    break;

  case MotionNotify:
    if (w->dialog != NULL && w->dialog->flags & D_WANTMOUSEMOVE) {
      XPointerMovedEvent *pmev;
      int ret_flags;

      pmev = (XPointerMovedEvent *) ev;

      w->mouse_x = pmev->x;
      w->mouse_y = pmev->y;
      if (pmev->state & Button1)
	w->mouse_b |= MOUSE_L;
      else if (pmev->state & Button2)
	w->mouse_b |= MOUSE_M;
      else if (pmev->state & Button3)
	w->mouse_b |= MOUSE_R;

      ret_flags = SEND_MESSAGE(MSG_MOUSEMOVE, w->dialog, 0);

      if (ret_flags & D_CLOSE) {
	main_dlg->flags |= D_CLOSE;
	win_hidewindow(main_dlg->win);
      }
      if (ret_flags & D_REDRAW)
	broadcast_msg(MSG_DRAW, main_dlg->win, 0);
    }
    break;

  case Expose:               /* We have to redraw the window */
    {
      XExposeEvent *eev;

      eev = (XExposeEvent *) ev;

      if (w->dialog == NULL) {
	if (w->image != None) {
	  int a;

	  a = win_getdrawmode(w);
	  if (a != DM_COPY)
	    win_setdrawmode(w, DM_COPY);
	  XCopyArea(w->disp, w->image, w->window, w->gc, eev->x, eev->y,
		    eev->width, eev->height, eev->x, eev->y);
	  if (a != DM_COPY)
	    win_setdrawmode(w, a);
	}
      } else {
	w->dialog->draw_x = eev->x;
	w->dialog->draw_y = eev->y;
	w->dialog->draw_w = eev->width;
	w->dialog->draw_h = eev->height;
	SEND_MESSAGE(MSG_DRAW, w->dialog, 1);
      }
    }
    break;
      
  case ConfigureNotify:      /* The size has changed */
    {
      XConfigureEvent *cev;

      cev = (XConfigureEvent *) ev;

      if (cev->x != 0 || cev->y != 0) {
	w->x = cev->x;
	w->y = cev->y;
      }

      if (w->dialog == NULL || w->dialog != main_dlg)
	break;

      w->w = cev->width;
      w->h = cev->height;

      broadcast_msg(MSG_RESIZE, w, 0);
    }
    break;

  case EnterNotify:
    if (main_dlg != NULL && (main_dlg->d2 != 0))
      break;
    w->selected = 1;
    if (w->dialog != NULL) {
      w->dialog->flags |= D_GOTMOUSE;
      SEND_MESSAGE(MSG_GOTMOUSE, w->dialog, 0);
    }
    break;

  case LeaveNotify:
    if (main_dlg != NULL && (main_dlg->d2 != 0))
      break;
    w->selected = 0;
    if (w->dialog != NULL) {
      w->dialog->flags &= ~D_GOTMOUSE;
      SEND_MESSAGE(MSG_LOSTMOUSE, w->dialog, 0);
    }
    break;
  }
}

/* Process all the pending events (i.e., on the queue). */
void process_pending_events(void)
{
  XEvent event;
  WINDOW *w;
  int n;

  n = XPending(xd_display);

  while (n-- > 0) {
    XNextEvent(xd_display, &event);
    if (event.xany.send_event != False && xd_no_sent_events)
      continue;
    w = search_window_handle(((XAnyEvent *) &event)->window);
    if (w != NULL)
      process_event(w, &event);
  }
}

/**************************/
/**** Output functions ****/

/* Print a string */
INLINE static void print_string(WINDOW *w, Drawable d, int x, int y,
				char *str, int n)
{
  XDrawImageString(w->disp, d, w->gc, x, y + w->font->font_y, str, n);
}

/* Create a pixmap image for the window */
void win_createimage(WINDOW *w)
{
  w->image = XCreatePixmap(w->disp, w->window, w->w, w->h, w->depth);
  XSetForeground(w->disp, w->gc, COLOR(w->bkcolor));
  XFillRectangle(w->disp, w->image, w->gc, 0, 0, w->w, w->h);
  XSetForeground(w->disp, w->gc, COLOR(w->color));
}

/* Destroy the image of the window */
void win_destroyimage(WINDOW *w)
{
  if (w->image != None) {
    XFreePixmap(w->disp, w->image);
    w->image = None;
  }
}

/* Return the width of a string of size n */
int win_textwidth_n(WINDOW *w, char *txt, int n)
{
  XCharStruct cs;
  int dir, asc, des;

  if (n == 0)
    return 0;

  XTextExtents(w->font->font_struct, txt, n, &dir, &asc, &des, &cs);
  return cs.width;
}

/* Return the text height of a string of size n */
int win_textheight_n(WINDOW *w, char *txt, int n)
{
  XCharStruct cs;
  int dir, asc, des;

  XTextExtents(w->font->font_struct, txt, n, &dir, &asc, &des, &cs);
  return asc + des;
}

/* Return the text width of a string */
int win_textwidth(WINDOW *w, char *txt)
{
  return win_textwidth_n(w, txt, strlen(txt));
}

/* Return the text height of a string */
int win_textheight(WINDOW *w, char *txt)
{
  return win_textheight_n(w, txt, strlen(txt));
}

/* Return the window drawing color */
int win_getcolor(WINDOW *w)
{
  return w->color;
}

/* Set the window drawing color */
void win_setcolor(WINDOW *w, int color)
{
  if (w->color != color) {
    w->color = color;
    XSetForeground(w->disp, w->gc, COLOR(color));
  }
}

/* Set the drawing mode (one of the DM_* constants) */
void win_setdrawmode(WINDOW *w, int mode)
{
  w->draw_mode = mode;
  XSetFunction(w->disp, w->gc, mode);
}

/* Return the drawing mode */
int win_getdrawmode(WINDOW *w)
{
  return w->draw_mode;
}

/* Return the window background drawing color */
int win_getbackcolor(WINDOW *w)
{
  return w->bkcolor;
}

/* Set the window background drawing color */
void win_setbackcolor(WINDOW *w, int color)
{
  if (w->bkcolor != color) {
    w->bkcolor = color;
    XSetBackground(w->disp, w->gc, COLOR(color));
  }
}

/* Set the window border color */
void win_setbordercolor(WINDOW *w, int color)
{
  XSetWindowBorder(w->disp, w->window, COLOR(color));
}

/* Set the window border width */
void win_setborderwidth(WINDOW *w, int width)
{
  if (width < 0)
    width = 0;
  XSetWindowBorderWidth(w->disp, w->window, width);
}
/* Set the window background */
void win_setbackground(WINDOW *w, int color)
{
  XSetWindowBackground(w->disp, w->window, COLOR(color));
  XClearWindow(w->disp, w->window);
}

/* Set the cursor to appear over the window */
void win_setcursor(WINDOW *w, Cursor cursor)
{
  XDefineCursor(w->disp, w->window, cursor);
}

/* Return the window font */
FONT *win_getfont(WINDOW *w)
{
  return w->font;
}

/* Change the window font, return 0 on success or 1 in failure */
void win_setfont(WINDOW *w, FONT *f)
{
  if (w->font != NULL)
    free_font(w->font);

  f->num_refs++;
  w->font = f;
  XSetFont(xd_display, w->gc, w->font->font_struct->fid);
}

/* Set the window output position */
void win_moveto(WINDOW *w, int x, int y)
{
  w->px = x;
  w->py = y;
}

/* Draw a line to a point */
void win_lineto(WINDOW *w, int x, int y)
{
  XDrawLine(w->disp, w->window, w->gc, w->px, w->py, x, y);
  if (w->image != None)
    XDrawLine(w->disp, w->image, w->gc, w->px, w->py, x, y);
  w->px = x;
  w->py = y;
}

/* Draw a point in (x,y) */
void win_putpixel(WINDOW *w, int x, int y, int color)
{
  int old_color;

  old_color = win_getcolor(w);

  XSetForeground(w->disp, w->gc, COLOR(color));

  XDrawPoint(w->disp, w->window, w->gc, x, y);
  if (w->image != None)
    XDrawPoint(w->disp, w->image, w->gc, x, y);

  XSetForeground(w->disp, w->gc, COLOR(old_color));
}

/* Draw a line */
void win_line(WINDOW *w, int x1, int y1, int x2, int y2)
{
  XDrawLine(w->disp, w->window, w->gc, x1, y1, x2, y2);
  if (w->image != None)
    XDrawLine(w->disp, w->image, w->gc, x1, y1, x2, y2);
}

void win_circle(WINDOW *w, int x, int y, int radius)
{
  XDrawArc(w->disp, w->window, w->gc,
	   x - radius/2, y - radius/2, radius, radius, 0, 360*64);
  if (w->image != None)
    XDrawArc(w->disp, w->image, w->gc,
	     x - radius/2, y - radius/2, radius, radius, 0, 360*64);
}

/* Draw a rectangle */
void win_rect(WINDOW *w, int x1, int y1, int x2, int y2)
{
  XDrawRectangle(w->disp, w->window, w->gc, x1, y1, x2-x1, y2-y1);
  if (w->image != None)
    XDrawRectangle(w->disp, w->image, w->gc, x1, y1, x2-x1, y2-y1);
}

/* Draw a filled rectangle */
void win_rectfill(WINDOW *w, int x1, int y1, int x2, int y2)
{
  XFillRectangle(w->disp, w->window, w->gc, x1, y1, x2-x1+1, y2-y1+1);
  if (w->image != None)
    XFillRectangle(w->disp, w->image, w->gc, x1, y1, x2-x1+1, y2-y1+1);
}

/* Draw a string */
void win_outtext_n(WINDOW *w, char *txt, int n)
{
  if (n <= 0)
    return;
  print_string(w, w->window, w->px, w->py, txt, n);
  if (w->image != None)
    print_string(w, w->image, w->px, w->py, txt, n);
  w->px += win_textwidth_n(w, txt, n);
}

void win_outtext(WINDOW *w, char *txt)
{
  win_outtext_n(w, txt, strlen(txt));
}

/* Draw a string in a specific position */
void win_outtextxy_n(WINDOW *w, int x, int y, char *txt, int n)
{
  if (n <= 0)
    return;
  print_string(w, w->window, x, y, txt, n);
  if (w->image != None)
    print_string(w, w->image, x, y, txt, n);
}

void win_outtextxy(WINDOW *w, int x, int y, char *txt)
{
  win_outtextxy_n(w, x, y, txt, strlen(txt));
}


/*************************/
/**** Input functions ****/

/* Grab the pointer in the window `w'. If `confine' is not zero, the pointer
 * will be confined to the extents of the window.
 */
int win_grab_pointer(WINDOW *w, int confine, Cursor cursor)
{
  if (XGrabPointer(w->disp, w->window, False,
		   ButtonPressMask|ButtonMotionMask|ButtonReleaseMask,
		   GrabModeAsync, GrabModeAsync,
		   (confine) ? w->window : None,
		   (cursor == None) ? xd_arrow_cursor : cursor,
		   CurrentTime) != GrabSuccess)
    return 1;
  w->pointer_grabbed = 1;
  return 0;
}

/* Ungrab the pointer */
void win_ungrab_pointer(WINDOW *w)
{
  XUngrabPointer(w->disp, CurrentTime);
  w->pointer_grabbed = 0;
}

/* Returns 1 if there's a key waiting for be read, or returns 0
 * (immediatelly) if the keyboard buffer is empty
 */
int win_has_key(WINDOW *win)
{
  process_pending_events();
  return (win->pkb_i != win->pkb_f) ? 1 : 0;
}

/* Returns a key read from the window. This function wait until there is an
 * available key
 */
int win_read_key(WINDOW *win)
{
  XEvent event;
  int ret;

  while (win->pkb_i == win->pkb_f) {  /* While there's no key... */
    /* Read event or wait */
    XWindowEvent(win->disp, win->window, EV_MASK, &event);
    process_event(win, &event);
    kill_time();
  }
  ret = win->kb_buf[win->pkb_i];
  win->pkb_i = (win->pkb_i+1) % MAX_KB_BUF;
  return ret;
}

/* Reads the mouse pointer position and stores in (*x,*y), and returns the
 * state of the buttons.
 */
int win_read_mouse(WINDOW *win, int *ret_x, int *ret_y)
{
  Window root, ch;
  int rx, ry, x, y;
  unsigned int mask;

  process_pending_events();

  XQueryPointer(win->disp, win->window, &root, &ch, &rx, &ry, &x, &y, &mask);

  /* Store the information on the window structure */
  win->mouse_x = x;
  win->mouse_y = y;
  win->mouse_b = mask & (MOUSE_L | MOUSE_M | MOUSE_R);

  /* Return the information */
  if (ret_x != NULL)
    *ret_x = x;
  if (ret_y != NULL)
    *ret_y = y;

  return win->mouse_b;/* mask; */
}
