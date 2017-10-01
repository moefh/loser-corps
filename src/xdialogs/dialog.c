/* dialog.c - Dialog manager functions.
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
#include <malloc.h>
#include "xdialogs.h"
#include "internal.h"

/* Send a message to all controls in a dialog */
void broadcast_msg(int msg, WINDOW *win, int c)
{
  int i;

  if (win->dialog == NULL)
    return;

  for (i = 0; win->dialog[i].proc != NULL; i++)
    SEND_MESSAGE(msg, win->dialog + i, c);
}

/* Returns 1 if (x,y) is in the rectangle (x1,y1),(x2,y1) */
int point_in_rect(int x, int y, int x1, int y1, int x2, int y2)
{
  return (x >= x1 && x < x2 && y >= y1 && y < y2);
}

/* Return the minimum rectangle containing the given ones */
void rect_union(int ax1, int ay1, int ax2, int ay2,
	       int bx1, int by1, int bx2, int by2,
	       int *rx1, int *ry1, int *rx2, int *ry2)
{
  int cx1, cy1, cx2, cy2;

  cx1 = MIN(ax1, bx1);
  cy1 = MIN(ay1, by1);
  cx2 = MIN(ax2, bx2);
  cy2 = MIN(ay2, by2);

  if (rx1 != NULL) *rx1 = cx1;
  if (ry1 != NULL) *ry1 = cy1;
  if (rx2 != NULL) *rx2 = cx2;
  if (ry2 != NULL) *ry2 = cy2;  
}

/* Returns 1 if the rectangle [ax1,ay1]-[ax2,ay2] intersects
 * the rectangle [bx1,by1]-[bx2,by2]. If it intersects, puts the
 * intersection in [*rx1,*ry1]-[*rx2,*ry2] (provided that the pointers
 * are non-null).
 */
int rect_intersection(int ax1, int ay1, int ax2, int ay2,
		      int bx1, int by1, int bx2, int by2,
		      int *rx1, int *ry1, int *rx2, int *ry2)
{
  int cx1, cy1, cx2, cy2;

  cx1 = MAX(ax1, bx1);
  cy1 = MAX(ay1, by1);
  cx2 = MIN(ax2, bx2);
  cy2 = MIN(ay2, by2);

  if (cx1 < cx2 && cy1 < cy2) {
    if (rx1 != NULL) *rx1 = cx1;
    if (ry1 != NULL) *ry1 = cy1;
    if (rx2 != NULL) *rx2 = cx2;
    if (ry2 != NULL) *ry2 = cy2;
    return 1;
  }
  return 0;
}

/* Returns the index of the control that holds
 * the input focus of the specifyed dialog.
 */
int find_dialog_focus(DIALOG *d)
{
  int i;

  for (i = 0; d[i].proc != NULL; i++)
    if (d[i].flags & D_GOTFOCUS)
      return i;

  return -1;
}

/* Set or clear the D_3D flag to all controls of the dialog */
void set_dialog_flag(DIALOG *dialog, unsigned int flag, int set)
{
  int i;

  for (i = 0; dialog[i].proc != NULL; i++)
    if (set)
      dialog[i].flags |= flag;
    else
      dialog[i].flags &= ~flag;
}

/* Set the color of all controls in a dialog */
void set_dialog_color(DIALOG *dlg, int fg, int bg)
{
  int i;

  for (i = 0; dlg[i].proc != NULL; i++) {
    dlg[i].fg = fg;
    dlg[i].bg = bg;
  }
}

/* Set the position of the dialog control window according to the DIALOG */
void set_dialog_position(DIALOG *dlg)
{
  XWindowChanges wc;

  wc.x = dlg->x;
  wc.y = dlg->y;
  wc.width = (dlg->w > 0) ? dlg->w : 1;
  wc.height = (dlg->h > 0) ? dlg->h : 1;
  dlg->win->x = wc.x;
  dlg->win->y = wc.y;
  dlg->win->w = wc.width;
  dlg->win->h = wc.height;
  XConfigureWindow(dlg->win->disp, dlg->win->window,
		   CWX|CWY|CWWidth|CWHeight, &wc);
}

