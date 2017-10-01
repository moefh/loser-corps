/* xdialogs.h
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
 * XDialogs header file. This file should be included in any source that
 * uses any XDialogs functions.
 *
 * This library is inspired in (i.e., stolen from :-)) the Allegro (a game
 * library for MS-DOS compiled with DJGPP, a free 32-bit compiler for DOS)
 * GUI functions. If you don't know Allegro (by Shawn Hargreaves), look for
 * it in www.demon.talula.co.uk.
 *
 * You don't have to know anything about Allegro to use this library
 * (e.g., you "don't do DOS"), look the file README.txt information.
 *
 * Although this code has little or nothing from the Allegro library, the
 * concepts and data structures are very close. This was done in part for
 * making easier for who is familiarized with Allegro to start writing X
 * applications and in part because the concepts are good (this is my
 * opinion, of course).
 *
 * This library is not finished, but it is already very useful. There are
 * many differences from the Allegro GUI; code written to Allegro may need
 * adjustments to compile and run here.
 *
 * Ricardo R. Massaro (massaro@ime.usp.br)
 */

#ifndef XDIALOGS_H
#define XDIALOGS_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#define MAX_KB_BUF  32   /* Size of the keyboard buffer */

#define SCROLL_DELAY   500
#define SCROLL_REPEAT  100

#ifdef __cplusplus
extern "C" {
#endif 

/* Number of colors in the palette */
#define PAL_SIZE     256

#define COLOR(i)   (((i) < 0 || (i) >= PAL_SIZE) ? xd_pal[0].pixel : \
                    xd_pal[i].pixel)

/* Pixel values for the colors in X */
typedef struct PALETTE {
  int pixel;                /* Pixel value (set in init_graph()) */
  int allocated;            /* 1 if allocated */
  unsigned int val[3];      /* Components (0..65536) */
} PALETTE;

extern Display *xd_display;
extern PALETTE xd_pal[];
extern char xd_version[];

enum {   /* Colors */
  COLOR_BLACK,
  COLOR_BLUE1,
  COLOR_GREEN1,
  COLOR_CYAN1,
  COLOR_RED1,
  COLOR_MAGENTA1,
  COLOR_YELLOW1,
  COLOR_GRAY,
  COLOR_GRAY1,
  COLOR_BLUE,
  COLOR_GREEN,
  COLOR_CYAN,
  COLOR_RED,
  COLOR_MAGENTA,
  COLOR_YELLOW,
  COLOR_WHITE,
};

enum {   /* Control types */
  CTL_NONE = -2,
  CTL_USER = -1,     /* User defined */
  CTL_WINDOW = 0,    /* Window */
  CTL_MENU,          /* Menu */
  CTL_BUTTON,        /* Button */
  CTL_EDIT,          /* Edit control */
  CTL_TEXT,          /* Static text */
  CTL_LIST,          /* List box */
  CTL_COMBO,         /* Combo box */
  CTL_CHECK,         /* Check box */
  CTL_HSCROLL,       /* Horizontal scroll box */
  CTL_VSCROLL,       /* Vertical scroll box */
};

enum {   /* Messages */
  MSG_INIT,          /* Initializing object */
  MSG_END,           /* Terminating the object */
  MSG_IDLE,          /* Sent when there is nothing to do */
  MSG_DRAW,          /* Drawing the object */
  MSG_KEY,           /* A key was pressed */
  MSG_CLICK,         /* The mouse button was pressed */
  MSG_DCLICK,        /* The mouse button was double-clicked */
  MSG_WANTFOCUS,     /* Querying if the control wants the focus */
  MSG_GOTFOCUS,      /* The control has gained the input focus */
  MSG_LOSTFOCUS,     /* The control has lost the input focus */
  MSG_GOTMOUSE,      /* The mouse is going into the control */
  MSG_LOSTMOUSE,     /* The mouse is leaving the control */
  MSG_MOUSEMOVE,     /* The mouse was moved */
  MSG_RESIZE,        /* The window size has changed */
  MSG_GETDATA,       /* Get the text from an edit cotrol to dlg->dp */
  MSG_SETDATA,       /* Set the text of an edit control from dlg->dp */
  MSG_USER = 100,    /* Reserved for users */
};

