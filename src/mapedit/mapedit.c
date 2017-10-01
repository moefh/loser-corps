/* mapedit.c */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#define USE_ICON_PIXMAP

#ifdef USE_ICON_PIXMAP
#include <X11/xpm.h>
#include "icon.xpm"
#endif /* USE_ICON_PIXMAP */

#include "mapedit.h"
#include "procs.h"
#include "dialogs.h"
#include "config.h"
#include "pal8bpp.h"

OPTIONS s_options, *options = &s_options;
STATE state;
#ifdef USE_SHM
int use_shm = 1;
#endif

/* Read object bitmaps */
static void read_obj_bitmaps(STATE *s)
{
  char obj_filename[256];
  int i;

  for (i = 0; i < MAX_BMPS; i++)
    s->obj_bmp[i] = NULL;

  for (i = 0; npc_names[i].npc >= 0; i++) {
    sprintf(obj_filename, "%s/%dbpp/%s.spr",
	    options->data_dir, 8*SCREEN_BPP,
	    s->npc_info[npc_names[i].npc].file);
    s->obj_bmp[npc_names[i].npc] = read_xbitmap(obj_filename);

    if (s->obj_bmp[npc_names[i].npc] == NULL) {
      printf("Error: can't read object bitmap file `%s'\n", obj_filename);
      exit(1);
    }
    if (s->obj_bmp[npc_names[i].npc]->bpp != SCREEN_BPP) {
      printf("Error: object bitmap file `%s' has wrong depth\n",
	     obj_filename);
      exit(1);
    }
  }
}

/* Initialize the state */
void init_state(STATE *s, OPTIONS *options)
{
  char block_filename[256];
  char back_filename[256];
  char npc_info_filename[256];

#ifdef USE_SHM
  use_shm = options->use_shm;
#endif

  s->selected_obj = -1;
  s->dbl_bmp = create_xbitmap(main_dlg[MAINDLG_MAP].w / 2 + 2 * DISP_X,
			      main_dlg[MAINDLG_MAP].h / 2 + 2 * DISP_Y,
			      SCREEN_BPP);
  if (s->dbl_bmp == NULL) {
    printf("Error: can't create temporary bitmap\n");
    exit(1);
  }

  /* Read the NPC info */
  sprintf(npc_info_filename, "%s/npcs.dat", options->data_dir);
  s->num_npcs = parse_npc_info(s->npc_info, npc_info_filename);
  if (s->num_npcs <= 0)
    exit(1);
  build_npc_table(s);

  /* Read block bitmaps */
  sprintf(block_filename, "%s/%dbpp/%s",
	  options->data_dir, 8*SCREEN_BPP, options->block_bmp_file);
  s->n_block_bmps = read_xbitmaps(block_filename, MAX_BMPS, s->block_bmp);
  if (s->n_block_bmps <= 0) {
    printf("Error: can't read block bitmap file `%s'\n", block_filename);
    exit(1);
  }
  if (s->block_bmp[0]->bpp != SCREEN_BPP) {
    printf("Error: block bitmap file `%s' has wrong depth\n",
	   block_filename);
    exit(1);
  }

  /* Read back bitmaps */
  sprintf(back_filename, "%s/%dbpp/%s",
	  options->data_dir, 8*SCREEN_BPP, options->back_bmp_file);
  s->n_back_bmps = read_xbitmaps(back_filename, MAX_BMPS, s->back_bmp);
  if (s->n_back_bmps <= 0) {
    printf("Error: can't read back bitmap file `%s'\n", back_filename);
    exit(1);
  }
  if (s->back_bmp[0]->bpp != SCREEN_BPP) {
    printf("Error: back bitmap file `%s' has wrong depth\n",
	   back_filename);
    exit(1);
  }

  read_obj_bitmaps(s);

  /* Undo info */
  s->undo_pos = s->undo_start = 0;

  /* Map filename */
  s->filename[0] = '\0';
  s->map = NULL;

  /* Bitmaps filename */
  s->bmp_filename[0] = '\0';
  s->n_bmps = 0;

  /* Editor state */
  s->x = s->y = 0;
  s->layers = (LAYER_FOREGROUND | LAYER_BACKGROUND
	       | LAYER_GRIDLINES | LAYER_BLOCKS);
  s->cur_layer = 0;
  s->pixel_size = 1;
}

static char *get_file_ext(char *filename)
{
  char *dot, *slash;

  slash = strrchr(filename, '/');
  dot = strrchr(filename, '.');
  if (dot != NULL && (slash == NULL || dot > slash))
    return dot + 1;
  return filename + strlen(filename);
}

