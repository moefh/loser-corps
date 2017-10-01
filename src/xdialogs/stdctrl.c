/* stdctrl.c - Standard control procedures.
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
 * ---
 * If the flag D_3D is set to a control, it will ignore its background
 * color and use xd_back_3d instead. It will also use the colors
 * xd_light_3d and xd_shadow_3d to create 3D effect. See BUGS below,
 * though.
 *
 * When a message MSG_DRAW is sent, the value of `c' is 1 if the control
 * can do a "soft" redraw (for example, when the control must be readrawn
 * because it stops from being hidden by a window), or is 0 if it must do
 * a "hard" redraw (for example, when the state of the control changed
 * and it must be redrawn to reflect the new state).
 *
 *
 * BUGS
 * ----
 * These are some things that are missing. Although it is possible to make
 * working dialogs without these features, it would be nice to have them
 * fixed/implemented/improved:
 *
 * - Edit controls should select text when an arrow key is pressed while
 *   shift is pressed. Delete and Backspace should remove selected text
 *   (if any) instead of a single character.
 *
 * - Lists are completely redrawn when the selection changes. Also, they
 *   don't draw or use the scroll bar (there's justa an empty space).
 *
 * - List and edit controls ignore the D_3D flag (they are always drawn in
 *   "2D"). The dialogs doesn't really look bad, but you should
 *   pay attention to the background color (I recommend COLOR_WHITE).
 *
 * - Menus completely ignore the keyboard.
 *
 * - I can't write documentation in english very well (did anyone notice
 *   that? :-)). If someone feels like rewritting or correcting something,
 *   please do.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "xdialogs.h"
#include "internal.h"


/* First control in a dialog.
 * Fields:
 *   d1 = index of the control with the focus in the dialog
 *   d2 = "lock count" of the dialog (number of locking childs)
 *   d3 = 1 if this window "locks" its parent, 0 if not
 *   d4 = 1 if the dialog has a window (i.e., if its window pointers
 *        are useful)
 *   d5 = 1 if the dialog has been initialized and has NOT been terminated,
 *        i.e., it's goot to receive messages.
 */
int window_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  switch (msg) {
  case MSG_INIT:
    if (dlg->flags & D_3D) {
      win_setbackground(win, xd_back_3d);
      dlg->d2 = 0;
    }
    if (dlg->flags & D_NORESIZE)
      win_set_min_max(win, win->w, win->h, win->w, win->h);
    break;

  case MSG_RESIZE:
    dlg->w = win->w;
    dlg->h = win->h;
    break;
  }
  return D_O_K;
}

/* Line control */
int hline_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  switch (msg) {
  case MSG_INIT:
    if (dlg->flags & D_3D)
      win_setbackground(win, xd_back_3d);
    win_setborderwidth(win, 0);
  case MSG_RESIZE:
    if (dlg->flags & D_AUTOW) {
      dlg->w = win->parent->w - 2 * dlg->x;
      set_dialog_position(dlg);
    }
    break;

  case MSG_DRAW:
    if (dlg->flags & D_3D) {
      win_setcolor(win, xd_shadow_3d);
      win_line(win, 0, 0, dlg->w - 1, 0);
      win_setcolor(win, xd_light_3d);
      win_line(win, 0, 1, dlg->w - 1, 1);
    } else {
      win_setcolor(win, dlg->fg);
      win_line(win, 0, 0, dlg->w - 1, 0);
    }
    break;
  }
  return D_O_K;
}

/* Box control */
int box_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  return D_O_K;
}

/* Filled box control */
int fillbox_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  if (msg == MSG_INIT)
    win_setborderwidth(win, 0);
  return D_O_K;
}

/* Extended button control.
 * Dialog fields are the same as in `button_proc' with an addition:
 *
 * dp1 = (function to call when the button is clicked)
 */
int xbutton_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  int ret;

  ret = button_proc(msg, win, dlg, c);

  if (ret & D_CLOSE && dlg->dp1 != NULL)
    return ((int (*)(WINDOW *, DIALOG *, int))dlg->dp1)(win, dlg, c);
  return ret;
}

/* Button control
 *
 * Dialog fields:
 * dp = pointer to '\0'-terminated string containing the button text.
 */
int button_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  int fg, bg, sel;
  int delta;

  switch (msg) {
  case MSG_INIT:
    if (dlg->flags & D_3D) {
      win_setbordercolor(win, ((dlg->flags & D_GOTFOCUS) ? dlg->fg :
			       xd_back_3d));
      win_setbackground(win, xd_back_3d);
    }
    if (dlg->flags & D_AUTOW)
      dlg->w = win_textwidth(win, dlg->dp) + 6;
    if (dlg->flags & D_AUTOH)
      dlg->h = win_textheight(win, dlg->dp) + 10;
    set_dialog_position(dlg);
    break;

  case MSG_DRAW:
    delta = 0;
    if (dlg->flags & D_3D) {
      /* Draw the border */
      if (dlg->flags & D_SELECTED) {
	delta = 2;
	fg = xd_shadow_3d;
	bg = xd_light_3d;
      } else {
	fg = xd_light_3d;
	bg = xd_shadow_3d;
      }
      /* Clear the area */
      if (c == 0) {
	win_setcolor(win, xd_back_3d);
	win_rectfill(win, delta, delta, dlg->w - 1, dlg->h - 1);
      }
      /* Draw the shadow */
      win_setcolor(win, fg);
      win_moveto(win, 0, dlg->h - 1);
      win_lineto(win, 0, 0);
      win_lineto(win, dlg->w - 1, 0);
      win_moveto(win, 1, dlg->h - 1);
      win_lineto(win, 1, 1);
      win_lineto(win, dlg->w - 1, 1);

      if ((dlg->flags & D_SELECTED) == 0) {
	win_setcolor(win, bg);
	win_moveto(win, 0, dlg->h - 1);
	win_lineto(win, dlg->w - 1, dlg->h - 1);
	win_lineto(win, dlg->w - 1, 0);
	win_moveto(win, 1, dlg->h - 2);
	win_lineto(win, dlg->w - 2, dlg->h - 2);
	win_lineto(win, dlg->w - 2, 1);
      }
      win_setcolor(win, dlg->fg);
      win_setbackcolor(win, xd_back_3d);
    } else {   /* ! (dlg->flags & D_3D) */
      if (dlg->flags & D_SELECTED) {
	fg = dlg->bg;
	bg = dlg->fg;
      } else {
	bg = dlg->bg;
	fg = dlg->fg;
      }
      if (c == 0) {
	win_setcolor(win, bg);
	win_rectfill(win, 0, 0, dlg->w - 1, dlg->h - 1);
      }
      if (dlg->flags & D_GOTMOUSE) {
	win_setcolor(win, dlg->fg);
	win_rect(win, 0, 0, dlg->w - 1, dlg->h - 1);
      }
      win_setcolor(win, fg);
      win_setbackcolor(win, bg);
    }

    win_outtextxy(win,
		  delta + (dlg->w - win_textwidth(win, dlg->dp)) / 2,
		  delta + (dlg->h - win_textheight(win, dlg->dp)) / 2,
		  dlg->dp);

    /* Draw the selection box */
    if (! (dlg->flags & D_3D)) {
      if (dlg->flags & D_GOTFOCUS)
	win_setcolor(win, dlg->fg);
      else
	win_setcolor(win, dlg->bg);
      win_rect(win, 1, 1, dlg->w - 2, dlg->h - 2);
    }
    if (c == 0)
      XFlush(win->disp);
    return D_O_K;

  case MSG_WANTFOCUS:
    return D_WANTFOCUS;

  case MSG_GOTMOUSE:
  case MSG_LOSTMOUSE:
    if (! (dlg->flags & D_3D))
      SEND_MESSAGE(MSG_DRAW, dlg, 0);
    return D_O_K;

  case MSG_GOTFOCUS:
  case MSG_LOSTFOCUS:
    if (dlg->flags & D_3D)
      win_setbordercolor(win, ((msg == MSG_GOTFOCUS) ? dlg->fg :
			       xd_back_3d));
    SEND_MESSAGE(MSG_DRAW, dlg, 0);
    return D_O_K;

  case MSG_KEY:
    if (KEYSYM(c) == XK_Return && KEYMOD(c) == 0)
      return ((dlg->flags & D_EXIT) ? D_CLOSE :  D_USED_CHAR);
    return D_O_K;

  case MSG_CLICK:
    win_grab_pointer(win, 0, None);
    do {
      if (point_in_rect(win->mouse_x, win->mouse_y, 0, 0,
			dlg->w, dlg->h)) {
	if (! (dlg->flags & D_SELECTED)) {
	  dlg->flags |= D_SELECTED;
	  SEND_MESSAGE(MSG_DRAW, dlg, 0);
	}
      }	else {
	if (dlg->flags & D_SELECTED) {
	  dlg->flags &= ~D_SELECTED;
	  SEND_MESSAGE(MSG_DRAW, dlg, 0);
	}
      }
    } while (win_read_mouse(win, NULL, NULL));
    win_ungrab_pointer(win);

    sel = ((dlg->flags & D_SELECTED) != 0);
    dlg->flags &= ~(D_SELECTED|D_GOTMOUSE);
    SEND_MESSAGE(MSG_DRAW, dlg, 0);
    if ((dlg->flags & D_EXIT) && sel)
      return D_CLOSE;
    break;
  }

  return D_O_K;
}