#define SEND_MESSAGE(msg, dlg, c)   (dlg)->proc(msg, (dlg)->win, dlg, c)
#define KEYSYM(key)                 (((unsigned) (key)) & 0x0000ffff)
#define KEYMOD(key)                 (((unsigned) (key)) >> 16)

/* Dialog procedure return values */
#define D_O_K        0x0000    /* OK: Nothing happened */
#define D_REDRAW     0x0001    /* Must redraw the entire dialog */
#define D_CLOSE      0x0002    /* Must close the dialog */
#define D_WANTFOCUS  0x0004    /* Yes, the control wants the input focus */
#define D_USED_CHAR  0x0008    /* The control used the key */

/* Control flags */
#define D_EXIT           0x00100   /* Should return D_CLOSE when clicked */
#define D_GOTFOCUS       0x00200   /* Control has the input focus */
#define D_GOTMOUSE       0x00400   /* Mouse is over the control's rectangle */
#define D_SELECTED       0x00800   /* Control is selected */
#define D_AUTOW          0x01000   /* Control adjusts the width */
#define D_AUTOH          0x02000   /* Control adjusts the height */
#define D_DISABLED       0x04000   /* Control is disabled */
#define D_3D             0x08000   /* Control has 3D aspect */
#define D_NORESIZE       0x10000   /* Window can't be resized */
#define D_WANTMOUSEMOVE  0x20000   /* Want MSG_MOUSEMOVE messages */


/* Drawing modes */
#define DM_CLEAR            GXclear
#define DM_AND              GXand
#define DM_AND_REVERSE      GXandReverse
#define DM_COPY             GXcopy
#define DM_AND_INVERTED     GXandInverted
#define DM_NOOP             GXnoop
#define DM_XOR              GXxor
#define DM_OR               GXor
#define DM_NOR              GXnor
#define DM_EQUIV            GXequiv
#define DM_INVERT           GXinvert
#define DM_OR_REVERSE       GXorReverse
#define DM_COPY_INVERTED    GXcopyInverted
#define DM_OR_INVERTED      GXorInverted
#define DM_NAND             GXnand
#define DM_SET              GXset

typedef struct WINDOW WINDOW;
typedef struct DIALOG DIALOG;
typedef struct NOTIFY_WIN NOTIFY_WIN;

typedef struct FONT {
  int font_x, font_y;          /* Font origin */
  XFontStruct *font_struct;    /* X font structure */
  int num_refs;                /* # of references to this font */
} FONT;

typedef struct MENU {
  char *text;                  /* "" for separator */
  int (*proc)(WINDOW *, int);  /* Function called when the menu is clicked */
  struct MENU *child;          /* Child menu */
  int id;                      /* ID to pass to function */
} MENU;

struct NOTIFY_WIN {
  int active;
  void (*func)(WINDOW *win);
  WINDOW *w;
  NOTIFY_WIN *next;
};

struct DIALOG {
  int type;                            /* Type of the control */
  int (*proc)(int, WINDOW *, DIALOG *, int);    /* Procedure */
  int x, y;                            /* Position */
  int w, h;                            /* Size */
  int fg, bg;                          /* Colors (fore/background) */
  int flags;                           /* Flags */
  int d1, d2;                          /* Data */
  void *dp, *dp1;                      /* Data pointers */
  int d3, d4, d5;                      /* More data */
  int draw_x, draw_y, draw_w, draw_h;  /* Exposed rectangle */
  WINDOW *win;                         /* Control window */
};

struct WINDOW {
  Display *disp;                     /* X Display */
  Window window;                     /* Window */
  int depth;                         /* Screen depth */
  GC gc;                             /* Window GC */
  Pixmap image;                      /* Pixmap */
  unsigned long last_click_usec;     /* Last click time */
  unsigned long last_click_sec;      /* Last click time */