/* Returns a pointer to the filename in the given path */
char *get_filename(char *path)
{
  char *ret;

  ret = strrchr(path, '/');
  if (ret == NULL)
    return path;
  return ret + 1;
}

static void install_palette(char *file)
{
  int i;
  FILE *f;
  char filename[256];

  sprintf(filename, file, 8*SCREEN_BPP);

  if ((f = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "Warning: can't open palette file `%s'\n", filename);
    return;
  }

  for (i = 0; i < PAL_SIZE; i++) {
    palette_data[3 * i] = fgetc(f);
    palette_data[3 * i + 1] = fgetc(f);
    palette_data[3 * i + 2] = fgetc(f);
  }
  calc_install_palette();
  xd_set_palette(palette_data, 0, 255);

  xd_light_3d = MAKECOLOR8(255,255,255);
  xd_shadow_3d = MAKECOLOR8(128,128,128);
  xd_back_3d = MAKECOLOR8(196,196,196);
  gui_bg_color = MAKECOLOR8(255,255,255);
}

/* Read the bitmaps for the given state */
int read_state_bitmaps(STATE *s, char *filename)
{
  XBITMAP *bmp[MAX_BMPS];
  int i, n;

  if (SCREEN_BPP == 1) {
    char pal_filename[256], *ext;

    strcpy(pal_filename, filename);
    ext = get_file_ext(pal_filename);
    strcpy(ext, "pal");
    install_palette(pal_filename);
  }

  n = read_xbitmaps(filename, MAX_BMPS, bmp);
  if (n <= 0)
    return 1;

  for (i = 0; i < s->n_bmps; i++)
    destroy_xbitmap(s->bmp[i]);

  s->n_bmps = n;
  for (i = 0; i < n; i++)
    s->bmp[i] = bmp[i];
  strcpy(s->bmp_filename, filename);
  if (s->map != NULL) {
    s->map->tile_w = s->bmp[0]->w;
    s->map->tile_h = s->bmp[0]->h;
  }
  return 0;
}

/* Adjust the main dialog map display size */
void adjust_map_size(STATE *s)
{
  int disp_w, disp_h;
  DIALOG *dlg;

  if (s->n_bmps == 0 || s->bmp[0] == NULL)
    return;

  disp_w = s->map->w * s->bmp[0]->w;
  disp_h = s->map->h * s->bmp[0]->h;
  if (disp_w > MAPDLG_MAX_W)
    disp_w = MAPDLG_MAX_W;
  if (disp_h > MAPDLG_MAX_H)
    disp_h = MAPDLG_MAX_H;

  dlg = main_dlg + MAINDLG_MAP;
  dlg->x = (MAPDLG_MAX_W - disp_w) / 2;
  dlg->y = (MAPDLG_MAX_H - disp_h) / 2 + main_dlg[MAINDLG_MENU].h;
  dlg->w = disp_w;
  dlg->h = disp_h;

  if (dlg->win != NULL) {
    win_movewindow(dlg->win, dlg->x, dlg->y);
    win_resizewindow(dlg->win, dlg->w, dlg->h);
  }
}


/* Read a map for the given state */
int read_state_map(STATE *s, char *filename)
{
  MAP *map;
  int x, y;
  char bmp_filename[256];

  map = read_map(filename);
  if (map == NULL)
    return 1;
  for (y = 0; y < map->h; y++)
    for (x = 0; x < map->w; x++)
      map->line[y][x].sel = 0;
  if (s->map != NULL)
    destroy_map(s->map);
  s->map = map;
  strcpy(s->filename, filename);
  adjust_map_size(s);

  {
    char tiles_filename[256], *p;

    strcpy(tiles_filename, s->map->tiles);
    if ((p = strchr(tiles_filename, ':')) != NULL)
      *p = '\0';
    sprintf(bmp_filename, "%s/%dbpp/tiles/%s.spr",
	    options->data_dir, 8*SCREEN_BPP, tiles_filename);
    if (read_state_bitmaps(s, bmp_filename))
      printf("Warning: can't read tiles from `%s'\n", bmp_filename);
  }
  return 0;
}