/* Static text control
 *
 * Dialog fields:
 * dp = pointer to '\0'-terminated string containing the text to
 *      be displayed.
 */
int text_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  int x, y;

  switch (msg) {
  case MSG_INIT:
    win_setborderwidth(win, 0);
    if (dlg->flags & D_3D)
      win_setbackground(win, xd_back_3d);
    if (dlg->flags & D_AUTOW)
      dlg->w = win_textwidth(win, dlg->dp) + 4;
    if (dlg->flags & D_AUTOH)
      dlg->h = win_textheight(win, dlg->dp) + 4;
    set_dialog_position(dlg);
    break;

  case MSG_DRAW:
    if (c == 0) {    /* "Active" redraw */
      win_setcolor(win, (dlg->flags & D_3D) ? xd_back_3d : dlg->bg);
      win_rectfill(win, 0, 0, dlg->w - 1, dlg->h - 1);
    }
    if (dlg->dp == NULL)
      break;
    win_setcolor(win, dlg->fg);
    win_setbackcolor(win, (dlg->flags & D_3D) ? xd_back_3d : dlg->bg);

    y = (dlg->h - win_textheight(win, dlg->dp)) / 2;
    switch (dlg->d1) {
    case 0: x = 2; break;
    case 1: x = (dlg->w - win_textwidth(win, dlg->dp)) / 2; break;
    default: x = dlg->w - win_textwidth(win, dlg->dp) - 2; break;
    }
    win_outtextxy(win, x, y, dlg->dp);
    break;
  }
  return D_O_K;
}

static void delete_selection(DIALOG *d)
{
  int i;
  char *txt;

  if (d->d4 >= d->d5)
    return;

  txt = (char *) d->dp;

  for (i = 0; txt[i + d->d5] != '\0'; i++)
    txt[i + d->d4] = txt[i + d->d5];
  txt[i + d->d4] = '\0';

  d->d1 = d->d4;
  d->d4 = d->d5 = 0;
}

/* Extended edit control.
 * Dialog fields are the same as in `edit_proc' with an addition:
 *
 * dp1 = (function to call when the edit box terminates the dialog)
 */
int xedit_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  int ret;

  ret = edit_proc(msg, win, dlg, c);

  if (ret & D_CLOSE && dlg->dp1 != NULL)
    return ((int (*)(WINDOW *, DIALOG *, int))dlg->dp1)(win, dlg, c);
  return ret;
}

/* Edit text control.
 *
 * Dialog fields:
 * d1 = position of the cursor in the text
 * d2 = maximum number of chars accepted
 * d3 = index of character on the left of the text box
 * d4 = start of selection
 * d5 = end of selection
 * dp = pointer to string containing the text
 */