  int x, y, w, h;                    /* Window position and dimensions */
  int pkb_i, pkb_f;                  /* Begin/end of keyboard buffer */
  unsigned long kb_buf[MAX_KB_BUF];  /* Keyboard buffer */
  int mouse_x, mouse_y, mouse_b;     /* Mouse status */
  int selected;                      /* 1 of the mouse is in the window */
  int pointer_grabbed;               /* 1 if the pointer is grabbed */
  int is_child;                      /* 1 if it is a X child */
  int px, py;                        /* Position of the drawing point */
  int color, bkcolor;                /* Current drawing colors */
  int draw_mode;                     /* Current drawing mode */

  FONT *font;
  DIALOG *dialog;
  WINDOW *parent, *child, *sibling;
};

extern WINDOW *active_window;
extern int xd_no_sent_events;
extern NOTIFY_WIN *notify_list;

extern char xd_def_window_name[];
extern char xd_def_window_class[];
extern char *xd_window_name;
extern char *xd_window_class;

extern Cursor xd_arrow_cursor, xd_text_cursor;
extern FONT *xd_default_font, *xd_fixed_font;
extern char xd_font_name[];
extern int xd_install_colormap;
extern int gui_fg_color, gui_bg_color, gui_mg_color;
extern int xd_stddlg_3d;
extern int xd_light_3d, xd_shadow_3d, xd_back_3d;


/************************************************************/
/* Window functions *****************************************/

WINDOW  *search_window_handle(Window w);
int     init_graph(char *display);
void    closegraph(void);
void    xd_install_palette(void);
void    xd_set_palette(unsigned char *pal, int from, int to);
int     screen_width(void);
int     screen_height(void);
void    win_set_selection(WINDOW *w, char *sel, int n);
int     win_get_selection(WINDOW *w, char *sel, int max);
WINDOW  *create_window(char *name, int x, int y, int w, int h,
		       WINDOW *parent, DIALOG *dlg);
WINDOW  *create_window_x(char *name, int x, int y, int w, int h,
			 int managed, WINDOW *parent, DIALOG *dlg);
void    set_active_window(WINDOW *win);
void    process_pending_events(void);
FONT    *create_font(char *font_name);
void    free_font(FONT *f);
void    kill_time(void);

void win_showwindow(WINDOW *win);
void win_hidewindow(WINDOW *win);
void win_movewindow(WINDOW *win, int x, int y);
void win_resizewindow(WINDOW *win, int w, int h);
void win_set_min_max(WINDOW *win, int min_w, int min_h, int max_w, int max_h);
void close_window(WINDOW *win);


int add_notify(void (*func)(WINDOW *win), WINDOW *w);
int remove_notify(void (*)(WINDOW *win), WINDOW *w);
int set_notify(void (*func)(WINDOW *win), WINDOW *w, int activate);

void broadcast_msg(int msg, WINDOW *w, int c);
void init_dialog(WINDOW *w, int stop, int focus);
int  do_dialog_step(WINDOW *w);
int  end_dialog(WINDOW *w);
int  do_dialog(WINDOW *w, int stop, int focus);
int  do_dialog_window(WINDOW *parent, DIALOG *dlg, int stop);
void set_dialog_flag(DIALOG *dialog, unsigned int flag, int set);
void set_dialog_color(DIALOG *dlg, int fb, int bg);
void set_dialog_position(DIALOG *dlg);
void centre_dialog(DIALOG *dlg, WINDOW *relative);
int  find_dialog_focus(DIALOG *dlg);
int  do_menu(WINDOW *parent, MENU *menu, int x, int y);
int  do_menu_x(WINDOW *parent, MENU **pmenu, int x, int y, int fg, int bg,
	       int do_3d);
int  file_select(WINDOW *parent, char *title, char *path, char *ext);
int  alert(WINDOW *parent, char *title, char *s1, char *s2, char *s3,
	   char *b1, char *b2);


/************************************************************/
/* Graphics functions ***************************************/