/* Create a new map for the given state */
int create_state_map(STATE *s, int w, int h, int fore, int back)
{
  MAP *map;
  int i, j;

  map = create_map(w, h);
  if (map == NULL)
    return 1;

  if (s->map != NULL)
    destroy_map(s->map);

  s->map = map;
  for (i = 0; i < h; i++) {
    for (j = 0; j < w; j++) {
      s->map->line[i][j].fore = fore;
      s->map->line[i][j].back = back;
      s->map->line[i][j].block = -1;
      s->map->line[i][j].sel = 0;
    }
  }

  s->filename[0] = '\0';
  s->tool = TOOL_TILES;

  adjust_map_size(s);

  return 0;
}

int change_map_size(STATE *s, int new_w, int new_h, int fore, int back)
{
  int i, j, w, h;
  MAP *map;

  map = create_map(new_w, new_h);
  if (map == NULL)
    return 1;

  strcpy(map->author, s->map->author);
  strcpy(map->comment, s->map->comment);
  strcpy(map->tiles, s->map->tiles);
  map->maxyspeed = s->map->maxyspeed;
  map->jumphold = s->map->jumphold;
  map->gravity = s->map->gravity;
  map->maxxspeed = s->map->maxxspeed;
  map->accel = s->map->accel;
  map->jumpaccel = s->map->jumpaccel;
  map->attrict = s->map->attrict;
  map->frameskip = s->map->frameskip;

  w = (s->map->w > new_w) ? new_w : s->map->w;
  h = (s->map->h > new_h) ? new_h : s->map->h;

  for (i = 0; i < new_h; i++)
    for (j = 0; j < new_w; j++) {
      map->line[i][j].fore = fore;
      map->line[i][j].back = back;
      map->line[i][j].block = -1;
      map->line[i][j].sel = 0;
    }
  
  for (i = 0; i < h; i++)
    for (j = 0; j < w; j++)
      map->line[i][j] = s->map->line[i][j];

  destroy_map(s->map);
  s->map = map;
  s->x = s->y = 0;
  adjust_map_size(s);

  return 0;
}

/* Reset the state after a change in the map or the bitmaps */
void reset_state(STATE *s, int map_changed, int bmps_changed)
{
  if (bmps_changed) {
    XImage *img;

    s->start_tile = s->start_block = 0;
    s->cur_tile = s->cur_block = 0;
    img = (XImage *) main_dlg[MAINDLG_TILE_IMG].dp;
    if (img != NULL && s->n_bmps > 0)
      memset(img->data, xd_back_3d, img->height * img->bytes_per_line);
    SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_TILE_SCROLL, 0);

    update_tile_images();
    SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_TILE_IMG, 0);
  }

  if (map_changed) {
    /* Clear undo info */
    s->undo_pos = s->undo_start = 0;
    s->selected_obj = -1;

    /* Reset active layer to foreground */
    s->cur_layer = 0;
    main_dlg[MAINDLG_RADIOLAYER_FORE].d1 = 1;
    main_dlg[MAINDLG_RADIOLAYER_BACK].d1 = 0;
    SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_RADIOLAYER_FORE, 0);
    SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_RADIOLAYER_BACK, 0);

    /* Set drawing layers to BACK and FORE */
    s->layers = (LAYER_FOREGROUND | LAYER_BACKGROUND
		 | LAYER_GRIDLINES | LAYER_BLOCKS);
    main_dlg[MAINDLG_LAYER_FORE].d1 = 1;
    main_dlg[MAINDLG_LAYER_BACK].d1 = 1;
    main_dlg[MAINDLG_LAYER_GRID].d1 = 1;
    main_dlg[MAINDLG_LAYER_BLOCK].d1 = 1;
    SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_LAYER_FORE, 0);
    SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_LAYER_BACK, 0);
    SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_LAYER_GRID, 0);
    SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_LAYER_BLOCK, 0);

    /* Reset doubling toggle */
    if (s->pixel_size == 1)
      main_dlg[MAINDLG_DOUBLE].d1 = 0;
    else
      main_dlg[MAINDLG_DOUBLE].d1 = 1;
    SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_DOUBLE, 0);

    /* Reset screen position */
    state.x = state.y = 0;
  }
    
  SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_MAP, 0);
}



static void print_help(char *name)
{
  printf("The L.O.S.E.R Corps game map editor\n"
	 "\n"
	 "%s [options] map-file\n"
	 "\n"
	 "options:\n"
	 "  -h, -help      print this help\n"
	 "  -fn FONT       use the font `FONT'\n"
#ifdef USE_SHM
	 "  -shm           use X shared memory extensions\n"
	 "  -noshm         don't use X shared memory extensions\n"
#endif
	 "\n", name);
}

