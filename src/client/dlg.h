/* dlg.h
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#ifndef DLG_H_FILE
#define DLG_H_FILE

enum {                   /* Dialog actions */
  DLGACT_DRAW,                    /* Redraw */
  DLGACT_KEY,                     /* Key press */
};

enum {                   /* Dialog return codes */
  DLGRET_FOCUSNEXT,               /* Focus to next control */
  DLGRET_FOCUSPREV,               /* Focus to previous control */
  DLGRET_RETURN,                  /* Quit dialog */
};

typedef struct GAME_DLG_ITEM {
  int x, y;                       /* Position */
  int selected;                   /* 1 if selected */
  int enabled;                    /* 1 if enabled */
  char text[256];                 /* Label or edit text */
  XBITMAP *bmp;                   /* Bitmap */
  int (*func)(int, GAME_DLG_ITEM *);   /* Callback function */
} GAME_DLG_ITEM;

typedef struct GAME_DLG {
  XBITMAP *back;                  /* Background bitmap (may be NULL) */
  void *data;                     /* User data */
  int n_items;                    /* # of items */
  GAME_DLG_ITEM *item;            /* Items */
} GAME_DLG;


int execute_dialog(GAME_DLG *dlg);

#endif /* DLG_H_FILE */
