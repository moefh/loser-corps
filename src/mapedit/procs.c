/* procs.c */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "mapedit.h"
#include "dialogs.h"
#include "procs.h"
#include "pal8bpp.h"

#ifndef SQR
#define SQR(x)   ((x)*(x))
#endif /* SQR */

#ifndef ABS
#define ABS(x)   (((x) < 0) ? -(x) : (x))
#endif /* ABS */


typedef struct MAP_POINT {
  short int x, y;
  MAP_DATA *d;
} MAP_POINT;


void prepare_dialog_colors(DIALOG *dlg)
{
  int i;

  if (SCREEN_BPP != 1)
    return;

  for (i = 0; dlg[i].proc != NULL; i++)
    if (dlg[i].type == CTL_EDIT || dlg[i].type == CTL_LIST)
      dlg[i].bg = gui_bg_color;
}

static void paste_map_sel(int tile_x, int tile_y)
{
  int x, y, i, n_sel, tmp;
  MAP_POINT *sel, *p;

  n_sel = 0;
  for (y = 0; y < state.map->h; y++)
    for (x = 0; x < state.map->w; x++)
      if (state.map->line[y][x].sel)
	n_sel++;

  if (n_sel == 0)
    return;

  sel = (MAP_POINT *) malloc(sizeof(MAP_POINT) * n_sel);
  if (sel == 0) {
    XBell(xd_display, 100);
    return;
  }
  n_sel = 0;
  for (y = 0; y < state.map->h; y++)
    for (x = 0; x < state.map->w; x++)
      if (state.map->line[y][x].sel) {
	sel[n_sel].x = x;
	sel[n_sel].y = y;
	sel[n_sel++].d = state.map->line[y] + x;
      }

  p = sel;
  for (i = 0; i < n_sel; i++) {
    y = tile_y + p->y - sel->y;
    x = tile_x + p->x - sel->x;
    if (x >= 0 && y >= 0 && x < state.map->w && y < state.map->h) {
      tmp = state.map->line[y][x].sel;
      state.map->line[y][x] = *p->d;
      state.map->line[y][x].sel = tmp;
    }
    p++;
  }

  free(sel);
}

/* Set a map tile selection */
static void set_tile_sel(int x, int y, int set)
{
  state.map->line[y][x].sel = set;
}

/* Set a tile in the map */
static void set_map_tile(int x, int y, int layer, int tile, int record_undo)
{
  UNDO undo;

  undo.x = x;
  undo.y = y;
  undo.layer = layer;
  switch (layer) {
  case ML_FORE:
    undo.tile = state.map->line[y][x].fore;
    state.map->line[y][x].fore = tile;
    break;

  case ML_BACK:
    undo.tile = state.map->line[y][x].back;
    state.map->line[y][x].back = tile;
    break;

  default:
    undo.tile = state.map->line[y][x].block;
    state.map->line[y][x].block = tile;
    break;
  }

  if (record_undo) {
    state.undo[state.undo_pos++] = undo;
    state.undo_pos %= MAX_UNDO;
    if (state.undo_pos == state.undo_start) {
      state.undo_start++;
      state.undo_start %= MAX_UNDO;
    }
  }
}



/* Get an object from its index */
static MAP_OBJECT *get_object(MAP_OBJECT *obj, int index)
{
  if (index < 0)
    return NULL;

  while (index-- > 0 && obj != NULL)
    obj = obj->next;
  return obj;
}

/* Return the index of an object in a list */
static int object_index(MAP_OBJECT *list, MAP_OBJECT *obj)
{
  int n = 0;

  if (obj == NULL)
    return -1;

  while (list != obj && list != NULL) {
    list = list->next;
    n++;
  }
  if (list == NULL)
    return -1;
  return n;
}


/* Create a new object and place it in the map */
static MAP_OBJECT *create_map_object(STATE *s)
{
  MAP_OBJECT *obj, *cur;

  obj = (MAP_OBJECT *) malloc(sizeof(MAP_OBJECT));
  if (obj == NULL)
    return NULL;

  memset(obj, 0, sizeof(MAP_OBJECT));
  fill_default_obj_values(obj);

  /* Make target references by pointers */
  for (cur = state.map->obj; cur != NULL; cur = cur->next)
    cur->temp = get_object(state.map->obj, cur->target);

  /* Insert the object */
  obj->next = s->map->obj;
  s->map->obj = obj;

  /* Rebuild target references by index */
  for (cur = state.map->obj; cur != NULL; cur = cur->next)
    cur->target = object_index(state.map->obj, cur->temp);
  return obj;
}

/* Make and return a copy of the given map object */
static MAP_OBJECT *copy_map_object(STATE *s, MAP_OBJECT *obj)
{
  MAP_OBJECT *new_obj;

  new_obj = create_map_object(s);
  if (new_obj == NULL)
    return NULL;

  new_obj->npc = obj->npc;
  new_obj->x = obj->x + s->obj_bmp[obj->npc]->w;
  new_obj->y = obj->y;
  new_obj->dir = obj->dir;
  new_obj->respawn = obj->respawn;
  new_obj->level = obj->level;
  new_obj->duration = obj->duration;
  new_obj->target = obj->target;
  new_obj->vulnerability = obj->vulnerability;
  strcpy(new_obj->text, obj->text);

  return new_obj;
}

/* Remove an object from the map */
void delete_map_object(MAP_OBJECT *obj)
{
  MAP_OBJECT *cur, *prev = NULL;

  /* Make target references by pointers */
  for (cur = state.map->obj; cur != NULL; cur = cur->next)
    cur->temp = get_object(state.map->obj, cur->target);

  /* Remove the object */
  for (cur = state.map->obj; cur != NULL; prev = cur, cur = cur->next)
    if (cur == obj) {
      if (prev == NULL)
	state.map->obj = cur->next;
      else
	prev->next = cur->next;
      free(cur);
      break;
    }

  /* Rebuild target references by index */
  for (cur = state.map->obj; cur != NULL; cur = cur->next)
    cur->target = object_index(state.map->obj, cur->temp);
}


/* Return the index of the nearest object from (x,y) */
static int get_map_object_at(MAP_OBJECT *obj, int x, int y)
{
  int index, dist, best, tmp, npc;

  dist = best = -1;
  for (index = 0; obj != NULL; index++, obj = obj->next) {
    if (obj->npc < 0)
      npc = 0;
    else
      npc = obj->npc;

    if (obj->x <= x && obj->x + state.obj_bmp[npc]->w >= x
	&& obj->y <= y && obj->y + state.obj_bmp[npc]->h >= y) {
      tmp = (SQR((obj->x + state.obj_bmp[npc]->w/2) - x) +
	     SQR((obj->y + state.obj_bmp[npc]->h/2) - y));
      if (dist < 0 || tmp < dist) {
	dist = tmp;
	best = index;
      }
    }
  }

  return best;
}

