/* mapedit.h */

#ifndef MAPEDIT_H_FILE
#define MAPEDIT_H_FILE


#define PRINT_DEBUG  0


#include <xdialogs.h>
#ifdef USE_SHM
#include <X11/extensions/XShm.h>

extern int use_shm;     /* 1 to create images on shared memory */
#endif /* USE_SHM */

#include "map.h"
#include "bitmap.h"
#include "common.h"
#include "npc_data.h"

#ifndef MIN
#define MIN(a,b)    (((a) < (b)) ? (a) : (b))
#define MAX(a,b)    (((a) > (b)) ? (a) : (b))
#endif

#define DISP_X   64
#define DISP_Y   64
#define SCREEN_BPP  screen_bpp

extern int screen_bpp;


typedef struct IMAGE {
#ifdef USE_SHM
  int is_shared;          /* 1 if the image is shared */
  XShmSegmentInfo info;
#endif /* USE_SHM */
  XImage *img;
} IMAGE;

IMAGE *create_image(Display *disp, int w, int h, int bpp);
void destroy_image(Display *disp, IMAGE *s);
void display_image(WINDOW *win, int x, int y,
		   IMAGE *image, int img_x, int img_y, int w, int h);

void x_put_image(Display *disp, Drawable d, GC gc, XImage *image, int src_x, int src_y, int dest_x, int dest_y, unsigned int width, unsigned int height);

#define LAYER_FOREGROUND   0x01
#define LAYER_BACKGROUND   0x02
#define LAYER_GRIDLINES    0x04
#define LAYER_BLOCKS       0x08

#define MAX_BMPS          256      /* Max bitmaps */
#define MAX_UNDO          256      /* Max undo queue */

enum {
  TOOL_TILES,        /* Change the tiles */
  TOOL_BLOCKS,       /* Change blocking attributes */
  TOOL_OBJECTS,      /* Change the objects */
};

typedef struct UNDO {
  int x, y;                     /* Position of changed tile */
  int layer;                    /* Layer of changed tile */
  int tile;                     /* Old tile */
} UNDO;

typedef struct STATE {
  XBITMAP *dbl_bmp;              /* Temporary bitmap to double the screen */
  int n_block_bmps;              /* # of blocking bitmaps */
  XBITMAP *block_bmp[MAX_BMPS];  /* Blocking bitmaps */
  int n_back_bmps;               /* # of background bitmaps */
  XBITMAP *back_bmp[MAX_BMPS];   /* Back bitmaps (parallax) */
  XBITMAP *obj_bmp[MAX_BMPS];    /* Object bitmaps */
  
  int num_npcs;                  /* Number of NPC infos */
  NPC_INFO npc_info[MAX_NPCS];   /* NPC info */
  
  int pixel_size;               /* 1 if screen is normal, 2 if doubled */
  int tool;                     /* TOOL_xxx: current tool */
  int selected_obj;             /* Index of the selected object (-1 if none) */
  
  int n_bmps;                   /* # of tiles */
  XBITMAP *bmp[MAX_BMPS];       /* Game tiles */
  char bmp_filename[256];       /* Tiles filename (.spr) */
  char pal_filename[256];       /* Palette filename */
  
  MAP *map;                     /* Game map data */
  char filename[256];           /* Map filename (.map) */
  
  UNDO undo[MAX_UNDO];          /* Undo information */
  int undo_pos;                 /* Next free undo position */
  int undo_start;               /* Start of undo info */
  
  int x, y;                     /* Position in the map */
  int layers;                   /* Layers to display */
  int cur_layer;                /* Current layer */
  int start_tile;
  int cur_tile;                 /* Current tile */
  int start_block;
  int cur_block;                /* Current block */
} STATE;


extern STATE state;
//extern XBITMAP *tmp;

int read_state_bitmaps(STATE *s, char *filename);
int read_state_map(STATE *s, char *filename);
int create_state_map(STATE *s, int w, int h, int fore, int back);
int change_map_size(STATE *s, int new_w, int new_h, int fore, int back);
void reset_state(STATE *s, int map_changed, int bmps_changed);
char *get_filename(char *path);

void build_npc_table(STATE *s);

void update_tile_txt(WINDOW *win, DIALOG *dlg, int use_mouse, int force);

void display_map(WINDOW *win, int w, int h, IMAGE *img, STATE *s);


#endif /* MAPEDIT_H_FILE */