void read_cmdline_options(int argc, char *argv[], OPTIONS *options)
{
  int i;

  for (i = 1; i < argc; i++)
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0) {
      print_help(argv[0]);
      exit(0);
#ifdef USE_SHM
    } else if (strcmp(argv[i], "-noshm") == 0) {
      options->use_shm = 0;
    } else if (strcmp(argv[i], "-shm") == 0) {
      options->use_shm = 1;
#endif
    } else if (strcmp(argv[i], "-fn") == 0) {
      if (argv[i + 1] == NULL) {
	printf("Expected value for option `-fn'\n");
	exit(1);
      }
      strcpy(options->font, argv[++i]);
    } else if (argv[i][0] != '-')
      strcpy(options->map_file, argv[i]);
    else {
      printf("Unknown option: `%s'\n", argv[i]);
      exit(1);
    }
}

#ifdef USE_ICON_PIXMAP
void win_set_icon(WINDOW *win, char **data)
{
  XWMHints *wm_hints;
  Pixmap icon, mask;

  if (XCreatePixmapFromData(win->disp, win->window, data, &icon, &mask, NULL)
      != XpmSuccess)
    return;

  if ((wm_hints = XAllocWMHints()) == NULL) {
    XFreePixmap(win->disp, icon);
    XFreePixmap(win->disp, mask);
    return;
  }
  wm_hints->flags = IconPixmapHint | IconMaskHint | WindowGroupHint;
  wm_hints->icon_pixmap = icon;
  wm_hints->icon_mask = mask;
  wm_hints->window_group = win->window;
  XSetWMProperties(win->disp, win->window, NULL, NULL,
		   NULL, 0, NULL, wm_hints, NULL);
  XFree(wm_hints);
}
#endif /* USE_ICON_PIXMAP */


void check_data_dir(char *data_dir)
{
  char maps_dir[1024];

  sprintf(maps_dir, "%s/maps", data_dir);
  if (access(maps_dir, R_OK) != 0) {
    strcpy(maps_dir, LOCAL_DATA_DIR "maps");
    if (access(maps_dir, R_OK) == 0)
      strcpy(data_dir, LOCAL_DATA_DIR);
  }
}

int main(int argc, char *argv[])
{
  WINDOW *main_window;
  char filename[256];

  xd_window_class = "LoserMap";
  xd_window_name = "loser";

  fill_default_options(options);
  read_options("mapeditrc", options);
  read_cmdline_options(argc, argv, options);
  strcpy(xd_font_name, options->font);

  /* Chack if the data directory on the rc file name is good.
   * If not, search in one of the standard locations */
  check_data_dir(options->data_dir);

  if (init_graph(NULL) != 0) {
    fprintf(stderr, "Can't open display (are you running X?)\n");
    exit(1);
  }
  switch (DefaultDepth(xd_display, DefaultScreen(xd_display))) {
  case 8:
    SCREEN_BPP = 1;
    xd_install_palette();
    break;

  case 16:
    SCREEN_BPP = 2;
    break;

  case 24:
    SCREEN_BPP = 2;
    break;

  case 32:
    SCREEN_BPP = 2;
    break;

  default:
    printf("Sorry, the map editor doesn't support this display depth (%d).\n"
	   "Your X must be configured with a display depth of 8 or 16\n",
	   DefaultDepth(xd_display, DefaultScreen(xd_display)));
    exit(1);
  }

  init_state(&state, options);

#if PRINT_DEBUG
  printf("loser-map: using data dir `%s'\n", options->data_dir);
#endif

  sprintf(filename, "%s/%s", options->data_dir, options->map_file);
  if (read_state_map(&state, filename)) {
    printf("Warning: can't read file `%s'\n", filename);
    if (create_state_map(&state, 32, 32, -1, -1)) {
      printf("Out of memory\n");
      exit(1);
    }
  }
  main_dlg[MAINDLG_TILE_SCROLL].d2 = (state.n_bmps
				      - (main_dlg[MAINDLG_TILE_IMG].w
					 / main_dlg[MAINDLG_TILE_IMG].h)
				      + 2);

  xd_stddlg_3d = 1;
  set_dialog_flag(main_dlg, D_3D, 1);
  main_window = create_window(NULL, 0, 0, 0, 0, NULL, main_dlg);
  if (main_window == NULL) {
    fprintf(stderr, "Can't create main window\n");
    exit(1);
  }
#ifdef USE_ICON_PIXMAP
  win_set_icon(main_window, icon_xpm);
#endif /* USE_ICON_PIXMAP */

  do_dialog(main_window, 1, MAINDLG_MAP);

  closegraph();
  return 0;
}