/* Set or move a map object */
static int set_map_object(WINDOW *win, int x, int y, int right_button,
			  int shift_key, int control_key, int alt_key,
			  int selection)
{
  MAP_OBJECT *obj;
  int old_sel = state.selected_obj;

  if (selection < 0) {
    state.selected_obj = selection = get_map_object_at(state.map->obj, x, y);
    if (right_button && selection >= 0)
      SEND_MESSAGE(MSG_DRAW, win->dialog, 0);
  }

  if (right_button) {
    if (selection >= 0) {        /* Change attributes */
      obj = get_object(state.map->obj, selection);
      if (obj != NULL) {
	win_ungrab_pointer(win);
	set_object_attributes(win->parent, obj,
			      object_index(state.map->obj, obj));
	win_grab_pointer(win, 1, None);
      }
    } else {                     /* Make new */
      obj = create_map_object(&state);
      if (obj == NULL)
	return -1;
      obj->x = x - state.obj_bmp[(obj->npc >= 0) ? obj->npc : 0]->w / 2;
      obj->y = y - state.obj_bmp[(obj->npc >= 0) ? obj->npc : 0]->h / 2;
      state.selected_obj = selection = object_index(state.map->obj, obj);
    }
  } else {                       /* Move or target */
    /* printf("(s,c) = (%d,%d)\n", shift_key, control_key); */
    if (selection >= 0) {
	obj = get_object(state.map->obj, selection);
      if (control_key) {         /* Target */
	MAP_OBJECT *old_obj;

	if (old_sel >= 0) {
	  old_obj = get_object(state.map->obj, old_sel);
	  if (old_obj != NULL) {
	    old_obj->target = state.selected_obj;
	    state.selected_obj = selection = old_sel;
	  }
	}	  
      } else {                   /* Move */
	if (obj == NULL)
	  selection = -1;
	else {
	  if (alt_key) {
	    int w = state.map->tile_w / 2;
	    int h = state.map->tile_h / 2;
	    obj->x = (x / w) * w - 1;
	    obj->y = (y / h) * h - 1;
	  } else {
	    obj->x = x - state.obj_bmp[(obj->npc >= 0) ? obj->npc : 0]->w / 2;
	    obj->y = y - state.obj_bmp[(obj->npc >= 0) ? obj->npc : 0]->h / 2;
	  }
	}
      }
    }
  }
  return selection;
}



static int map_can_undo(void)
{
  return (state.undo_pos != state.undo_start);
}

static void map_undo(void)
{
  if (map_can_undo()) {
    UNDO *u;

    state.undo_pos--;
    if (state.undo_pos < 0)
      state.undo_pos = MAX_UNDO - 1;
    u = &state.undo[state.undo_pos];
    set_map_tile(u->x, u->y, u->layer, u->tile, 0);
  }
}

/********************************************************************/


int menu_new(WINDOW *win, int id)
{
  int w, h, fore, back;

  if (state.map == NULL) {
    strcpy(newdlg_width_txt, "64");
    strcpy(newdlg_height_txt, "64");
  } else {
    sprintf(newdlg_width_txt, "%d", state.map->w);
    sprintf(newdlg_height_txt, "%d", state.map->h);
  }

  strcpy(newdlg_fore_txt, "-1");
  strcpy(newdlg_back_txt, "0");

  centre_dialog(new_dlg, win->parent);
  set_dialog_flag(new_dlg, D_GOTFOCUS, 0);
  set_dialog_flag(new_dlg, D_3D, 1);
  prepare_dialog_colors(new_dlg);
  if (do_dialog_window(win->parent, new_dlg, 1) != NEWDLG_BUT_OK)
    return D_O_K;

  w = atoi(newdlg_width_txt);
  h = atoi(newdlg_height_txt);
  fore = atoi(newdlg_fore_txt);
  back = atoi(newdlg_back_txt);

  if (w <= 0 || h <= 0) {
    alert(win->parent, "Error", "", "Invalid size.", "", "OK", NULL);
    return D_O_K;
  }
  if (create_state_map(&state, w, h, fore, back)) {
    alert(win->parent, "Error", "", "Can't create new map.", "", "OK", NULL);
    return D_O_K;
  }
  reset_state(&state, 1, 0);
  
  return D_O_K;
}

int menu_open(WINDOW *win, int id)
{
  char file[256];

  strcpy(file, state.filename);
  get_filename(file)[0] = '\0';
  if (file_select(win->parent, "Open", file, ".map") == 0)
    return D_O_K;
  if (read_state_map(&state, file)) {
    alert(win->parent, "Error", "", "Can't read map.", "", "OK", NULL);
    return D_O_K;
  }
  reset_state(&state, 1, 0);
  update_tile_images();

  return D_O_K;
}


static int save_map(WINDOW *win, char *file, MAP *map)
{
  int x, y, out_x, out_y;
  int has_error;

  out_x = out_y = 0;

  if (write_map(file, state.map)) {
    alert(win, "Error", "", "Can't write map.", "", "OK", NULL);
    return 1;
  }

  has_error = 0;
  for (y = 0; y < map->h; y++)
    for (x = 0; x < map->w; x++)
      if ((map->line[y][x].fore >= state.n_bmps
	   || map->line[y][x].back >= state.n_bmps) && ! has_error) {
	has_error = 1;
	out_x = x;
	out_y = y;
      }
  if (has_error) {
    char str[42];

    snprintf(str, sizeof(str), "(%d,%d)", out_x, out_y);
    alert(win, "Warning", "This map has a tile out of the range",
	  "of the current set of bitmaps:", str, "OK", NULL);
    return 0;
  }

  for (y = 0; y < map->h; y++)
    for (x = 0; x < map->w; x++)
      if (map->line[y][x].block >= state.n_block_bmps) {
	has_error = 1;
	out_x = x;
	out_y = y;
      }
  if (has_error) {
    char str[42];

    snprintf(str, sizeof(str), "(%d,%d)", out_x, out_y);
    alert(win, "Warning", "This map has a block out of the range",
	  "of the current set of blocks:", str, "OK", NULL);
    return 0;
  }
  return 0;
}

int menu_save(WINDOW *win, int id)
{
  if (state.filename[0] == '\0')
    return menu_save_as(win, id);

  save_map(win->parent, state.filename, state.map);
  return D_O_K;
}

int menu_save_as(WINDOW *win, int id)
{
  char file[256];

  strcpy(file, state.filename);
  get_filename(file)[0] = '\0';
  if (file_select(win->parent, "Save as", file, ".map") == 0)
    return D_O_K;
  if (save_map(win->parent, file, state.map) == 0)
    strcpy(state.filename, file);

  return D_O_K;
}


int menu_read_tiles(WINDOW *win, int id)
{
  char file[256];
  int len;

  strcpy(file, state.bmp_filename);
  get_filename(file)[0] = '\0';
  if (file_select(win->parent, "Read tiles", file, ".spr*") == 0)
    return D_O_K;

  /* Remove trailing `.gz', if exists */
  len = strlen(file);
  if (len > 3 && strcmp(file + len - 3, ".gz") == 0)
    file[len - 3] = '\0';

  if (read_state_bitmaps(&state, file)) {
    alert(win->parent, "Error", "", "Can't read tiles.", "", "OK", NULL);
    return D_O_K;
  }
  reset_state(&state, 0, 1);
  update_tile_images();

  return D_O_K;
}