int edit_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  char *txt;
  int i = 0;

  txt = dlg->dp;
  if (txt == NULL && msg != MSG_INIT)
    return D_O_K;

  switch (msg) {
  case MSG_INIT:
    if (txt != NULL)
      dlg->d1 = strlen(txt);
    win_setcursor(win, xd_text_cursor);
    if (dlg->flags & D_AUTOW)
      dlg->w = win_textwidth(win, dlg->dp) + 6;
    if (dlg->flags & D_AUTOH)
      dlg->h = win_textheight(win, dlg->dp) + 6;
    set_dialog_position(dlg);
    if (dlg->d1 < 0)
      dlg->d1 = 0;
    if (dlg->d2 < 0)
      dlg->d2 = 0;
    if (dlg->d1 > dlg->d2)
      dlg->d1 = dlg->d2;
    if (strlen(txt) > dlg->d2)
      txt[dlg->d2] = '\0';
    dlg->d3 = dlg->d4 = dlg->d5 = 0;
    break;

  case MSG_WANTFOCUS:
    return D_WANTFOCUS;

  case MSG_GOTFOCUS:
  case MSG_LOSTFOCUS:
    SEND_MESSAGE(MSG_DRAW, dlg, 0);
    break;

  case MSG_DRAW:
    {
      int end, y;

      y = (dlg->h - win_textheight(win, txt)) / 2;
      i = strlen(txt);

      for (end = dlg->d3; end < i; end++)
	if (win_textwidth_n(win, txt + dlg->d3, end - dlg->d3) > dlg->w - 4) {
	  end--;
	  break;
	}
      if (end < 0)
	end = 0;

      if (c == 0) {
	win_setcolor(win, dlg->bg);
	win_rectfill(win, 0, 0, dlg->w - 1, dlg->h - 1);
      }
      if ((dlg->flags & D_GOTFOCUS) == 0 || dlg->d4 >= dlg->d5
	  || dlg->d5 <= dlg->d3 || dlg->d4 >= end) {
	/* The simplest case: no selection or selection is not visible */
	win_setcolor(win, dlg->fg);
	win_setbackcolor(win, dlg->bg);
	win_outtextxy_n(win, 2, y, txt + dlg->d3, end - dlg->d3);
      } else {
	if (dlg->d4 > dlg->d3) {
	  /* Draw text BEFORE selection */
	  win_setcolor(win, dlg->fg);
	  win_setbackcolor(win, dlg->bg);
	  win_outtextxy_n(win, 2, y, txt + dlg->d3, dlg->d4 - dlg->d3);
	}
	if (dlg->d5 > dlg->d4) {
	  /* Draw SELECTED text */
	  win_setcolor(win, dlg->bg);
	  win_setbackcolor(win, dlg->fg);
	  if (dlg->d4 > dlg->d3)
	    win_outtextxy_n(win, 2 + win_textwidth_n(win, txt + dlg->d3,
						     dlg->d4 - dlg->d3),
			    y, txt + dlg->d4, MIN(end, dlg->d5) - dlg->d4);
	  else
	    win_outtextxy_n(win, 2, y, txt + dlg->d3,
			    MIN(end, dlg->d5) - dlg->d3);
	}
	if (dlg->d5 < end) {
	  /* Draw text AFTER the selection */
	  win_setcolor(win, dlg->fg);
	  win_setbackcolor(win, dlg->bg);
	  win_outtextxy_n(win, 2 + win_textwidth_n(win, txt + dlg->d3,
						   dlg->d5 - dlg->d3),
			  y, txt + dlg->d5, end - dlg->d5);
	}
      }

      if (dlg->flags & D_GOTFOCUS) {
	int x;

	x = win_textwidth_n(win, txt + dlg->d3, dlg->d1 - dlg->d3) + 2;
	win_setcolor(win, COLOR_RED);
	win_line(win, x, 1, x, dlg->h - 2);
      }
      break;
    }

  case MSG_CLICK:
    {
      int x, n, ini_pos, new_pos;
      char ch[2];

      n = strlen(txt);

      dlg->d1 = -1;

      /* See if the click was in one of the extremes */
      ch[0] = txt[0];
      ch[1] = '\0';
      if (win->mouse_x < 2 + win_textwidth(win, ch) / 2)
	dlg->d1 = dlg->d3;
      else {
	ch[0] = txt[(n > 0) ? n-1 : n];
	ch[1] = '\0';
	if (win->mouse_x > win_textwidth(win, txt + dlg->d3)
	    - win_textwidth(win, ch)/2)
	  dlg->d1 = n;
      }

      /* If clicked in the middle of the text... */
      if (dlg->d1 < 0) {
	for (x = dlg->d3; x < n; x++) {
	  ch[0] = txt[x];
	  if (win->mouse_x - 2 <
	      win_textwidth_n(win, txt + dlg->d3, x - dlg->d3)
	      + win_textwidth(win, ch) / 2) {
	    dlg->d1 = x;
	    break;
	  }
	}
	if (dlg->d1 < 0)
	  dlg->d1 = i;
      }
      dlg->d4 = dlg->d5 = 0;           /* Cancel current selection */
      SEND_MESSAGE(MSG_DRAW, dlg, 0);

      /* Now, if the mouse is being dragged... */
      ini_pos = new_pos = dlg->d1;
      win_grab_pointer(win, 0, xd_text_cursor);
      while (win_read_mouse(win, NULL, NULL)) {
	new_pos = -1;
	for (x = dlg->d3; x < n; x++) {
	  if (win_textwidth_n(win, txt + dlg->d3, x - dlg->d3) > dlg->w - 4) {
	    new_pos = x;
	    break;
	  }
	  ch[0] = txt[x];
	  if (win->mouse_x - 2 <
	      win_textwidth_n(win, txt + dlg->d3, x - dlg->d3)
	      + win_textwidth(win, ch) / 2) {
	    new_pos = x;
	    break;
	  }
	}
	if (new_pos < 0)
	  new_pos = n;
	if (new_pos > 0 && win->mouse_x < 0) {
	  new_pos--;
	  usleep(100000);
	} else if (new_pos < n && win->mouse_x > dlg->w) {
	  new_pos++;
	  usleep(100000);
	}
	if (new_pos != dlg->d1) {
	  if (new_pos < ini_pos) {
	    dlg->d4 = new_pos;
	    dlg->d5 = ini_pos;
	  } else {
	    dlg->d4 = ini_pos;
	    dlg->d5 = new_pos;
	  }
	  dlg->d1 = new_pos;
	  if (dlg->d1 < dlg->d3)
	    dlg->d3 = dlg->d1;
	  while (win_textwidth_n(win, txt + dlg->d3, dlg->d1 - dlg->d3)
		 > dlg->w - 4)
	    dlg->d3++;
	  SEND_MESSAGE(MSG_DRAW, dlg, 0);
	}
      }
      win_ungrab_pointer(win);
    }
    break;

  case MSG_KEY:
    {
      int preserv_sel = 0;
      int code, modif, num;

      num = strlen(txt);
      code = KEYSYM(c);
      modif = KEYMOD(c);

      if (code >= ' ' && code <= 127) {
	delete_selection(dlg);
	if (num >= dlg->d2)
	  break;
	for (i = num; i >= dlg->d1; i--)
	  txt[i+1] = txt[i];
	txt[dlg->d1] = code;
	dlg->d1++;
      } else switch (code) {
      case XK_Insert:
	preserv_sel = 1;
	if (modif & ShiftMask) {
	  char *p;
	  int n;

	  if (num >= dlg->d2)
	    break;
	  p = malloc(dlg->d2 - num);
	  n = win_get_selection(win, p, dlg->d2 - num);
	  for (i = 0; i < n; i++)
	    SEND_MESSAGE(MSG_KEY, dlg, p[i]);
	  free(p);
	} else if (modif & ControlMask) {
	  if (dlg->d4 < dlg->d5)
	    win_set_selection(win, txt + dlg->d4, dlg->d5 - dlg->d4);
	}
	break;

      case XK_Return: if (dlg->flags & D_EXIT) return D_CLOSE; break;
      case XK_Home: dlg->d1 = 0; break;
      case XK_Left: if (dlg->d1 > 0) dlg->d1--; break;
      case XK_Right: if (dlg->d1 < num) dlg->d1++; break;
      case XK_End:
	dlg->d1 = num;
	break;

      case XK_BackSpace:
	if (dlg->d4 < dlg->d5) {
	  delete_selection(dlg);
	  break;
	}
	if (dlg->d1 == 0)
	  break;
	for (i = dlg->d1 - 1; txt[i] != '\0'; i++)
	  txt[i] = txt[i+1];
	dlg->d1--;
	break;

      case XK_Delete:
	if (dlg->d4 < dlg->d5) {
	  delete_selection(dlg);
	  break;
	}
	for (i = dlg->d1; txt[i] != '\0'; i++)
	  txt[i] = txt[i+1];
	break;

      default:
	preserv_sel = 1;
	return D_O_K;
      }

      num = strlen(txt);

      /* Make sure the insertion point is not to the left */
      if (dlg->d1 < dlg->d3)
	dlg->d3 = dlg->d1;

      /* Make sure the insertion point is not to the right */
      while (win_textwidth_n(win, txt + dlg->d3, dlg->d1 - dlg->d3)
	     > dlg->w - 4)
	dlg->d3++;

      if (! preserv_sel)
	dlg->d4 = dlg->d5 = 0;
      SEND_MESSAGE(MSG_DRAW, dlg, 0);
      return D_USED_CHAR;
    }
  }

  return D_O_K;
}

/* Password edit control.
 * This function is like the edit control, but does not display the text.
 */
