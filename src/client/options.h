/* options.h */

#ifndef OPTIONS_H_FILE
#define OPTIONS_H_FILE


typedef struct {
  int debug;             /* Show error messages */
  int credits;           /* Start showing the credits */
  int frames_per_draw;   /* # of frames to process on each screen redraw */
  char *name;            /* Set the player name to `name' */
  char jack[256];        /* Set jack type to `jack' */
  char ask_map[256];     /* Ask the server to use this map (if owner) */
  int do_go;             /* Ask the server to start */
  char *record_file;     /* File to record to */
  char *playback_file;   /* File to playback from */

#ifdef HAS_SOUND
  char *midi_dev;        /* Midi device (/dev/sequencer) */
  char *sound_dev;       /* Digital sound device (/dev/dsp) */
#endif

  int conn_type;         /* Connection type (CONNECT_NETWORK) */
  char host[256];        /* Server host (localhost) */
  int port;              /* Server port (5042) */

#ifdef GRAPH_DOS
  int gmode;             /* Graph mode (0) */
  int width;             /* Screen width (320) */
  int height;            /* Screen height (200) */
  int bpp;               /* Color depth (8) */
  int page_flip;         /* 1 to do page flipping (1) */
#endif /* GRAPH_DOS */

#ifdef GRAPH_X11
  int width;             /* Window width (320) */
  int height;            /* Window height (200) */
#ifndef NO_SHM
  int no_shm;            /* Don't use shared memory (0) */
#endif /* NO_SHM */
  int double_screen;     /* Double screen (1) */
  int no_scanlines;      /* 0 to draw only even lines (when doubled) (0) */
#endif /* GRAPH_X11 */

#ifdef GRAPH_SVGALIB
  int gmode;             /* Graphics mode (0) */
#endif /* GRAPH_SVGALIB */
} OPTIONS;

#endif /* OPTIONS_H_FILE */