int menu_export(WINDOW *win, int id)
{
  char file[512];

  snprintf(file, sizeof(file), "%s.h", state.filename);
  FILE *f = fopen(file, "w");
  if (f == NULL) {
    alert(win->parent, "Error", "Can't open file", file, "", "OK", NULL);
    return D_O_K;
  }

  MAP *map = state.map;
  
  fprintf(f, "/* File exported from '%s' */\n\n", state.filename);
  fprintf(f, "extern const MAP_TILE game_map_tiles[];\n");
  fprintf(f, "extern const MAP_SPAWN_POINT game_map_spawn_points[];\n");
  fprintf(f, "const MAP game_map = {\n");
  fprintf(f, "  .width = %d,\n", map->w);
  fprintf(f, "  .height = %d,\n", map->h);
  fprintf(f, "  .tiles = game_map_tiles,\n");
  fprintf(f, "  .num_spawn_points = 1,\n");
  fprintf(f, "  .spawn_points = game_map_spawn_points,\n");
  fprintf(f, "};\n\n");
  fprintf(f, "const MAP_TILE game_map_tiles[] = {");
  int num_tiles = 0;
  for (int y = 0; y < map->h; y++) {
    MAP_DATA *line = map->line[y];
    for (int x = 0; x < map->w; x++) {
      if (num_tiles++ % 3 == 0) {
        fprintf(f, "\n ");
      }
      fprintf(f, " {0x%04x,0x%04x,0x%04x},", (unsigned short)line[x].back, (unsigned short)line[x].fore, (unsigned short)line[x].block);
    }
  }
  fprintf(f, "\n};\n");
  fprintf(f, "const MAP_SPAWN_POINT game_map_spawn_points[] = {\n");
  fprintf(f, "  { { 0xf0000, 0xf0000 }, 1 },\n");
  fprintf(f, "};\n");
  
  fclose(f);
  return D_O_K;
}

int menu_exit(WINDOW *win, int id)
{
  return D_CLOSE;
}


int menu_undo(WINDOW *win, int id)
{
  if (map_can_undo()) {
    map_undo();
    SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_MAP, 0);
    return D_O_K;
  }
  alert(win->parent, "Error", "", "No undo information!", "", "OK", NULL);
  return D_O_K;
}

int menu_setsize(WINDOW *win, int id)
{
  int w, h;

  if (state.map == NULL) {
    alert(win->parent, "Error", "You must have a map", "to do that", "",
	  "OK", NULL);
    return D_O_K;
  }

  sprintf(sizedlg_width_txt, "%d", state.map->w);
  sprintf(sizedlg_height_txt, "%d", state.map->h);

  centre_dialog(size_dlg, win->parent);
  set_dialog_flag(size_dlg, D_GOTFOCUS, 0);
  set_dialog_flag(size_dlg, D_3D, 1);
  prepare_dialog_colors(size_dlg);
  if (do_dialog_window(win->parent, size_dlg, 1) != SIZEDLG_BUT_OK)
    return D_O_K;

  w = atoi(sizedlg_width_txt);
  h = atoi(sizedlg_height_txt);

  if (w < 1 || h < 1) {
    alert(win->parent, "Error", "", "Invalid size.", "", "OK", NULL);
    return D_O_K;
  }
  if (change_map_size(&state, w, h, -1, -1)) {
    alert(win->parent, "Error", "", "Can't change map size.", "", "OK", NULL);
    return D_O_K;
  }
  reset_state(&state, 1, 0);
  return D_REDRAW;
}


int menu_info(WINDOW *win, int id)
{
  if (state.map == NULL) {
    alert(win->parent, "Error", "", "No map!", "", "OK", NULL);
    return D_O_K;
  }

  strcpy(infodlg_author_txt, state.map->author);
  strcpy(infodlg_comment_txt, state.map->comment);
  strcpy(infodlg_tile_txt, state.map->tiles);

  centre_dialog(info_dlg, win->parent);
  set_dialog_flag(info_dlg, D_GOTFOCUS, 0);
  set_dialog_flag(info_dlg, D_3D, 1);
  prepare_dialog_colors(info_dlg);
  if (do_dialog_window(win->parent, info_dlg, 1) != INFODLG_BUT_OK)
    return D_O_K;

  strcpy(state.map->author, infodlg_author_txt);
  strcpy(state.map->comment, infodlg_comment_txt);
  strcpy(state.map->tiles, infodlg_tile_txt);

  return D_O_K;
}


int menu_parms(WINDOW *win, int id)
{
  if (state.map == NULL) {
    alert(win->parent, "Error", "", "No map!", "", "OK", NULL);
    return D_O_K;
  }

  sprintf(parmdlg_maxyspeed_txt, "%d", state.map->maxyspeed);
  sprintf(parmdlg_jumphold_txt,  "%d", state.map->jumphold);
  sprintf(parmdlg_gravity_txt,   "%d", state.map->gravity);
  sprintf(parmdlg_maxxspeed_txt, "%d", state.map->maxxspeed);
  sprintf(parmdlg_accel_txt,     "%d", state.map->accel);
  sprintf(parmdlg_jumpaccel_txt, "%d", state.map->jumpaccel);
  sprintf(parmdlg_attrict_txt,   "%d", state.map->attrict);
  sprintf(parmdlg_frameskip_txt, "%d", state.map->frameskip);

  centre_dialog(parm_dlg, win->parent);
  set_dialog_flag(parm_dlg, D_GOTFOCUS, 0);
  set_dialog_flag(parm_dlg, D_3D, 1);
  prepare_dialog_colors(parm_dlg);
  if (do_dialog_window(win->parent, parm_dlg, 1) != PARMDLG_BUT_OK)
    return D_O_K;

  state.map->maxyspeed = atoi(parmdlg_maxyspeed_txt);
  state.map->jumphold = atoi(parmdlg_jumphold_txt);
  state.map->gravity = atoi(parmdlg_gravity_txt);
  state.map->maxxspeed = atoi(parmdlg_maxxspeed_txt);
  state.map->accel = atoi(parmdlg_accel_txt);
  state.map->jumpaccel = atoi(parmdlg_jumpaccel_txt);
  state.map->attrict = atoi(parmdlg_attrict_txt);
  state.map->frameskip = atoi(parmdlg_frameskip_txt);

  return D_O_K;
}


int menu_about(WINDOW *win, int id)
{
  char *info[] = {
    "Author: Ricardo R. Massaro",
    "Internet: massaro@ime.usp.br",
    "",
    NULL,
  };

  about_box(win->parent, "About", "Map Editor",
	    "The LOSER Corps Map Editor",
	    "Copyright (C) 1998,1999 Ricardo R. Massaro", info);
  return D_O_K;
}