int password_proc(int msg, WINDOW *w, DIALOG *dlg, int c)
{
  static char no_text[1];
  char *dp;
  int d1, d2;

  switch (msg) {
  case MSG_DRAW:
    d1 = dlg->d1;
    d2 = dlg->d2;
    dp = (char *) dlg->dp;
    dlg->d1 = dlg->d2 = 0;
    dlg->dp = (void *) no_text;
    edit_proc(MSG_DRAW, w, dlg, c);
    dlg->d1 = d1;
    dlg->d2 = d2;
    dlg->dp = (void *) dp;
    return D_O_K;

  case MSG_IDLE:
  case MSG_CLICK:
  case MSG_GOTMOUSE:
  case MSG_LOSTMOUSE:
    return D_O_K;

  default:
    return edit_proc(msg, w, dlg, c);
  }
}


/* Image control: shows a pixmap */
int image_proc(int msg, WINDOW *w, DIALOG *dlg, int c)
{
  switch (msg) {
  case MSG_INIT:
    win_createimage(w);
    break;

  case MSG_DRAW:
    XCopyArea(w->disp, w->image, w->window, w->gc, 0, 0, dlg->w, dlg->h, 0, 0);
    break;

  case MSG_END:
    win_destroyimage(w);
    break;
  }
  return D_O_K;
}

/* Displays a list of strings
 *
 * Dialog fields:
 * d1 = index of currently selected item
 * d2 = index of item at the top of the list
 * dp = pointer to `char *list_getter(int index, int *num_items)'
 *      functions used to get the items.
 */
int list_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  int n_items;
  char *(*get_items)(int index, int *n_items);

  get_items = dlg->dp;
  switch (msg) {
  case MSG_INIT:
    {
      int i, h;

      get_items(-1, &n_items);
      if (dlg->flags & D_AUTOH) {
	for (h = i = 0; h < dlg->h; i++)
	  h += win_textheight(win, " ") + 3;
	dlg->h = (i - 1) * (win_textheight(win, " ") + 3) + 2;
	set_dialog_position(dlg);
      }
      if (dlg->d1 >= n_items)
	dlg->d1 = n_items - 1;
      if (dlg->d2 >= n_items - 1)
	dlg->d2 = n_items - 2;
      if (dlg->d1 < 0)
	dlg->d1 = 0;
      if (dlg->d2 < 0)
	dlg->d2 = 0;
    }
    return D_O_K;

  case MSG_GOTFOCUS:
  case MSG_LOSTFOCUS:
    SEND_MESSAGE(MSG_DRAW, dlg, 0);
    return D_O_K;

  case MSG_DRAW:
    {
      int i, h1, h2;
      int fg, bg;
      char *txt_item;

      get_items(-1, &n_items);

      h1 = dlg->h / (win_textheight(win, "LIST") + 3);
      if (n_items > h1 && dlg->d2 > n_items - h1)
	dlg->d2 = n_items - h1;

      h1 = h2 = 1;
      for (i = dlg->d2; i < n_items; i++) {
	txt_item = get_items(i, NULL);
	if (txt_item[0] == '\0')
	  h2 += win_textheight(win, "LIST") + 3;
	else
	  h2 += win_textheight(win, txt_item) + 3;
	if (h2 > dlg->h)
	  break;

	/* Set the color to inverse if this item is selected */
	if (dlg->d1 == i) {
	  fg = dlg->bg;
	  bg = dlg->fg;
	} else {
	  fg = dlg->fg;
	  bg = dlg->bg;
	}
	/* Clear the item area if inverse or if this is a "hard" drawing */
	if (c == 0 || dlg->d1 == i) {
	  win_setcolor(win, bg);
	  win_rectfill(win, 1, h1, dlg->w - 17, h2 - 1);
	}
	win_setcolor(win, fg);
	win_setbackcolor(win, bg);
	win_outtextxy(win, 4, h1 + 1, txt_item);
	h1 = h2;
      }
      /* Fill the rest of the window */
      if (c == 0 && h1 < dlg->h - 2) {
	win_setcolor(win, dlg->bg);
	win_rectfill(win, 1, h1, dlg->w - 17, dlg->h - 2);
      }
      /* FIXME: Draw the scroll bar */
      win_setcolor(win, dlg->fg);
      win_line(win, dlg->w - 15, 0, dlg->w - 15, dlg->h - 1);
      win_setcolor(win, dlg->bg);
      win_rectfill(win, dlg->w - 14, 1, dlg->w - 1, dlg->h - 1);

      if (dlg->flags & D_GOTFOCUS)
	win_setcolor(win, dlg->fg);
      else
	win_setcolor(win, dlg->bg);
      win_rect(win, 0, 0, dlg->w - 16, dlg->h - 1);
      return D_O_K;
    }

  case MSG_WANTFOCUS:
    return D_WANTFOCUS;

  case MSG_KEY:
    {
      int used = 1, draw = 0, h, n_view;

      get_items(-1, &n_items);
      for (h = n_view = 0; h < dlg->h; n_view++)
	h += win_textheight(win, " ") + 3;
      n_view--;

      switch (c) {
      case XK_Home:
	draw = 1;
	dlg->d1 = dlg->d2 = 0;
	break;
      case XK_End:
	draw = 1;
	dlg->d1 = n_items - 1;
	dlg->d2 = n_items - n_view;
	break;
      case XK_Left:
      case XK_Up:
	if (dlg->d1 > 0) {
	  draw = 1;
	  dlg->d1--;
	  if (dlg->d1 < dlg->d2)
	    dlg->d2 = dlg->d1;
	}
	break;
      case XK_Right:
      case XK_Down:
	if (dlg->d1 < n_items-1) {
	  draw = 1;
	  dlg->d1++;
	  if (dlg->d1 >= dlg->d2 + n_view)
	    dlg->d2++;
	}
	break;	
      default:
	used = 0;
      }
      if (draw)
	SEND_MESSAGE(MSG_DRAW, dlg, 0);
      if (used)
	return D_USED_CHAR;
      return D_O_K;
    }

  case MSG_DCLICK:
    if (dlg->flags & D_EXIT)
      return D_CLOSE;
    break;

  case MSG_CLICK:
    {
      int i, h1, h2, n_view, change;

      get_items(-1, &n_items);
      for (h1 = n_view = 0; h1 < dlg->h; n_view++)
	h1 += win_textheight(win, " ") + 3;
      n_view--;

      win_grab_pointer(win, 0, None);
      do {
	change = 0;
	h1 = h2 = 1;
	for (i = dlg->d2; i < dlg->d2 + n_view && i < n_items; i++) {
	  h2 += win_textheight(win, " ") + 3;
	  if (point_in_rect(win->mouse_x, win->mouse_y,
			    0, h1, dlg->w, h2) && dlg->d1 != i) {
	    dlg->d1 = i;
	    SEND_MESSAGE(MSG_DRAW, dlg, 0);
	    change = 1;
	    break;
	  }
	  h1 = h2;
	}
	if (! change && win->mouse_y > dlg->h - 1
	    && dlg->d2 + n_view < n_items) {
	  usleep(100000);
	  dlg->d2++;
	  dlg->d1 = dlg->d2 + n_view - 1;
	  SEND_MESSAGE(MSG_DRAW, dlg, 0);
	} else if (! change && dlg->d2 > 0 && win->mouse_y < 0) {
	  usleep(100000);
	  dlg->d2--;
	  dlg->d1 = dlg->d2;
	  SEND_MESSAGE(MSG_DRAW, dlg, 0);
	}
      } while (win_read_mouse(win, NULL, NULL));
      win_ungrab_pointer(win);
      return D_O_K;
    }
  }
  return D_O_K;
}

