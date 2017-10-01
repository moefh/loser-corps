/* bgiclone.c
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
#include <unistd.h>

#define BGI_FUNCS

#include "xdialogs.h"
#include "internal.h"

#define aw active_window

char *xd_bgi_window_title = NULL;


/* Initialize the graphics system with the default display, ignoring
 * all parameters.
 */
int initgraph(int *driver, int *mode, char *path)
{
  char *tit;

  if (init_graph(NULL))
    return -1;

  if (xd_bgi_window_title == NULL)
    tit = "";
  else
    tit = xd_bgi_window_title;

  if (create_window(tit, 100, 40, 640, 480, NULL, NULL) == NULL) {
    aw = NULL;
    closegraph();
    return -1;
  }
  setbackground(0);
  setcolor(15);
  setbackcolor(0);
  createimage();
  showwindow();
  return 0;
}

int graphresult(void) { return (aw == NULL) ? -1 : grOk; }
char *grapherrormsg(int a) { return "Can't open display"; }

void delay(int msec)
{
  if (aw != NULL)
    XFlush(aw->disp);
  usleep(msec*1000);
}

int getmaxx(void) { return win_getmaxx(aw); }
int getmaxy(void) { return win_getmaxy(aw); }
int getcolor(void) { return win_getcolor(aw); }

void clearviewport(void)
{
  int c;

  c = getcolor();
  setcolor(0);
  bar(0, 0, getmaxx(), getmaxy());
  setcolor(c);
}

void setwritemode(int m)
{
  if (m != 0)
    win_setdrawmode(aw, DM_XOR);
  else
    win_setdrawmode(aw, DM_COPY);
}

void setcolor(int n) { win_setcolor(aw, n); }
void moveto(int x, int y) { win_moveto(aw, x, y); }
void lineto(int x, int y) { win_lineto(aw, x, y); }
void line(int x1, int y1, int x2, int y2) { win_line(aw, x1, y1, x2, y2); } 
void putpixel(int x, int y, int c) { win_putpixel(aw, x, y, c); }
void bar(int x1, int y1, int x2, int y2) { win_rectfill(aw, x1, y1, x2, y2); }

void outtext(char *t)
{
  int a;

  a = win_getdrawmode(aw);
  if (a != DM_COPY)
    win_setdrawmode(aw, DM_COPY);

  win_outtext(aw, t);

  if (a != DM_COPY)
    win_setdrawmode(aw, a);
}

void outtextxy(int x, int y, char *t)
{
  int a;

  a = win_getdrawmode(aw);
  if (a != DM_COPY)
    win_setdrawmode(aw, DM_COPY);

  win_outtextxy(aw, x, y, t);

  if (a != DM_COPY)
    win_setdrawmode(aw, a);
}

void setlinestyle(int a, int b, int c) { }


/* These aren't really BGI, but you might find them useful... */

int kbhit(void)
{
  return win_has_key(aw);
}

int getch(void)
{
  int c;

  c = win_read_key(aw) & 0xffff;
  if ((c & 0xff00) != 0 && (c & 0xff00) != 0xff00)
    return 0;
  return (c & 0x00ff);
}