/***************************************************/



/* Toggle button.
 * d1 = button state (0 = up, 1 = down)
 * dp = button text
 * dp1 = ptr to function to call when the state has changed */
int toggle_button_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  int fore, back, tmp;

  switch (msg) {
  case MSG_INIT:
    win_setborderwidth(win, 0);
    win_setbackground(win, xd_back_3d);
    break;

  case MSG_DRAW:
    if (dlg->d1 == 0) {
      fore = xd_light_3d;
      back = xd_shadow_3d;
      tmp = 0;
    } else {
      fore = xd_shadow_3d;
      back = xd_light_3d;
      tmp = 1;
    }
    win_setcolor(win, fore);
    win_line(win, 0, 0, dlg->w - 1, 0);
    win_line(win, 0, 0, 0, dlg->h - 1);
    win_setcolor(win, back);
    win_line(win, 0, dlg->h - 1, dlg->w - 1, dlg->h - 1);
    win_line(win, dlg->w - 1, 0, dlg->w - 1, dlg->h - 1);
    win_setcolor(win, dlg->fg);
    win_setbackcolor(win, xd_back_3d);
    if (dlg->dp != NULL)
      win_outtextxy(win,
		    tmp + (dlg->w - win_textwidth(win, dlg->dp)) / 2,
		    tmp + (dlg->h - win_textheight(win, dlg->dp)) / 2,
		    dlg->dp);
    break;

  case MSG_CLICK:
    tmp = dlg->d1;
    XSync(win->disp, 0);
    win_grab_pointer(win, 0, None);
    while (win_read_mouse(win, NULL, NULL) != 0) {
      if (point_in_rect(win->mouse_x, win->mouse_y, 0, 0, dlg->w, dlg->h)) {
	if (dlg->d1 == 0) {
	  dlg->d1 = 1;
	  SEND_MESSAGE(MSG_DRAW, dlg, 0);
	}
      } else
	if (dlg->d1 == 1) {
	  dlg->d1 = 0;
	  SEND_MESSAGE(MSG_DRAW, dlg, 0);
	}
    }
    win_ungrab_pointer(win);
    if (point_in_rect(win->mouse_x, win->mouse_y, 0, 0, dlg->w, dlg->h)) {
      dlg->d1 = ! tmp;
      if (dlg->dp1 != NULL)
	((void (*) (WINDOW *, DIALOG *)) dlg->dp1)(win, dlg);
    } else
      dlg->d1 = tmp;
    SEND_MESSAGE(MSG_DRAW, dlg, 0);
    break;
  }

  return D_O_K;
}


/* Update the "Tile: (x, y)" indicator */
void update_tile_txt(WINDOW *win, DIALOG *dlg, int use_mouse, int force)
{
  static int old_tile_x, old_tile_y;

  if (use_mouse && state.n_bmps > 0) {
    int tile_x, tile_y;

    tile_x = (win->mouse_x / state.pixel_size + state.x) / state.bmp[0]->w;
    tile_y = (win->mouse_y / state.pixel_size + state.y) / state.bmp[0]->h;
    if (force || tile_x != old_tile_x || tile_y != old_tile_y) {
      sprintf(maindlg_tile_txt, "Map (%05d, %05d): ", tile_x, tile_y);

      if (state.map != NULL && tile_x >= 0 && tile_y >= 0
	  && tile_x < state.map->w && tile_y < state.map->h) {
	if (state.map->line[tile_y][tile_x].fore >= 0)
	  sprintf(maindlg_tile_txt + strlen(maindlg_tile_txt),
		  "(fg = %04d, ", state.map->line[tile_y][tile_x].fore);
	else
	  sprintf(maindlg_tile_txt + strlen(maindlg_tile_txt), "(fg = none, ");
	if (state.map->line[tile_y][tile_x].back >= 0)
	  sprintf(maindlg_tile_txt + strlen(maindlg_tile_txt),
		  "bg = %04d)", state.map->line[tile_y][tile_x].back);
	else
	  sprintf(maindlg_tile_txt + strlen(maindlg_tile_txt), "bg = none)");
      } else
	strcat(maindlg_tile_txt, "(no map)");
      SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_TILE_TXT, 0);
      old_tile_x = tile_x;
      old_tile_y = tile_y;
    }
  } else {
    if (! force && old_tile_x == -1 && old_tile_y == -1)
      return;
    old_tile_x = old_tile_y = -1;
    maindlg_tile_txt[0] = '\0';
    SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_TILE_TXT, 0);
  }
}

static void map_set_tile(WINDOW *win, DIALOG *dlg, int shift, int control)
{
  int tile_x, tile_y, old_tile_x, old_tile_y;
  int pos_x, pos_y, old_pos_x, old_pos_y;
  int sel = -1;

  if (state.n_bmps <= 0 || state.map == NULL)
    return;

  pos_x = win->mouse_x / state.pixel_size + state.x;
  pos_y = win->mouse_y / state.pixel_size + state.y;
  tile_x = pos_x / state.bmp[0]->w;
  tile_y = pos_y / state.bmp[0]->h;

  if (shift) {
    if (win->mouse_b & MOUSE_R) {     /* Paste */
      paste_map_sel(tile_x, tile_y);
      update_tile_txt(win, dlg, 1, 1);
      SEND_MESSAGE(MSG_DRAW, dlg, 0);
      return;
    } else {                          /* Unselect */
      int x, y, stop;

      stop = (state.map->line[tile_y][tile_x].sel) ? 1 : 0;
      
      for (y = 0; y < state.map->h; y++)
	for (x = 0; x < state.map->w; x++)
	  state.map->line[y][x].sel = 0;

      if (stop) {
	SEND_MESSAGE(MSG_DRAW, dlg, 0);
	return;
      }
    }
  }

  old_tile_x = old_tile_y = old_pos_x = old_pos_y = -1;
  win_grab_pointer(win, 1, None);
  do {
    int alt = 0;

    {
      Window a, b;
      int x1, y1, x2, y2;
      unsigned int mask;
      XQueryPointer(win->disp, win->window, &a, &b, &x1, &y1, &x2, &y2, &mask);
      alt = (mask & (Mod1Mask|Mod2Mask)) ? 1 : 0;
    }

    pos_x = win->mouse_x / state.pixel_size + state.x;
    pos_y = win->mouse_y / state.pixel_size + state.y;
    tile_x = pos_x / state.bmp[0]->w;
    tile_y = pos_y / state.bmp[0]->h;

    if (state.tool == TOOL_OBJECTS) {
      if (old_pos_x == pos_x && old_pos_y == pos_y)
	continue;
    } else {
      if (old_tile_x == tile_x && old_tile_y == tile_y)
	continue;
    }

    old_pos_x = pos_x;
    old_pos_y = pos_y;
    old_tile_x = tile_x;
    old_tile_y = tile_y;
    if (shift)
      set_tile_sel(tile_x, tile_y, 1);
    else {
      switch (state.tool) {
      case TOOL_TILES:
	set_map_tile(tile_x, tile_y,
		     (state.cur_layer == 0) ? ML_FORE : ML_BACK,
		     (win->mouse_b & MOUSE_R) ? -1 : state.cur_tile, 1);
	break;

      case TOOL_BLOCKS:
	set_map_tile(tile_x, tile_y, ML_BLOCK,
		     (win->mouse_b & MOUSE_R) ? -1 : state.cur_block, 1);
	break;

      case TOOL_OBJECTS:
	sel = set_map_object(win, pos_x, pos_y,
			     (win->mouse_b & MOUSE_R) ? 1 : 0,
			     shift, control, alt, sel);
	break;
      }
      update_tile_txt(win, dlg, 1, 1);
    }
    SEND_MESSAGE(MSG_DRAW, dlg, 0);
  } while (win_read_mouse(win, NULL, NULL) & (MOUSE_L | MOUSE_R));
  win_ungrab_pointer(win);
}