/* Check box procedure
 *
 * Fields:
 * d1 = 1 if selected, 0 if not
 * dp = pointer to '\0'-terminated string containing the check box text.
 */
int check_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  switch (msg) {
  case MSG_INIT:
    win_setborderwidth(win, 0);
    if (dlg->flags & D_3D)
      win_setbackground(win, xd_back_3d);
    break;

  case MSG_WANTFOCUS:
    return D_WANTFOCUS;

  case MSG_GOTFOCUS:
  case MSG_LOSTFOCUS:
    SEND_MESSAGE(MSG_DRAW, dlg, 1);
    break;

  case MSG_DRAW:
    {
      int dim, ini;

      dim = win_textheight(win, dlg->dp);
      if (dim == 0)
	dim = 15;
      ini = (dlg->h - dim) / 2;

      if (c == 0) {
	win_setcolor(win, (dlg->flags & D_3D) ? xd_back_3d : dlg->bg);
	win_rectfill(win, 0, 0, dlg->w - 1, dlg->h - 1);
      }

      win_setcolor(win, dlg->fg);
      win_setbackcolor(win, (dlg->flags & D_3D) ? xd_back_3d : dlg->bg);
      win_rect(win, 2, ini, dim + 2, ini + dim);
      win_outtextxy(win, dim + 8, ini, dlg->dp);
      if (dlg->flags & D_GOTFOCUS)
	win_setcolor(win, dlg->fg);
      else
	win_setcolor(win, (dlg->flags & D_3D) ? xd_back_3d : dlg->bg);
      win_rect(win, dim + 6, ini - 1,
	       dim + 10 + win_textwidth(win, dlg->dp), ini + dim + 1);

      if (dlg->d1)
	win_setcolor(win, dlg->fg);
      else
	win_setcolor(win, (dlg->flags & D_3D) ? xd_back_3d : dlg->bg);
      win_line(win, 3, ini + 1, dim + 1, ini + dim - 1);
      win_line(win, 3, ini + dim - 1, dim + 1, ini + 1);
      break;
    }

  case MSG_CLICK:
    {
      int ini_state, in_box;
      int dim, ini;

      dim = win_textheight(win, dlg->dp);
      if (dim == 0)
	dim = 15;
      ini = (dlg->h - dim) / 2;

      ini_state = dlg->d1;
      in_box = 0;
      win_grab_pointer(win, 0, None);
      do {
	if (point_in_rect(win->mouse_x, win->mouse_y,
			  2, ini, 2 + dim, ini + dim)) {
	  if (! in_box) {
	    in_box = 1;
	    dlg->d1 = ! ini_state;
	    SEND_MESSAGE(MSG_DRAW, dlg, 1);
	  }
	} else {
	  if (in_box) {
	    in_box = 0;
	    dlg->d1 = ini_state;
	    SEND_MESSAGE(MSG_DRAW, dlg, 1);
	  }
	}
      } while (win_read_mouse(win, NULL, NULL));
      win_ungrab_pointer(win);
      SEND_MESSAGE(MSG_DRAW, dlg, 1);
      break;
    }
  }
  return D_O_K;
}

/* Draw a menu item */
static void draw_menu_item(WINDOW *win, int x, int y, int w, int h,
			   int fg, int bg, int sel, int do_3d, char *txt)
{
  if (do_3d) {
    bg = xd_back_3d;
    win_setcolor(win, bg);
    if (sel) { 
      win_rectfill(win, x + 1, y + 1, x + w - 2, y + h - 2);
      win_setcolor(win, xd_light_3d);
      win_moveto(win, x, y + h - 1);
      win_lineto(win, x, y);
      win_lineto(win, x + w - 1, y);
      win_setcolor(win, xd_shadow_3d);
      win_moveto(win, x, y + h - 1);
      win_lineto(win, x + w - 1, y + h - 1);
      win_lineto(win, x + w - 1, y);
    } else
      win_rectfill(win, x, y, x + w - 1, y + h - 1);
  } else {
    if (sel) {
      int tmp;

      tmp = fg;
      fg = bg;
      bg = tmp;
    }
    win_setcolor(win, bg);
    win_rectfill(win, x, y, x + w - 1, y + h - 1);
  }

  if (txt[0] != '\0') {
    win_setcolor(win, fg);
    win_setbackcolor(win, bg);
    win_outtextxy(win, x + 5, y + ((do_3d) ? 1 : 0), txt);
  } else {
    if (do_3d) {
      win_setcolor(win, xd_shadow_3d);
      win_line(win, 1, y + h / 2 - 1, w, y + h / 2 - 1);
      win_setcolor(win, xd_light_3d);
      win_line(win, 1, y + h / 2, w, y + h / 2);
    } else {
      win_setcolor(win, fg);
      win_line(win, 0, y + h / 2, w, y + h / 2);
    }
  }
}

/* Return what submenu was pressed */
static int what_submenu(WINDOW *win, DIALOG *dlg)
{
  MENU *menu;
  int sel, i, w, t;

  menu = dlg->dp;

  if (win->mouse_y < 0 || win->mouse_y > dlg->h)
    return -1;

  sel = -1;
  w = 5;
  for (i = 0; menu[i].text != NULL; i++) {
    t = win_textwidth(win, menu[i].text) + 10;
    if (win->mouse_x >= w && win->mouse_x <= w + t) {
      sel = i;
      break;
    }
    w += t;
  }
  return sel;
}

static int what_menuitem(WINDOW *win, MENU *menu, int item_h, int do_3d)
{
  int i;

  if (win->mouse_x < 0 || win->mouse_y < 0 ||
      win->mouse_x >= win->w || win->mouse_y >= win->h)
    return -1;

  for (i = 0; menu[i].text != NULL; i++)
    if (point_in_rect(win->mouse_x, win->mouse_y,
		      0, i * item_h + ((do_3d) ? 1 : 0),
		      win->w, (i + 1) * item_h))
      return i;
  return -1;
}

/* Do a menu in the specified position */
int do_menu(WINDOW *parent, MENU *menu, int x, int y)
{
  MENU *menu_sel;
  int sel;

  menu_sel = menu;
  sel = do_menu_x(parent, &menu_sel, x, y, gui_fg_color, gui_bg_color,
		  xd_stddlg_3d);
  if (sel >= 0 && menu_sel[sel].proc != NULL)
    menu_sel[sel].proc(parent, menu_sel[sel].id);
  if (menu_sel == menu)
    return sel;
  return -1;
}