/* Center the dialog in the screen */
void centre_dialog(DIALOG *dialog, WINDOW *rel)
{
  if (dialog == NULL)
    return;

  if (rel == NULL) {
    int w, h;

    w = WidthOfScreen(DefaultScreenOfDisplay(xd_display));
    h = HeightOfScreen(DefaultScreenOfDisplay(xd_display));
    dialog[0].x = (w - dialog[0].w) / 2;
    dialog[0].y = (h - dialog[0].h) / 2;
  } else {
    dialog[0].x = rel->x + (rel->w - dialog[0].w) / 2;
    dialog[0].y = rel->y + (rel->h - dialog[0].h) / 2;
  }
}

/* Initialize the dialog and set the focused control to `focus' */
void init_dialog(WINDOW *w, int stop, int focus)
{ 
  if (w->dialog == NULL)
    return;

  /* Set the dialog focus */
  if (focus < 0) {
    focus = find_dialog_focus(w->dialog);
    if (focus < 0) {
      focus = 0;
      w->dialog[focus].flags |= D_GOTFOCUS;
    }
  }

  /* Disable the parent dialog, if any */
  w->dialog->d3 = stop;
  if (stop && w->parent != NULL && w->parent->dialog != NULL)
    w->parent->dialog->d2++;

  /* Initialize and draw the dialog */
  w->dialog->flags &= ~D_CLOSE;
  w->dialog->d1 = focus;
  w->dialog->d5 = 1;
  broadcast_msg(MSG_INIT, w, 0);
  broadcast_msg(MSG_RESIZE, w, 0);
  win_showwindow(w);
}

/* Do all notifications */
static void do_notify(NOTIFY_WIN *list)
{
  NOTIFY_WIN *w;

  for (w = list; w != NULL; w = w->next)
    if (w->active && w->func != NULL)
      w->func(w->w);
}

/* Do a dialog step. This function returns 0 if the dialog has been
 * terminated, 1 if it should continue.
 */
int do_dialog_step(WINDOW *w)
{
  if (w->dialog == NULL || (w->dialog->flags & D_CLOSE) != 0)
    return 0;

  kill_time();
  do_notify(notify_list);

  if (win_has_key(w))     /* Allow processing of events */
    win_read_key(w);
  else
    broadcast_msg(MSG_IDLE, w, 0);

  if (w->dialog->flags & D_CLOSE)
    return 0;
  return 1;
}

/* Terminate a dialog */
int end_dialog(WINDOW *w)
{
  if (w->dialog == NULL)
    return 0;

  /* End the dialog and close the window */
  broadcast_msg(MSG_END, w, 0);
  win_hidewindow(w);
  w->dialog->d5 = 0;

  /* Re-enable the parent dialog, if any */
  if (w->dialog->d3 != 0 && w->parent != NULL && w->parent->dialog != NULL)
    w->parent->dialog->d2--;

  return w->dialog->d1;
}

/* Execute the dialog of the window pointed to by w */
int do_dialog(WINDOW *w, int stop, int focus)
{
  init_dialog(w, stop, focus);

  while (do_dialog_step(w))
    ;

  return end_dialog(w);
}

/* Creates a sub-window of the `parent' window and executes the dialog
 * `dlg' on it */
int do_dialog_window(WINDOW *parent, DIALOG *dlg, int stop)
{
  WINDOW *w;
  int ret;

  w = create_window(NULL, 0, 0, 0, 0, parent, dlg);
  if (w == NULL)
    return -1;
  ret = do_dialog(w, stop, -1);
  close_window(w);
  return ret;
}


int add_notify(void (*func)(WINDOW *), WINDOW *win)
{
  NOTIFY_WIN *w;

  if ((w = (NOTIFY_WIN *) malloc(sizeof(NOTIFY_WIN))) == NULL)
    return 1;
  w->func = func;
  w->w = win;
  w->active = 1;
  w->next = notify_list;
  notify_list = w;
  return 0;
}

int remove_notify(void (*func)(WINDOW *), WINDOW *win)
{
  NOTIFY_WIN *w, *prev = NULL;

  for (w = notify_list; w != NULL; prev = w, w = w->next)
    if (w->func == func && w->w == win) {
      if (prev == NULL)
	notify_list = w->next;
      else
	prev->next = w->next;
      free(w);
      return 0;
    }
  return 1;
}

int set_notify(void (*func)(WINDOW *), WINDOW *win, int activate)
{
  NOTIFY_WIN *w;

  for (w = notify_list; w != NULL; w = w->next)
    if (w->func == func && w->w == win) {
      w->active = activate;
      return 0;
    }
  return 1;
}