/* Set the tile scroll state */
void set_tile_scroll_state(int start_tile, int n_tiles)
{
  main_dlg[MAINDLG_TILE_SCROLL].d1 = start_tile;
  main_dlg[MAINDLG_TILE_SCROLL].d2 = (n_tiles
				      - (main_dlg[MAINDLG_TILE_IMG].w
					 / main_dlg[MAINDLG_TILE_IMG].h)
				      + 2);
}

/* Adjust the state position so that the
 * visible area will not be outside the map */
void clip_state_pos(STATE *s, DIALOG *dlg)
{
  if (s->n_bmps <= 0 || s->map == NULL)
    return;

  if (s->x < 0)
    s->x = 0;
  if (s->y < 0)
    s->y = 0;
  if ((s->x + dlg->w / s->pixel_size) / state.bmp[0]->w > s->map->w - 1)
    s->x = s->map->w * s->bmp[0]->w - dlg->w / s->pixel_size;
  if ((s->y + dlg->h / s->pixel_size) / s->bmp[0]->h > s->map->h - 1)
    s->y = s->map->h * s->bmp[0]->h - dlg->h / s->pixel_size;
}

static void map_scroll(WINDOW *win, DIALOG *dlg)
{
  int delta_x, delta_y;

  if (state.n_bmps <= 0 || state.map == NULL)
    return;

  win_grab_pointer(win, 1, None);
  while (win_read_mouse(win, NULL, NULL) != 0) {
    delta_x = (win->mouse_x - dlg->w / 2);
    delta_y = (win->mouse_y - dlg->h / 2);

    delta_x *= ABS(delta_x);
    delta_y *= ABS(delta_y);
    delta_x /= 600;
    delta_y /= 300;

    if (delta_x == 0 && delta_y == 0)
      continue;
    state.x += delta_x;
    state.y += delta_y;
    clip_state_pos(&state, dlg);
    update_tile_txt(win, dlg, 1, 0);
    SEND_MESSAGE(MSG_DRAW, dlg, 0);
  }
  win_ungrab_pointer(win);
}

/* The map display control */
int map_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  switch (msg) {
  case MSG_INIT:
    win_setborderwidth(win, 0);
    dlg->dp = (void *) create_image(win->disp, MAPDLG_MAX_W + 2 * DISP_X,
				    MAPDLG_MAX_H + 2 * DISP_Y, SCREEN_BPP);
    if (dlg->dp == NULL) {
      printf("Error: can't create image\n");
      exit(1);
    }
    if (state.n_bmps <= 0)
      break;
    display_map(win, dlg->w, dlg->h, (IMAGE *) dlg->dp, &state);
    break;

  case MSG_DRAW:
    if (state.n_bmps > 0 && c == 0) {
      display_map(win, dlg->w, dlg->h, (IMAGE *) dlg->dp, &state);
      SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_MAP_MAP, 0);
    }
    display_image(win, 0, 0, (IMAGE *) dlg->dp, state.pixel_size * DISP_X,
		  state.pixel_size * DISP_Y, dlg->w, dlg->h);
    XSync(win->disp, 0);
    break;

  case MSG_CLICK:
    if (win->mouse_b & (MOUSE_L | MOUSE_R))
      map_set_tile(win, dlg, (c & ShiftMask) ? 1 : 0,
		   (c & ControlMask) ? 1 : 0);
    else if (win->mouse_b & MOUSE_M)
      map_scroll(win, dlg);
    break;

  case MSG_MOUSEMOVE:
    update_tile_txt(win, dlg, 1, 0);
    break;

  case MSG_WANTFOCUS:
    return D_WANTFOCUS;

  case MSG_KEY:
    if (state.map != NULL && state.n_bmps > 0) {
      int tile_x, tile_y, redraw = 1;
      int tile_w, tile_h;

      tile_w = state.bmp[0]->w;
      tile_h = state.bmp[0]->h;
      tile_x = (win->mouse_x / state.pixel_size + state.x) / tile_w;
      tile_y = (win->mouse_y / state.pixel_size + state.y) / tile_h;

      switch (KEYSYM(c)) {
      case XK_c:
	if (state.tool == TOOL_OBJECTS) {
	  MAP_OBJECT *obj, *new_obj;

	  obj = get_object(state.map->obj, state.selected_obj);
	  if (obj != NULL) {
	    new_obj = copy_map_object(&state, obj);
	    if (new_obj != NULL)
	      state.selected_obj = object_index(state.map->obj, new_obj);
	  }
	} else
	  redraw = 0;
	break;

      case XK_space:
	if (state.tool == TOOL_OBJECTS) {
	  int x, y;

	  x = win->mouse_x / state.pixel_size + state.x;
	  y = win->mouse_y / state.pixel_size + state.y;
	  state.selected_obj = get_map_object_at(state.map->obj, x, y);
	}
	break;

      case XK_BackSpace:
      case XK_Delete:
	if (state.tool == TOOL_OBJECTS) {
	  MAP_OBJECT *obj;

	  obj = get_object(state.map->obj, state.selected_obj);
	  if (obj != NULL) {
	    delete_map_object(obj);
	    state.selected_obj = -1;
	  } else
	    XBell(win->disp, 0);
	} else if (KEYMOD(c) & (ShiftMask | ControlMask | Mod1Mask))
	  if (map_can_undo())
	    map_undo();
	  else
	    XBell(win->disp, 0);
	else {
	  win_read_mouse(win, NULL, NULL);
	  set_map_tile(tile_x, tile_y,
		       (state.cur_layer == 0) ? ML_FORE : ML_BACK, -1, 1);
	}
	redraw = 1;
	break;

      case XK_Up: state.y -= tile_h; clip_state_pos(&state, dlg); break;
      case XK_Down: state.y += tile_h; clip_state_pos(&state, dlg); break;
      case XK_Left: state.x -= tile_w; clip_state_pos(&state, dlg); break;
      case XK_Right: state.x += tile_w; clip_state_pos(&state, dlg); break;

      default: redraw = 0;
      }
      if (redraw) {
	update_tile_txt(win, dlg, 1, 0);
	SEND_MESSAGE(MSG_DRAW, dlg, 0);
      }
    }
    return D_USED_CHAR;

  case MSG_END:
    destroy_image(win->disp, (IMAGE *) dlg->dp);
    break;
  }

  return D_O_K;
}


