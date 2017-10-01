/* screen.h
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#ifndef SCREEN_H_FILE
#define SCREEN_H_FILE

#include "common.h"

#ifdef _WINDOWS
#include "sys/timeb.h"
#endif /* _WINDOWS */

#define DISP_X      64
#define DISP_Y      64

#define ENEMY_ENERGY_FRAMES   80    /* # of frames to show eneemy energy */


typedef struct FPS_INFO {      /* FPS Counter */
  int frames;               /* Current frame counter */
  int last_fps;             /* Last FPS count */
  int cur_sec;              /* Running second */
#ifdef GRAPH_WIN
  struct _timeb cur_time;
#else
  struct timeval cur_time;
#endif /* GRAPH_WIN */
} FPS_INFO;

void gr_printf(int x, int y, BMP_FONT *font, const char *fmt, ...);
void gr_printf_selected(int sel, int x, int y,
			BMP_FONT *font, const char *fmt, ...);

#ifdef GRAPH_DOS
#include "dos/screen.h"
#endif /* GRAPH_DOS */

#ifdef GRAPH_WIN
#include "win/screen.h"
#endif /* GRAPH_WIN */

#ifdef GRAPH_X11
#include "X/screen.h"
#endif /* GRAPH_X11 */

#ifdef GRAPH_SVGALIB
#include "svgalib/screen.h"
#endif /* GRAPH_SVGALIB */

#endif /* SCREEN_H_FILE */