void win_createimage(WINDOW *w);
void win_destroyimage(WINDOW *w);
void win_outtext(WINDOW *w, char *txt);
void win_outtext_n(WINDOW *w, char *txt, int n);
void win_outtextxy(WINDOW *w, int x, int y, char *txt);
void win_outtextxy_n(WINDOW *w, int x, int y, char *txt, int n);
int  win_textwidth(WINDOW *w, char *txt);
int  win_textheight(WINDOW *w, char *txt);
int  win_textwidth_n(WINDOW *w, char *txt, int);
int  win_textheight_n(WINDOW *w, char *txt, int);
void win_moveto(WINDOW *w, int x, int y);
void win_lineto(WINDOW *w, int x, int y);
void win_putpixel(WINDOW *w, int x, int y, int cor);
void win_line(WINDOW *w, int x1, int y1, int x2, int y2);
void win_circle(WINDOW *w, int x, int y, int radius);
void win_rect(WINDOW *w, int x1, int y1, int x2, int y2);
void win_rectfill(WINDOW *w, int x1, int y1, int x2, int y2);
int  win_getcolor(WINDOW *w);
void win_setcolor(WINDOW *w, int color);
int  win_getdrawmode(WINDOW *w);
void win_setdrawmode(WINDOW *w, int mode);
int  win_getbackcolor(WINDOW *w);
void win_setbackcolor(WINDOW *w, int color);
void win_setbackground(WINDOW *w, int color);
void win_setbordercolor(WINDOW *w, int color);
void win_setborderwidth(WINDOW *w, int width);

#define showwindow()        win_showwindow(active_window)
#define hidewindow()        win_hidewindow(active_window)
#define createimage()       win_createimage(active_window)
#define destroyimage()      win_destroyimage(active_window)
#define setdrawmode(m)      win_setdrawmode(active_window, (m))
#define getbackcolor(c)     win_getbackcolor(active_window, (c))
#define setbackcolor(c)     win_setbackcolor(active_window, (c))
#define setbackground(c)    win_setbackground(active_window, (c))
#define setbordercolor(c)   win_setbordercolor(active_window, (c))
#define setborderwidth(w)   win_setborderwidth(active_window, (w))
#define rect(a,b,c,d)       win_rect(active_window,(a),(b),(c),(d))
#define rectfill(a,b,c,d)   win_rectfill(active_window,(a),(b),(c),(d))
#define textwidth(t)        win_textwidth(active_window,(t))
#define textheight(t)       win_textheight(active_window,(t))
#define textwidth_n(t,n)    win_textwidth(active_window,(t),(n))
#define textheight_n(t,n)   win_textheight(active_window,(t),(n))

#define win_getmaxx(win)    ((win)->w-1)
#define win_getmaxy(win)    ((win)->h-1)


/************************************************************/
/* BGI clone stuff ******************************************/

#ifdef BGI_FUNCS

/* Get the BGI functions defined in bgiclone.c */

int  graphresult(void);
char *grapherrormsg(int);
int  getcolor(void);
int  getmaxx(void);
int  getmaxy(void);
void setcolor(int);
void setwritemode(int);
void outtext(char *);
void outtextxy(int, int, char *);
void lineto(int, int);
void moveto(int, int);
void putpixel(int, int, int);
void line(int, int, int, int);
void bar(int, int, int, int);
void setlinestyle(int, int, int);
void setwritemode(int);
void clearviewport(void);

int kbhit(void);
int getch(void);
void delay(int);

#else /* BGI_FUNCS */

/* Get macros */

#define graphresult()       ((active_window == NULL) ? -1 : grOk)
#define grapherrormsg(a)    "Can't open display"
#define getcolor()          win_getcolor(active_window)
#define setcolor(c)         win_setcolor(active_window, (c))
#define getmaxx()           (active_window->w-1)
#define getmaxy()           (active_window->h-1)
#define outtext(t)          win_outtext(active_window,(t))
#define outtextxy(x,y,t)    win_outtextxy(active_window,(x),(y),(t))
#define moveto(x,y)         win_moveto(active_window,(x),(y))
#define lineto(x,y)         win_lineto(active_window,(x),(y))
#define putpixel(x,y,c)     win_putpixel(active_window,(x),(y),(c))
#define line(a,b,c,d)       win_line(active_window,(a),(b),(c),(d))
#define bar(a,b,c,d)        win_rectfill(active_window,(a),(b),(c),(d))
#define setlinestyle(a,b,c)
#define setwritemode(a)
#define clearviewport()