/* Create a popup menu window and return the selected window */
int do_menu_x(WINDOW *parent, MENU **pmenu, int pos_x, int pos_y,
	      int fg, int bg, int do_3d)
{
  WINDOW *menu_win;
  MENU *menu;
  int i, t, w, h, item_h;
  int sel, mouse_b;
  int scr_w, scr_h;

  scr_w = WidthOfScreen(DefaultScreenOfDisplay(xd_display));
  scr_h = HeightOfScreen(DefaultScreenOfDisplay(xd_display));
  menu = *pmenu;

  /* Create the menu window */
  menu_win = create_window_x("", pos_x, pos_y, 1, 1, 0, NULL, NULL);
  if (parent != NULL)
    win_setfont(menu_win, win_getfont(parent));

  /* Calculate the item and window sizes */
  item_h = 0;
  w = 50;
  for (i = 0; menu[i].text != NULL; i++)
    if (menu[i].text[0] != '\0') {
      if (item_h == 0)
	item_h = win_textheight(menu_win, menu[i].text);
      t = win_textwidth(menu_win, menu[i].text);
      if (w < t)
	w = t;
    }
  if (item_h == 0)
    item_h = win_textheight(menu_win, "MENU");
  item_h += (do_3d) ? 4 : 2;
  w += 40;
  h = i * item_h;

  /* Set the menu window size */
  if (pos_x + w >= scr_w)
    pos_x = scr_w - w - 1;
  if (pos_y + h >= scr_h)
    pos_y = scr_h - h - 1;
  if (pos_x < 0)
    pos_x = 0;
  if (pos_y < 0)
    pos_y = 0;
  win_movewindow(menu_win, pos_x, pos_y);
  win_resizewindow(menu_win, w + ((do_3d) ? 2 : 0), h + ((do_3d) ? 2 : 0));
  win_setborderwidth(menu_win, (do_3d) ? 0 : 1);
  win_setbackground(menu_win, (do_3d) ? xd_back_3d : bg);
  win_showwindow(menu_win);

  if (do_3d) {
    /* Draw the 3d effect */
    win_setcolor(menu_win, xd_light_3d);
    win_moveto(menu_win, w + 1, 0);
    win_lineto(menu_win, 0, 0);
    win_lineto(menu_win, 0, h + 1);
    win_setcolor(menu_win, xd_shadow_3d);
    win_moveto(menu_win, w + 1, 0);
    win_lineto(menu_win, w + 1, h + 1);
    win_lineto(menu_win, 0, h + 1);
  }
  for (i = 0; menu[i].text != NULL; i++)
    draw_menu_item(menu_win,
		   (do_3d) ? 1 : 0, i * item_h + ((do_3d) ? 1 : 0),
		   w, item_h, fg, bg, 0, do_3d, menu[i].text);

  /* Show the selected item while the button doesn't change */
  {
    int mx, my;
      
    win_grab_pointer(menu_win, 0, None);
    mouse_b = win_read_mouse(menu_win, &mx, &my);
    while (ABS(DIST2(menu_win->mouse_x - mx, menu_win->mouse_y - my)) < 10)
      mouse_b = win_read_mouse(menu_win, NULL, NULL);
    win_ungrab_pointer(menu_win);
  }
  sel = -1;
  while (1) {
    if (do_3d) {
      /* Draw the 3d effect */
      win_setcolor(menu_win, xd_light_3d);
      win_moveto(menu_win, w + 1, 0);
      win_lineto(menu_win, 0, 0);
      win_lineto(menu_win, 0, h + 1);
      win_setcolor(menu_win, xd_shadow_3d);
      win_moveto(menu_win, w + 1, 0);
      win_lineto(menu_win, w + 1, h + 1);
      win_lineto(menu_win, 0, h + 1);
    }
    for (i = 0; menu[i].text != NULL; i++)
      draw_menu_item(menu_win,
		     (do_3d) ? 1 : 0, i * item_h + ((do_3d) ? 1 : 0),
		     w, item_h, fg, bg, 0, do_3d, menu[i].text);

    if (win_grab_pointer(menu_win, 0, None)) {
      close_window(menu_win);
      XBell(menu_win->disp, 100);
      return D_O_K;
    }
    while (win_read_mouse(menu_win, NULL, NULL) == mouse_b) {
      i = what_menuitem(menu_win, menu, item_h, do_3d);
      
      if (i >= 0 && i != sel) {
	if (sel >= 0)
	  draw_menu_item(menu_win, (do_3d) ? 1 : 0,
			 sel * item_h + ((do_3d) ? 1 : 0), w, item_h,
			 fg, bg, 0, do_3d, menu[sel].text);
	sel = i;
	if (menu[sel].text[0] != '\0')
	  draw_menu_item(menu_win, (do_3d) ? 1 : 0,
			 sel * item_h + ((do_3d) ? 1 : 0), w, item_h,
			 fg, bg, 1, do_3d, menu[sel].text);
      } else if (i < 0 && sel >= 0) {
	draw_menu_item(menu_win, (do_3d) ? 1 : 0,
		       sel * item_h + ((do_3d) ? 1 : 0), w, item_h,
		       fg, bg, 0, do_3d, menu[sel].text);      
	sel = -1;
      }
    }
    win_ungrab_pointer(menu_win);

    /* Check if an item was selected */
    sel = what_menuitem(menu_win, menu, item_h, do_3d);
    if (sel >= 0 && menu[sel].child != NULL) {
      int x, y;

      *pmenu = menu[sel].child;
      sel = do_menu_x(menu_win, pmenu, pos_x + w - 2, pos_y + sel * item_h,
		      fg, bg, do_3d);
      if (sel >= 0)
	break;

      *pmenu = menu;
      x = menu_win->mouse_x;
      y = menu_win->mouse_y;
      mouse_b = win_read_mouse(menu_win, NULL, NULL);
      menu_win->mouse_x = pos_x - x;
      menu_win->mouse_y = pos_y - y;
    } else
      break;
  }

  /* For dialog menus, we set this so the manager can see if another
   * menu was selected.
   */
  if (parent != NULL) {
    parent->mouse_x = menu_win->mouse_x + pos_x;
    parent->mouse_y = menu_win->mouse_y + pos_y;
  }
  close_window(menu_win);

  return sel;
}

/* The menu controller */
int menu_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  int t, i, win_x, win_y, w, sel;
  MENU *menu;

  menu = dlg->dp;

  switch (msg) {
  case MSG_INIT:
    if (dlg->flags & D_3D) {
      win_setborderwidth(win, 0);
      win_setbackground(win, xd_back_3d);
    }
    if (dlg->flags & D_AUTOH) {
      dlg->h = win_textheight(win, dlg->dp) + 8;
      set_dialog_position(dlg);
    }
  case MSG_RESIZE:
    if (dlg->flags & D_AUTOW) {
      dlg->w = win->parent->w - (2 * dlg->x + ((dlg->flags & D_3D) ? 0 : 2));
      set_dialog_position(dlg);
      if (msg == MSG_RESIZE)
	SEND_MESSAGE(MSG_DRAW, dlg, 0);
    }
    return D_O_K;

  case MSG_DRAW:
    /* Only draw the menu bar */
    for (w = i = 0; menu[i].text != NULL; i++)
      w += win_textwidth(win, menu[i].text) + 10;
    w = MAX(w, dlg->w);

    if (c == 0) {
      win_setcolor(win, (dlg->flags & D_3D) ? xd_back_3d : dlg->bg);
      if (dlg->flags & D_3D)
	win_rectfill(win, 1, 1, w - 2, dlg->h - 2);
      else
	win_rectfill(win, 0, 0, w - 1, dlg->h - 1);
    }
    win_setcolor(win, dlg->fg);
    win_setbackcolor(win, (dlg->flags & D_3D) ? xd_back_3d : dlg->bg);

    for (w = i = 0; menu[i].text != NULL; i++) {
      t = win_textwidth(win, menu[i].text);
      win_outtextxy(win, w + 5, (dlg->h - win_textheight(win, dlg->dp)) / 2,
		    menu[i].text);
      w += t + 10;
    }
    if (dlg->flags & D_3D) {
      win_setcolor(win, xd_light_3d);
      win_moveto(win, 0, dlg->h - 1);
      win_lineto(win, 0, 0);
      win_lineto(win, dlg->w - 1, 0);

      win_setcolor(win, xd_shadow_3d);
      win_moveto(win, 0, dlg->h - 1);
      win_lineto(win, dlg->w - 1, dlg->h - 1);
      win_lineto(win, dlg->w - 1, 0);
    }
    return D_O_K;

  case MSG_CLICK:
    while (1) {
      /* Check if we have a good item in the clicked position */
      sel = what_submenu(win, dlg);
      if (sel < 0)
	return D_O_K;
      
      /* Calculate the position of the menu in the window */
      menu = dlg->dp;
      win_x = 5 + win->parent->x + dlg->x;
      for (i = 0; i < sel; i++)
	win_x += win_textwidth(win, menu[i].text) + 10;
      win_y = win->parent->y + dlg->y + dlg->h - 2;
      menu = menu[sel].child;
      if (menu == NULL)
	return D_O_K;
      
      /* Do the menu */
      sel = do_menu_x(win, &menu, win_x, win_y, dlg->fg, dlg->bg,
		      (dlg->flags & D_3D) ? 1 : 0);
      
      /* If we have a selection, let's call its handler */
      if (sel >= 0 && menu[sel].proc != NULL)
	return menu[sel].proc(win, menu[sel].id);

      win->mouse_x -= win->parent->x + win->x;
      win->mouse_y -= win->parent->y + win->y;
    }
    return D_O_K;

  }
  return D_O_K;
}