static void block_8(unsigned char *start, int w, int h, int line_w)
{
  int x, y;
  unsigned char *p;

  *start = 0;
  for (y = 0; y < h; y++) {
    p = start + line_w * y;
    for (x = 0; x < w; x++)
      *p++ = MAKECOLOR8(0,0,0);
  }
}

static void block_16(unsigned short *start, int w, int h, int line_w)
{
  int x, y;
  unsigned short *p;

  *start = 0;
  for (y = 0; y < h; y++) {
    p = start + line_w * y;
    for (x = 0; x < w; x++)
      *p++ = 0;
  }
}

static void draw_map_map(WINDOW *win, XImage *img,
			 int x1, int y1, int x2, int y2)
{
  int x, y;
  int img_x, img_y;


  if (img != NULL) {
    if (SCREEN_BPP == 1) {
      unsigned char *p;

      memset(img->data, 7, img->height * img->bytes_per_line);
      for (y = 0; y < state.map->h; y++)
	for (x = 0; x < state.map->w; x++) {
	  if (state.map->line[y][x].block >= 0
	      && state.map->line[y][x].block < 16) {
	    img_x = (x * img->width) / state.map->w;
	    img_y = (y * img->height) / state.map->h;
	    p = (((unsigned char *) (img->data + img->bytes_per_line * img_y))
		 + img_x);
	    block_8(p,
		    (((x+1) * img->width) / state.map->w
		     - (x*img->width) / state.map->w),
		    (((y+1) * img->height) / state.map->h
		     - (y * img->height) / state.map->h),
		    img->bytes_per_line);
	  }
	}
    } else {
      unsigned short *p;

      p = (unsigned short *) img->data;
      for (y = img->height * img->width; y > 0; y--)
	*p++ = MAKECOLOR16(160,160,160);

      for (y = 0; y < state.map->h; y++)
	for (x = 0; x < state.map->w; x++) {
	  if (state.map->line[y][x].block >= 0
	      && state.map->line[y][x].block < 15) {
	    img_x = (x * img->width) / state.map->w;
	    img_y = (y * img->height) / state.map->h;
	    p = (((unsigned short *) (img->data + img->bytes_per_line * img_y))
		 + img_x);
	    block_16(p,
		     (((x+1) * img->width) / state.map->w
		      - (x*img->width) / state.map->w),
		     (((y+1) * img->height) / state.map->h
		      - (y * img->height) / state.map->h),
		     img->bytes_per_line >> 1);
	  }
	}
    }

    win_setdrawmode(win, DM_COPY);
    x_put_image(win->disp, win->window, win->gc,
                img, 0, 0, 0, 0, img->width, img->height);
  }

  if (SCREEN_BPP == 1) {
    win_setcolor(win, /* COLOR_WHITE */ MAKECOLOR8(255,255,255));
    win_setdrawmode(win, DM_XOR);
  } else {
    win_setcolor(win, COLOR_BLACK);
    win_setdrawmode(win, DM_INVERT);
  }
  win_rectfill(win, x1, y1, x2, y2);
}

/* The map map control */
int map_map_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  switch (msg) {
  case MSG_INIT:
    {
      XImage *img;
      Screen *screen;

      screen = DefaultScreenOfDisplay(win->disp);
      img = XCreateImage(win->disp, DefaultVisualOfScreen(screen),
			 8*SCREEN_BPP, ZPixmap, 0, NULL, dlg->w, dlg->h,
			 8*SCREEN_BPP, 0);
      if (img == NULL) {
	dlg->dp = NULL;
	break;
      }
      img->data = (char *) malloc(img->height * img->bytes_per_line);
      if (img->data == NULL) {
	free(img);
	dlg->dp = NULL;
	break;
      }
      memset(img->data, 0, img->height * img->bytes_per_line);
      dlg->dp = (void *) img;
      win_setborderwidth(win, 1);
      win_setbackground(win, xd_back_3d);
      break;
    }

  case MSG_DRAW:
    if (c == 0 && dlg->dp == NULL) {
      win_setcolor(win, xd_back_3d);
      win_rectfill(win, 0, 0, dlg->w, dlg->h);
    }
    if (state.map != NULL && state.map->w > 0 && state.map->h > 0
	&& state.n_bmps > 0) {
      int x1, y1, x2, y2;
      int map_w, map_h;

      map_w = main_dlg[MAINDLG_MAP].w / state.pixel_size;
      map_h = main_dlg[MAINDLG_MAP].h / state.pixel_size;
      x1 = (dlg->w * state.x / state.bmp[0]->w) / state.map->w;
      y1 = (dlg->h * state.y / state.bmp[0]->h) / state.map->h;
      x2 = (dlg->w * (state.x + map_w) / state.bmp[0]->w) / state.map->w;
      y2 = (dlg->h * (state.y + map_h) / state.bmp[0]->h) / state.map->h;

      draw_map_map(win, dlg->dp, x1, y1, x2, y2);
    }
    break;
  }
  return D_O_K;
}


static void draw_rect_image(XImage *img, int x, int y, int w, int h, int color)
{
  if (SCREEN_BPP == 1) {
    memset(img->data + y * img->bytes_per_line + x*SCREEN_BPP,
	   color, w*SCREEN_BPP);
    memset(img->data + (y + 1) * img->bytes_per_line + x*SCREEN_BPP,
	   2, w*SCREEN_BPP);
    memset(img->data + (y + 2) * img->bytes_per_line + x*SCREEN_BPP,
	   color, w*SCREEN_BPP);
    memset(img->data + (y + h - 3) * img->bytes_per_line + x*SCREEN_BPP,
	   color, w*SCREEN_BPP);
    memset(img->data + (y + h - 2) * img->bytes_per_line + x*SCREEN_BPP,
	   2, w*SCREEN_BPP);
    memset(img->data + (y + h - 1) * img->bytes_per_line + x*SCREEN_BPP,
	   color, w*SCREEN_BPP);
  } else {
    unsigned short *p;
    int i;

    p = ((unsigned short *)
	 (img->data + y * img->bytes_per_line + x*SCREEN_BPP));
    for (i = w; i > 0; i--)
      *p++ = color;
    p = ((unsigned short *)
	 (img->data + (y+1) * img->bytes_per_line + x*SCREEN_BPP));
    for (i = w; i > 0; i--)
      *p++ = MAKECOLOR16(0,196,0);
    p = ((unsigned short *)
	 (img->data + (y+2) * img->bytes_per_line + x*SCREEN_BPP));
    for (i = w; i > 0; i--)
      *p++ = color;


    p = ((unsigned short *)
	 (img->data + (h+y-3) * img->bytes_per_line + x*SCREEN_BPP));
    for (i = w; i > 0; i--)
      *p++ = color;
    p = ((unsigned short *)
	 (img->data + (h+y-2) * img->bytes_per_line + x*SCREEN_BPP));
    for (i = w; i > 0; i--)
      *p++ = MAKECOLOR16(0,196,0);
    p = ((unsigned short *)
	 (img->data + (h+y-1) * img->bytes_per_line + x*SCREEN_BPP));
    for (i = w; i > 0; i--)
      *p++ = color;
  }
}