#define getch()             win_read_key(active_window)
#define kbhit()             win_has_key(active_window)
#define delay(n)            usleep(((unsigned long)(n))*1000ul);

#endif  /* BGI_FUNCS */

extern char *xd_bgi_window_title;    /* Must be set *before* initgraph() */

int initgraph(int *, int *, char *);

#define VGA            0     /* dummy */
#define VGAHI          0     /* dummy */
#define grOk           0
#define LIGHTMAGENTA   COLOR_MAGENTA
#define YELLOW         COLOR_YELLOW
#define GREEN          COLOR_GREEN1


/************************************************************/
/* Mouse & keyboard functions *******************************/

#define MOUSE_L  Button1Mask      /* Left */
#define MOUSE_R  Button3Mask      /* Right */
#define MOUSE_M  Button2Mask      /* Middle */

int  win_grab_pointer(WINDOW *w, int confine, Cursor cursor);
void win_ungrab_pointer(WINDOW *w);
void win_setcursor(WINDOW *w, Cursor cursor);
void win_setfont(WINDOW *w, FONT *font);
FONT *win_getfont(WINDOW *w);
int  win_read_key(WINDOW *win);
int  win_has_key(WINDOW *win);
int  win_read_mouse(WINDOW *win, int *ret_x, int *ret_y);

#define grab_pointer(c)    win_grab_pointer(active_window, c)
#define ungrab_pointer()   win_ungrab_pointer(active_window)
#define getfont()          win_getfont(active_window)
#define setfont(f)         win_setfont(active_window, (f))
#define setcursor(c)       win_setcursor(active_window, (c))
#define has_key()          win_has_key(active_window)
#define read_key()         win_read_key(active_window)
#define read_mouse(rx,ry)  win_read_mouse((active_window),(rx),(ry))


/************************************************************/
/* Dialog control procedures ********************************/


int window_proc     (int msg, WINDOW *w, DIALOG *dlg, int c);
int button_proc     (int msg, WINDOW *w, DIALOG *dlg, int c);
int xbutton_proc    (int msg, WINDOW *w, DIALOG *dlg, int c);
int text_proc       (int msg, WINDOW *w, DIALOG *dlg, int c);
int hline_proc      (int msg, WINDOW *w, DIALOG *dlg, int c);
int box_proc        (int msg, WINDOW *w, DIALOG *dlg, int c);
int fillbox_proc    (int msg, WINDOW *w, DIALOG *dlg, int c);
int edit_proc       (int msg, WINDOW *w, DIALOG *dlg, int c);
int xedit_proc      (int msg, WINDOW *w, DIALOG *dlg, int c);
int password_proc   (int msg, WINDOW *w, DIALOG *dlg, int c);
int list_proc       (int msg, WINDOW *w, DIALOG *dlg, int c);
int check_proc      (int msg, WINDOW *w, DIALOG *dlg, int c);
int menu_proc       (int msg, WINDOW *w, DIALOG *dlg, int c);
int image_proc      (int msg, WINDOW *w, DIALOG *dlg, int c);
int hscroll_proc    (int msg, WINDOW *w, DIALOG *dlg, int c);
int vscroll_proc    (int msg, WINDOW *w, DIALOG *dlg, int c);


/************************************************************/
/* General purpose functions ********************************/

int point_in_rect(int px, int py, int rx1, int ry1, int rx2, int ry2);

int rect_intersection  (int ax1, int ay1, int ax2, int ay2,
			int bx1, int by1, int bx2, int by2,
			int *rx1, int *ry1, int *rx2, int *ry2);
void rect_union        (int ax1, int ay1, int ax2, int ay2,
			int bx1, int by1, int bx2, int by2,
			int *rx1, int *ry1, int *rx2, int *ry2);


#ifdef __cplusplus
}  /* extern "C" */
#endif 

#endif /* XDIALOGS_H */