/*****************/
/*** NEW *********/

static void raised_rect(WINDOW *w, int x1, int y1, int x2, int y2)
{
  win_setcolor(w, xd_light_3d);
  win_moveto(w, x1, y2);
  win_lineto(w, x1, y1);
  win_lineto(w, x2, y1);
  win_setcolor(w, xd_shadow_3d);
  win_moveto(w, x1, y2);
  win_lineto(w, x2, y2);
  win_lineto(w, x2, y1);
}

static void hscroll_do_mouse(WINDOW *win, DIALOG *dlg)
{
  int delay, new_pos;
  void (*notify)(DIALOG *);

  win_read_mouse(win, NULL, NULL);
  notify = dlg->dp;
  if (point_in_rect(win->mouse_x, win->mouse_y, 0, 0, dlg->h, dlg->h)) {
    if (dlg->d1 > 0) {
      dlg->d1 -= dlg->d3;
      if (dlg->d1 < 0)
	dlg->d1 = 0;
      SEND_MESSAGE(MSG_DRAW, dlg, 1);
      if (notify != NULL)
	notify(dlg);
    }
    delay = -dlg->d4;
    while (win->mouse_b) {
      win_read_mouse(win, NULL, NULL);
      if (point_in_rect(win->mouse_x, win->mouse_y, 0, 0, dlg->h, dlg->h)
	  && dlg->d1 > 0 && delay++ >= dlg->d5) {
	delay = 0;
	dlg->d1 -= dlg->d3;
	if (dlg->d1 < 0)
	  dlg->d1 = 0;
	SEND_MESSAGE(MSG_DRAW, dlg, 1);
	if (notify != NULL)
	  notify(dlg);
      }
    }
  } else if (point_in_rect(win->mouse_x, win->mouse_y,
			   dlg->w - dlg->h, 0, dlg->w, dlg->h)) {
    if (dlg->d1 < dlg->d2 - 1) {
      dlg->d1 += dlg->d3;
      if (dlg->d1 >= dlg->d2)
	dlg->d1 = dlg->d2 - 1;
      SEND_MESSAGE(MSG_DRAW, dlg, 1);
      if (notify != NULL)
	notify(dlg);
    }
    delay = -dlg->d4;
    while (win->mouse_b) {
      win_read_mouse(win, NULL, NULL);
      if (point_in_rect(win->mouse_x, win->mouse_y,
			dlg->w - dlg->h, 0, dlg->w, dlg->h)
	  && dlg->d1 < dlg->d2 - 1 && delay++ >= dlg->d5) {
	delay = 0;
	dlg->d1 += dlg->d3;
	if (dlg->d1 >= dlg->d2)
	  dlg->d1 = dlg->d2 - 1;
	SEND_MESSAGE(MSG_DRAW, dlg, 1);
	if (notify != NULL)
	  notify(dlg);
      }
    }
  } else if (point_in_rect(win->mouse_x, win->mouse_y,
			   dlg->h, 0, dlg->w - dlg->h, dlg->h)) {
    while (win->mouse_b) {
      win_read_mouse(win, NULL, NULL);

      new_pos = (((dlg->d2 - 1) *
		  (win->mouse_x - dlg->h - (dlg->h - 4) / 2 - 2))
		 / (dlg->w - (3 * dlg->h + 2)));
      if (new_pos < 0)
	new_pos = 0;
      if (new_pos >= dlg->d2)
	new_pos = dlg->d2 - 1;
      if (new_pos != dlg->d1) {
	dlg->d1 = new_pos;
	SEND_MESSAGE(MSG_DRAW, dlg, 1);
	if (notify != NULL)
	  notify(dlg);
      }
    }
  }
}

/* Horizontal scroll
 * d1 = current position (0 -> dlg->d2-1)
 * d2 = number of divisions
 * d3 = scroll increment
 * d4 = scroll delay
 * d5 = scroll repeat
 */
int hscroll_proc(int msg, WINDOW *w, DIALOG *dlg, int c)
{
  int pos;

  switch (msg) {
  case MSG_INIT:
    {
      void (*notify)(DIALOG *);

      notify = dlg->dp;
      if (dlg->d3 <= 0)
	dlg->d3 = 1;       /* Increment */
      if (dlg->d4 <= 0)
	dlg->d4 = SCROLL_DELAY;
      if (dlg->d5 <= 0)
	dlg->d5 = SCROLL_REPEAT;
      if (dlg->d1 < 0)
	dlg->d1 = 0;
      if (dlg->d1 >= dlg->d2)
	dlg->d1 = dlg->d2 - 1;
      if (notify != NULL)
	notify(dlg);

      win_setborderwidth(w, 0);
      if (dlg->flags & D_3D)
	win_setbackground(w, xd_back_3d);

      return D_O_K;
    }

  case MSG_DRAW:
    if (dlg->flags & D_DISABLED)
      return D_O_K;

    if (c == 0) {
      win_setcolor(w, xd_back_3d);
      win_rectfill(w, 0, 0, dlg->w - 1, dlg->h - 1);
    }

    if (dlg->flags & D_3D) {
      win_setcolor(w, xd_shadow_3d);
      win_moveto(w, dlg->h + 1, dlg->h - 1);
      win_lineto(w, dlg->h + 1, 0);
      win_lineto(w, dlg->w - dlg->h - 2, 0);
      win_setcolor(w, xd_light_3d);
      win_moveto(w, dlg->h + 1, dlg->h - 1);
      win_lineto(w, dlg->w - dlg->h - 2, dlg->h - 1);
      win_lineto(w, dlg->w - dlg->h - 2, 0);
    }

    /* Left */
    win_setcolor(w, xd_shadow_3d);
    win_line(w, 0, dlg->h / 2, dlg->h - 1, dlg->h - 1);
    win_line(w, dlg->h - 1, dlg->h - 1, dlg->h - 1, 0);
    win_line(w, 0, dlg->h / 2, dlg->h - 1, 0);

    /* Right */
    win_line(w, dlg->w - 1, dlg->h / 2, dlg->w - dlg->h, dlg->h - 1);
    win_line(w, dlg->w - dlg->h, dlg->h - 1, dlg->w - dlg->h, 0);
    win_line(w, dlg->w - dlg->h, 0, dlg->w - 1, dlg->h / 2);

    win_setcolor(w, xd_back_3d);
    win_rectfill(w, dlg->h + 2, 1, dlg->w - dlg->h - 3, dlg->h - 2);

    /* Handle */
    if (dlg->d2 > 1) {
      pos = (dlg->d1 * (dlg->w - (3 * dlg->h + 2))) / (dlg->d2 - 1);
      raised_rect(w, pos + dlg->h + 3, 2, pos + 2 * dlg->h - 2, dlg->h - 3);
    }
    return D_O_K;

  case MSG_CLICK:
    hscroll_do_mouse(w, dlg);
    return D_O_K;

  default:
    return D_O_K;
  }
}