static void copy_null_image(XImage *img, int x, int y, int w, int h,
			    int color)
{
  int i;

  for (i = 0; i < h; i++) {
    if (SCREEN_BPP == 1)
      memset(img->data + (y + i) * img->bytes_per_line + x, color, w);
    else {
      unsigned short *p;
      int j;

      p = ((unsigned short *)
	   (img->data + (y + i) * img->bytes_per_line + x*SCREEN_BPP));
      for (j = w; j > 0; j--)
	*p++ = color;
    }
  }
}

static void copy_bmp_image(XImage *img, int x, int y, XBITMAP *bmp)
{
  int i, n_cols;

  n_cols = MIN(bmp->w, img->width);

  if (SCREEN_BPP == 1) {
    for (i = 0; i < bmp->h; i++)
#if 0
      memcpy(img->data + (y + i) * img->bytes_per_line + x,
	     bmp->line[i], n_cols);
#else
    {
      int j;
      for (j = 0; j < bmp->w; j++)
	if (bmp->line[i][j] != XBMP8_TRANSP)
	  *(img->data + (y+i) * img->bytes_per_line + x+j) = bmp->line[i][j];
	else
	  *(img->data + (y+i) * img->bytes_per_line + x+j) =
	    MAKECOLOR8(ABS((n_cols/2 - j) * (bmp->h/2 - i)) % 256, 0, 0);
    }
#endif
  } else {
    unsigned short *src, *dest;
    int j;

    for (i = 0; i < bmp->h; i++) {
      src = (unsigned short *) bmp->line[i];
      dest = ((unsigned short *)
	      (img->data + (y + i) * img->bytes_per_line + x * SCREEN_BPP));
      for (j = n_cols; j > 0; j--) {
	if (*src == XBMP16_TRANSP)
	  *dest++ = MAKECOLOR16(ABS((n_cols/2 - j) * (bmp->h/2 - i)) % 256,
				0, 0);
	else
	  *dest++ = *src;
	src++;
      }
    }
  }
}

/* Copy a bitmap to an image */
static void copy_image(XImage *img, XBITMAP **bmp, int max,
		       int start, int cur, int back_color)
{
  int n;

  cur++;
  for (n = 0; n < img->width / bmp[0]->w; n++) {
    if (n + start <= 0)
      copy_null_image(img, n * bmp[0]->w, 0, bmp[0]->w, bmp[0]->h,
		      back_color);
    else
      copy_bmp_image(img, n * bmp[0]->w, 0, bmp[(n + start - 1) % max]);

    if (cur == n + start)
      draw_rect_image(img, n * bmp[0]->w, 0, bmp[0]->w, bmp[0]->h,
		      ((SCREEN_BPP==1)
		       ? MAKECOLOR8(0,255,0)
		       : MAKECOLOR16(0,255,0)));
  }
}


void update_tile_images(void)
{
  XImage *img;

  if ((img = (XImage *) (main_dlg + MAINDLG_TILE_IMG)->dp) == NULL)
    return;

  switch (state.tool) {
  case TOOL_TILES:
    copy_image(img, state.bmp, state.n_bmps,
	       state.start_tile, state.cur_tile,
	       (SCREEN_BPP == 1) ? xd_back_3d : MAKECOLOR16(128,128,128));
    break;

  case TOOL_BLOCKS:
    copy_image(img, state.block_bmp, state.n_block_bmps,
	       state.start_block, state.cur_block,
	       (SCREEN_BPP == 1) ? xd_back_3d : MAKECOLOR16(128,128,128));
    break;

  case TOOL_OBJECTS:
    copy_image(img, state.block_bmp, 1, -256, -2,
	       (SCREEN_BPP == 1) ? xd_back_3d : MAKECOLOR16(128,128,128));
    break;
  }
}

int ximage_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  switch (msg) {
  case MSG_INIT:
    {
      XImage *img;
      Screen *screen;

      screen = DefaultScreenOfDisplay(win->disp);
      img = XCreateImage(win->disp, DefaultVisualOfScreen(screen),
			 8*SCREEN_BPP, ZPixmap, 0, NULL, dlg->w, dlg->h,
			 8*SCREEN_BPP, 0);
      if (img == NULL) {
	dlg->dp = NULL;
	break;
      }
      img->data = (char *) malloc(img->height * img->bytes_per_line);
      if (img->data == NULL) {
	free(img);
	dlg->dp = NULL;
	break;
      }
      memset(img->data, 0, img->height * img->bytes_per_line);
      dlg->dp = (void *) img;
      break;
    }

  case MSG_CLICK:
    if (state.bmp[0] != NULL) {
      int n = -2, old_n, roll;
      int max_tiles;

      if (state.tool == TOOL_TILES)
	max_tiles = state.n_bmps;
      else
	max_tiles = state.n_block_bmps;

      win_grab_pointer(win, 0, None);
      do {
	old_n = n;
	if (win->mouse_x < 0)
	  n = -1;
	else if (win->mouse_x > dlg->w)
	  n = dlg->w / state.bmp[0]->w;
	else
	  n = win->mouse_x / state.bmp[0]->w;
	n += main_dlg[MAINDLG_TILE_SCROLL].d1;
	if (n < 0)
	  n = 0;
	else if (n > max_tiles)
	  n = max_tiles;

	roll = 0;
	if (win->mouse_x < 0) {
	  if (main_dlg[MAINDLG_TILE_SCROLL].d1 > 0)
	    roll = -1;
	} else if (win->mouse_x >= dlg->w) {
	  if (main_dlg[MAINDLG_TILE_SCROLL].d1
	      < (max_tiles
		 - (main_dlg[MAINDLG_TILE_IMG].w
		    / main_dlg[MAINDLG_TILE_IMG].h)
		 + 1))
	    roll = 1;
	}

	if (roll != 0) {
	  main_dlg[MAINDLG_TILE_SCROLL].d1 += roll;
	  SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_TILE_SCROLL, 0);
	  if (main_dlg[MAINDLG_TILE_SCROLL].dp != NULL)
	    ((void (*)(DIALOG *))main_dlg[MAINDLG_TILE_SCROLL].dp)
	      (main_dlg + MAINDLG_TILE_SCROLL);
	}

	if (n != old_n) {
	  old_n = n;
	  if (state.tool == TOOL_TILES)
	    state.cur_tile = n - 1;
	  else
	    state.cur_block = n - 1;
	  update_tile_images();
	  SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_TILE_IMG, 0);
	}
	if (roll)
	  usleep(25000);
      } while (win_read_mouse(win, NULL, NULL) != 0);
      win_ungrab_pointer(win);
    }
    break;

  case MSG_DRAW:
    if (dlg->dp != NULL) {
      x_put_image(win->disp, win->window, win->gc,
                  (XImage *) dlg->dp, 0, 0, 0, 0,
                  ((XImage *) dlg->dp)->width, ((XImage *) dlg->dp)->height);
    }
    break;

  case MSG_END:
    if (dlg->dp != NULL)
      XDestroyImage((XImage *) dlg->dp);
    break;
  }
  return D_O_K;
}