static void vscroll_do_mouse(WINDOW *win, DIALOG *dlg)
{
  int delay, new_pos;
  void (*notify)(DIALOG *);

  win_read_mouse(win, NULL, NULL);
  notify = dlg->dp;
  if (point_in_rect(win->mouse_x, win->mouse_y, 0, 0, dlg->w, dlg->w)) {
    if (dlg->d1 > 0) {
      dlg->d1 -= dlg->d3;
      if (dlg->d1 < 0)
	dlg->d1 = 0;
      SEND_MESSAGE(MSG_DRAW, dlg, 1);
      if (notify != NULL)
	notify(dlg);
    }
    delay = -dlg->d4;
    while (win->mouse_b) {
      win_read_mouse(win, NULL, NULL);
      if (point_in_rect(win->mouse_x, win->mouse_y, 0, 0, dlg->w, dlg->w)
	  && dlg->d1 > 0 && delay++ >= dlg->d5) {
	delay = 0;
	dlg->d1 -= dlg->d3;
	if (dlg->d1 < 0)
	  dlg->d1 = 0;
	SEND_MESSAGE(MSG_DRAW, dlg, 1);
	if (notify != NULL)
	  notify(dlg);
      }
    }
  } else if (point_in_rect(win->mouse_x, win->mouse_y, 0, dlg->h - dlg->w,
			   dlg->w, dlg->h)) {
    if (dlg->d1 < dlg->d2 - 1) {
      dlg->d1 += dlg->d3;
      if (dlg->d1 >= dlg->d2)
	dlg->d1 = dlg->d2 - 1;
      SEND_MESSAGE(MSG_DRAW, dlg, 1);
      if (notify != NULL)
	notify(dlg);
    }
    delay = -dlg->d4;
    while (win->mouse_b) {
      win_read_mouse(win, NULL, NULL);
      if (point_in_rect(win->mouse_x, win->mouse_y, 0, dlg->h - dlg->w,
			dlg->w, dlg->h)
	  && dlg->d1 < dlg->d2 - 1 && delay++ >= dlg->d5) {
	delay = 0;
	dlg->d1 += dlg->d3;
	if (dlg->d1 >= dlg->d2)
	  dlg->d1 = dlg->d2 - 1;
	SEND_MESSAGE(MSG_DRAW, dlg, 1);
	if (notify != NULL)
	  notify(dlg);
      }
    }
  } else if (point_in_rect(win->mouse_x, win->mouse_y, 0, dlg->w,
			   dlg->w, dlg->h - dlg->w)) {
    while (win->mouse_b) {
      win_read_mouse(win, NULL, NULL);

      new_pos = (((dlg->d2 - 1) *
		  (win->mouse_y - dlg->w - (dlg->w - 4) / 2 - 2))
		 / (dlg->h - (3 * dlg->w + 2)));
      if (new_pos < 0)
	new_pos = 0;
      if (new_pos >= dlg->d2)
	new_pos = dlg->d2 - 1;
      if (new_pos != dlg->d1) {
	dlg->d1 = new_pos;
	SEND_MESSAGE(MSG_DRAW, dlg, 1);
	if (notify != NULL)
	  notify(dlg);
      }
    }
  }
}

/* Vertical scroll
 * d1 = current position (0 -> dlg->d2-1)
 * d2 = number of divisions
 * d3 = scroll increment
 */
int vscroll_proc(int msg, WINDOW *w, DIALOG *dlg, int c)
{
  int pos;

  switch (msg) {
  case MSG_INIT:
    {
      void (*notify)(DIALOG *);

      notify = dlg->dp;
      if (dlg->d3 <= 0)
	dlg->d3 = 1;       /* Increment */
      if (dlg->d4 <= 0)
	dlg->d4 = SCROLL_DELAY;
      if (dlg->d5 <= 0)
	dlg->d5 = SCROLL_REPEAT;
      if (dlg->d1 < 0)
	dlg->d1 = 0;
      if (dlg->d1 >= dlg->d2)
	dlg->d1 = dlg->d2 - 1;
      if (notify != NULL)
	notify(dlg);

      win_setborderwidth(w, 0);
      if (dlg->flags & D_3D)
	win_setbackground(w, xd_back_3d);

      return D_O_K;
    }

  case MSG_DRAW:
    if (dlg->flags & D_DISABLED)
      return D_O_K;
    if (dlg->flags & D_3D) {
      win_setcolor(w, xd_shadow_3d);
      win_moveto(w, 0, dlg->h - dlg->w - 1);
      win_lineto(w, 0, dlg->w);
      win_lineto(w, dlg->w - 1, dlg->w);
      win_setcolor(w, xd_light_3d);
      win_moveto(w, 0, dlg->h - dlg->w - 1);
      win_lineto(w, dlg->w - 1, dlg->h - dlg->w - 1);
      win_lineto(w, dlg->w - 1, dlg->w);
    }

    /* Up */
    win_setcolor(w, xd_shadow_3d);
    win_line(w, dlg->w / 2, 0, dlg->w - 1, dlg->w - 2);
    win_line(w, dlg->w - 1, dlg->w - 2, 0, dlg->w - 2);
    win_line(w, 0, dlg->w - 2, dlg->w / 2, 0);

    /* Down */
    win_line(w, dlg->w / 2, dlg->h - 1, dlg->w - 1, dlg->h - dlg->w + 1);
    win_line(w, dlg->w - 1, dlg->h - dlg->w + 1, 0, dlg->h - dlg->w + 1);
    win_line(w, 0, dlg->h - dlg->w + 1, dlg->w / 2, dlg->h - 1);

    win_setcolor(w, xd_back_3d);
    win_rectfill(w, 1, dlg->w + 2, dlg->w - 2, dlg->h - dlg->w - 3);

    /* Handle */
    if (dlg->d2 > 1) {
      pos = (dlg->d1 * (dlg->h - (3 * dlg->w + 2))) / (dlg->d2 - 1);
      raised_rect(w, 2, pos + dlg->w + 3, dlg->w - 3, pos + 2 * dlg->w - 2);
    }
    return D_O_K;

  case MSG_CLICK:
    vscroll_do_mouse(w, dlg);
    return D_O_K;

  default:
    return D_O_K;
  }
}