int radio_layer_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  switch (msg) {
  case MSG_INIT:
    win_setborderwidth(win, 1);
    break;

  case MSG_DRAW:
    if (dlg->d1) {
      if (SCREEN_BPP == 1)
	win_setcolor(win, MAKECOLOR8(0,0,255));
      else
	win_setcolor(win, COLOR_BLUE);
    } else
      win_setcolor(win, xd_back_3d);
    win_rectfill(win, 0, 0, dlg->w - 1, dlg->h - 1);
    break;

  case MSG_CLICK:
    if (dlg->dp != NULL)
      ((void (*) (WINDOW *, DIALOG *)) dlg->dp)(win, dlg);
  }
  return D_O_K;
}


#if 0
static char *get_tool_text(int tool)
{
  switch (tool) {
  case TOOL_TILES: return "tiles";
  case TOOL_BLOCKS: return "blocks";
  case TOOL_OBJECTS: return "objects";
  default: return "*invalid*";
  }
}
#endif /* 0 */

int tool_sel_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  int ret;

  ret = popup_proc(msg, win, dlg, c);

  if (msg == MSG_CLICK) {
    state.tool = dlg->d1;

    if (state.tool == TOOL_TILES)
      set_tile_scroll_state(state.start_tile, state.n_bmps);
    else
      set_tile_scroll_state(state.start_block, state.n_block_bmps);

    image_change(main_dlg + MAINDLG_TILE_SCROLL);
    SEND_MESSAGE(MSG_DRAW, dlg, 0);
    SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_TILE_IMG, 0);
    SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_TILE_SCROLL, 0);
  }
  return ret;
}

int popup_change_proc(WINDOW *win, int id)
{
  win->dialog->d1 = id;
  return 0;
}

int popup_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  MENU menu[256];
  int i;

  if (msg != MSG_CLICK) {
    int ret, d1;
    void *dp;
    char txt[1024];

    dp = dlg->dp;
    d1 = dlg->d1;
    dlg->dp = txt;
    dlg->d1 = 0;
    if (dlg->d1 >= 0)
      strcpy(txt, ((char **) dp)[d1]);
    else
      txt[0] = '\0';
    ret = text_proc(msg, win, dlg, c);
    dlg->d1 = d1;
    dlg->dp = dp;
    return ret;
  }

  if (win->mouse_b & MOUSE_L) {
    dlg->d1++;
    if (((char **)dlg->dp)[dlg->d1] == NULL)
      dlg->d1 = 0;
  } else {
    memset(menu, 0, sizeof(MENU) * 256);
    for (i = 0; ((char **) dlg->dp)[i] != NULL; i++) {
      menu[i].text = ((char **) dlg->dp)[i];
      menu[i].proc = popup_change_proc;
      menu[i].id = i;
    }
    menu[i].text = NULL;
    win_setfont(win, xd_fixed_font);
    do_menu(win, menu,
	    win->parent->x + win->x + 10, win->parent->y + win->y + 10);
    win_setfont(win, xd_default_font);
  }
  return D_REDRAW;
}


/******************************/
/*** About box ****************/

static char *about_program_info, *about_copyright_info, **about_user_info;
static int about_userinfo_size;

static char *get_gpl_info(int index, int *num)
{
  static char *gpl_info[] = {
    "",
    "This program  is  free  software;",
    "you can  redistribute  it  and/or",
    "modify it under  the terms of the",
    "GNU  General  Public  License  as",
    "published by  the  Free  Software",
    "Foundation;  either version 2  of",
    "the License,  or (at your option)",
    "any later version.",
    "",
    "This program  is  distributed  in",
    "the hope that it will be  useful,",
    "but WITHOUT ANY WARRANTY; without",
    "even  the   implied  warranty  of",
    "MERCHANTABILITY or  FITNESS FOR A",
    "PARTICULAR PURPOSE.  See  the GNU",
    "General Public License  for  more",
    "details.",
    "",
    "You should have received  a  copy",
    "of the GNU General Public License",
    "along with this program;  if not,",
    "write   to   the   Free  Software",
    "Foundation,  Inc.,  675 Mass Ave,",
    "Cambridge, MA 02139, USA.",
    "",
  };

  if (index < 0) {
    if (num != NULL)
      *num = 2 + sizeof(gpl_info) / sizeof(gpl_info[0]) + about_userinfo_size;
    return NULL;
  }
  if (index == 0)
    return about_program_info;
  if (index == 1)
    return about_copyright_info;
  if (index < 2 + sizeof(gpl_info) / sizeof(gpl_info[0]))
    return gpl_info[index - 2];
  return about_user_info[index - 2 - sizeof(gpl_info) / sizeof(gpl_info[0])];
}

static DIALOG about_dlg[] = {
  { CTL_WINDOW, window_proc, 0,   0,   400, 300, 0, COLOR_WHITE,
    D_NORESIZE,         0, 0, NULL },
  { CTL_TEXT,   text_proc,   10,  10,  380, 30,  0, COLOR_WHITE,
    0,                  1, 0, NULL },
  { CTL_LIST,   list_proc,   10,  50,  380, 200, 0, COLOR_WHITE,
    0,                  0, 0, get_gpl_info },
  { CTL_BUTTON, button_proc, 150, 260, 100, 30,  0, COLOR_WHITE,
    D_GOTFOCUS|D_EXIT,  0, 0, "OK" },
  { CTL_NONE, NULL },
};

void about_box(WINDOW *parent, char *title,
	       char *prog_name, char *prog_info, char *copyright, char **info)
{
  about_dlg[0].dp = title;
  about_dlg[1].dp = prog_name;
  about_dlg[2].d1 = about_dlg[2].d2 = 0;  /* Scroll list up */
  about_program_info = prog_info;
  about_copyright_info = copyright;
  about_user_info = info;
  if (about_user_info == NULL)
    about_userinfo_size = 0;
  else {
    int i;

    for (i = 0; info[i] != NULL; i++)
      ;
    about_userinfo_size = i;
  }

  set_dialog_flag(about_dlg, D_3D, 1);
  centre_dialog(about_dlg, parent);
  prepare_dialog_colors(about_dlg);
  do_dialog_window(parent, about_dlg, 1);
}
