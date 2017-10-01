/* dialogs.c */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mapedit.h"
#include "dialogs.h"
#include "procs.h"
#include "map.h"


MENU file_menu[] = {
  { "New", menu_new, NULL },
  { "Open...", menu_open, NULL },
  { "Save", menu_save, NULL },
  { "Save as...", menu_save_as, NULL },
  { "", NULL, NULL },
  { "Read tiles...", menu_read_tiles, NULL },
  { "", NULL, NULL },
  { "Exit", menu_exit, NULL },
  { NULL },
};

MENU edit_menu[] = {
  { "Undo",           menu_undo, NULL },
  { "Change size...", menu_setsize, NULL },
  { NULL },
};

MENU configure_menu[] = {
  { "Map info...", menu_info, NULL },
  { "Map parameters...", menu_parms, NULL },
  { NULL },
};

MENU info_menu[] = {
  { "About...", menu_about, NULL },
  { NULL },
};

MENU main_menu[] = {
  { "File", NULL, file_menu },
  { "Edit", NULL, edit_menu },
  { "Configure", NULL, configure_menu },
  { "?", NULL, info_menu },
  { NULL },
};




/***************************************/
/*** Object dialog *********************/

char objdlg_title_txt[256];
MENU *objdlg_npc_menu;
char objdlg_x_txt[16];
char objdlg_y_txt[16];
char objdlg_dir_txt[16];
char objdlg_respawn_txt[16];
char objdlg_level_txt[16];
char objdlg_duration_txt[16];
char objdlg_vulnerability_txt[16];
char objdlg_text_txt[ITEM_TEXT_LEN];
char objdlg_target_number[16];
char *objdlg_target_txt[] = { "None", NULL, NULL };

static int obj_popup_change_proc(WINDOW *win, int id)
{
  win->dialog->d1 = id;
  return 0;
}

static MENU *build_npc_submenu(STATE *s, int *pi)
{
  MENU *menu;
  int i, num_ent = 0;

  i = *pi;

  while (npc_names[i].npc >= 0 && (i == *pi || npc_names[i].child))
    num_ent++, i++;
  menu = (MENU *) malloc(sizeof(MENU) * (num_ent + 1));
  memset(menu, 0, sizeof(MENU) * (num_ent + 1));

  i = *pi;
  num_ent = 0;
  while (npc_names[i].npc >= 0 && (i == *pi || npc_names[i].child)) {
    menu[num_ent].text = strdup(npc_names[i].name);
    menu[num_ent].proc = obj_popup_change_proc;
    menu[num_ent].id = i;
    menu[num_ent].child = NULL;
    num_ent++;
    i++;
  }
  menu[num_ent].text = NULL;
  *pi = i;
  return menu;
}

#if 0
static void print_menu(MENU *menu, int level)
{
  int i, j;

  for (i = 0; menu[i].text != NULL; i++) {
    for (j = 0; j < level; j++)
      printf("  ");
    printf("%s\n", menu[i].text);
    if (menu[i].child)
      print_menu(menu[i].child, level + 1);
  }
}
#endif

void build_npc_menu(STATE *s)
{
  int num_ent = 0, i;

  for (i = 0; npc_names[i].npc >= 0;) {
    num_ent++;
    if (npc_names[i+1].npc >= 0 && npc_names[i+1].child) {
      i++;
      while (npc_names[i].npc >= 0 && npc_names[i].child)
	i++;
    }
    else
      i++;
  }

  if (num_ent == 0) {
    objdlg_npc_menu = NULL;
    return;
  }
  objdlg_npc_menu = (MENU *) malloc(sizeof(MENU) * (num_ent + 2));
  memset(objdlg_npc_menu, 0, sizeof(MENU) * (num_ent + 2));
  num_ent = 0;
  for (i = 0; npc_names[i].npc >= 0;) {
    if (i > 0 && s->npc_info[npc_names[i-1].npc].type == NPC_TYPE_ENEMY
	&& s->npc_info[npc_names[i].npc].type == NPC_TYPE_ITEM) {
      objdlg_npc_menu[num_ent].text = strdup("");
      objdlg_npc_menu[num_ent].id = -1;
      objdlg_npc_menu[num_ent].child = NULL;
      num_ent++;
    }
    
    if (npc_names[i+1].npc && npc_names[i+1].child) {
      char *p, txt[256];

      strcpy(txt, npc_names[i].name);
      p = strchr(txt, '/');
      if (p == NULL)
	objdlg_npc_menu[num_ent].text = strdup(npc_names[i].name);
      else {
	int i;

	strcpy(p, "/...");
	for (i = 26 - strlen(txt); i >= 0; i--)
	  strcat(p, " ");
	strcat(p, ">>");
	objdlg_npc_menu[num_ent].text = strdup(txt);
      }
      objdlg_npc_menu[num_ent].proc = NULL;
      objdlg_npc_menu[num_ent].id = -1;
      objdlg_npc_menu[num_ent].child = build_npc_submenu(s, &i);
    } else {
      objdlg_npc_menu[num_ent].text = npc_names[i].name;
      objdlg_npc_menu[num_ent].proc = obj_popup_change_proc;
      objdlg_npc_menu[num_ent].id = i;
      objdlg_npc_menu[num_ent].child = NULL;
      i++;
    }
    
    num_ent++;
  }
  objdlg_npc_menu[num_ent].text = NULL;
}

/* Look for the menu item with `id' in the given `menu' tree */ 
static int get_menu_text(char *txt, MENU *menu, int id)
{
  int i;

  for (i = 0; menu[i].text != NULL; i++) {
    if (menu[i].id == id) {
      strcpy(txt, menu[i].text);
      return 1;
    }
    if (menu[i].child != NULL && get_menu_text(txt, menu[i].child, id))
      return 1;
  }
  return 0;
}

static int obj_popup_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  MENU *menu;

  if (msg != MSG_CLICK) {
    int ret, d1;
    void *dp;
    char txt[1024];

    dp = dlg->dp;
    d1 = dlg->d1;
    dlg->dp = txt;
    dlg->d1 = 0;
    if (! get_menu_text(txt, *(MENU **) dp, d1))
      txt[0] = '\0';
    ret = text_proc(msg, win, dlg, c);
    dlg->d1 = d1;
    dlg->dp = dp;
    return ret;
  }

  menu = *(MENU **) dlg->dp;
  win_setfont(win, xd_fixed_font);
  do_menu(win, menu,
	  win->parent->x + win->x + 10, win->parent->y + win->y + 10);
  win_setfont(win, xd_default_font);
  return D_REDRAW;
}


DIALOG obj_dialog[] = {
  { CTL_WINDOW,  window_proc,        0,   0,    400, 370, 0, 0,
    D_NORESIZE, 0, 0, objdlg_title_txt },

  { CTL_TEXT,    text_proc,          10,  20,   150, 20, 0, 0,
    0, 2, 0, "NPC:" },
  { CTL_EDIT,    obj_popup_proc,     170, 20,   170, 20,  0, COLOR_WHITE,
    0, 0, 0, &objdlg_npc_menu },

  { CTL_TEXT,    text_proc,          10,  50,   150, 20, 0, 0,
    0, 2, 0, "X:" },
  { CTL_EDIT,    edit_proc,          170, 50,   170, 20,  0, COLOR_WHITE,
    0, 0, 15, objdlg_x_txt },

  { CTL_TEXT,    text_proc,          10,  80,   150, 20, 0, 0,
    0, 2, 0, "Y:" },
  { CTL_EDIT,    edit_proc,          170, 80,   170, 20,  0, COLOR_WHITE,
    0, 0, 15, objdlg_y_txt },

  { CTL_TEXT,    text_proc,          10,  110,  150, 20, 0, 0,
    0, 2, 0, "Respawn:" },
  { CTL_EDIT,    edit_proc,          170, 110,  170, 20,  0, COLOR_WHITE,
    0, 0, 15, objdlg_respawn_txt },

  { CTL_TEXT,    text_proc,          10,  140,  150, 20, 0, 0,
    0, 2, 0, "Direction:" },
  { CTL_EDIT,    edit_proc,          170, 140,  170, 20,  0, COLOR_WHITE,
    0, 0, 15, objdlg_dir_txt },

  { CTL_TEXT,    text_proc,          10,  170,  150, 20, 0, 0,
    0, 2, 0, "Level:" },
  { CTL_EDIT,    edit_proc,          170, 170,  170, 20,  0, COLOR_WHITE,
    0, 0, 15, objdlg_level_txt },

  { CTL_TEXT,    text_proc,          10,  200,  150, 20, 0, 0,
    0, 2, 0, "Duration:" },
  { CTL_EDIT,    edit_proc,          170, 200,  170, 20,  0, COLOR_WHITE,
    0, 0, 15, objdlg_duration_txt },

  { CTL_TEXT,    text_proc,          10,  230,  150, 20, 0, 0,
    0, 2, 0, "Target:" },
  { CTL_EDIT,    popup_proc,         170, 230,  170, 20,  0, COLOR_WHITE,
    0, 0, 0, objdlg_target_txt },
 
  { CTL_TEXT,    text_proc,          10,  260,  150, 20, 0, 0,
    0, 2, 0, "Vulnerability:" },
  { CTL_EDIT,    edit_proc,          170, 260,  170, 20,  0, COLOR_WHITE,
    0, 0, 15, objdlg_vulnerability_txt },

  { CTL_TEXT,    text_proc,          10,  290,  150, 20, 0, 0,
    0, 2, 0, "Text:" },
  { CTL_EDIT,    edit_proc,          170, 290,  170, 20,  0, COLOR_WHITE,
    0, 0, ITEM_TEXT_LEN-1, objdlg_text_txt },

  { CTL_BUTTON,   button_proc,       180, 330,  100, 30,  0, 0,
    D_EXIT, 0, 0, "OK" },
  { CTL_BUTTON,   button_proc,       290, 330,  100, 30,  0, 0,
    D_EXIT, 0, 0, "Cancel" },
  { CTL_NONE, NULL },
};

void set_object_attributes(WINDOW *win, MAP_OBJECT *obj, int id)
{
  /* Set initial values */
#if 0
  int i;

  for (i = 0; npc_names[i].npc >= 0; i++)
    objdlg_npc_list[i] = npc_names[i].name;
#endif
  obj_dialog[OBJDLG_NPC_LIST].d1 = get_npc_index(obj->npc);

  sprintf(objdlg_title_txt, "Object [%d] properties", id);
  sprintf(objdlg_x_txt, "%d", obj->x);
  sprintf(objdlg_y_txt, "%d", obj->y);
  sprintf(objdlg_respawn_txt, "%d", obj->respawn);
  sprintf(objdlg_dir_txt, "%d", obj->dir);
  sprintf(objdlg_level_txt, "%d", obj->level);
  sprintf(objdlg_duration_txt, "%d", obj->duration);
  sprintf(objdlg_vulnerability_txt, "%d", obj->vulnerability);
  sprintf(objdlg_text_txt, "%s", obj->text);

  if (obj->target < 0) {
    objdlg_target_txt[1] = NULL;
    obj_dialog[OBJDLG_TARGET_TEXT].d1 = 0;
  } else {
    objdlg_target_txt[1] = objdlg_target_number;
    sprintf(objdlg_target_number, "%d", obj->target);
    obj_dialog[OBJDLG_TARGET_TEXT].d1 = 1;
  }


  /* Show the dialog */
  centre_dialog(obj_dialog, win);
  set_dialog_flag(obj_dialog, D_GOTFOCUS, 0);
  set_dialog_flag(obj_dialog, D_3D, 1);
  prepare_dialog_colors(obj_dialog);
  if (do_dialog_window(win, obj_dialog, 1) != OBJDLG_BUT_OK)
    return;


  /* Read the values */
  if (obj_dialog[OBJDLG_NPC_LIST].d1 < 0)
    obj_dialog[OBJDLG_NPC_LIST].d1 = 0;
  obj->npc = npc_names[obj_dialog[OBJDLG_NPC_LIST].d1].npc;
  if (obj->npc < 0)
    obj->npc = 12;    /* FIXME: NPC_ITEM_ENERGY */
  obj->x = atoi(objdlg_x_txt);
  obj->y = atoi(objdlg_y_txt);
  obj->respawn = atoi(objdlg_respawn_txt);
  obj->dir = atoi(objdlg_dir_txt);
  obj->level = atoi(objdlg_level_txt);
  obj->duration = atoi(objdlg_duration_txt);
  obj->vulnerability = atoi(objdlg_vulnerability_txt);
  strcpy(obj->text, objdlg_text_txt);
  if (obj_dialog[OBJDLG_TARGET_TEXT].d1 == 0)
    obj->target = -1;
}


/***************************************/
/*** New dialog ************************/

char newdlg_width_txt[16];
char newdlg_height_txt[16];
char newdlg_fore_txt[16];
char newdlg_back_txt[16];

DIALOG new_dlg[] = {
  { CTL_WINDOW,   window_proc,        0,   0,   300, 200, 0, 0,
    D_NORESIZE, 0, 0, "New map" },

  { CTL_TEXT,     text_proc,          10,  20,  100, 20,  0, 0,
    0, 2, 0, "Width:" },
  { CTL_EDIT,     edit_proc,          120, 20,  170, 20,  0, COLOR_WHITE,
    0, 0, 15, newdlg_width_txt },
  { CTL_TEXT,     text_proc,          10,  50,  100, 20,  0, 0,
    0, 2, 0, "Height:" },
  { CTL_EDIT,     edit_proc,          120, 50,  170, 20,  0, COLOR_WHITE,
    0, 0, 15, newdlg_height_txt },

  { CTL_TEXT,     text_proc,          10,  80,  100, 20,  0, 0,
    0, 2, 0, "Foreground:" },
  { CTL_EDIT,     edit_proc,          120, 80,  170, 20,  0, COLOR_WHITE,
    0, 0, 15, newdlg_fore_txt },
  { CTL_TEXT,     text_proc,          10,  110, 100, 20,  0, 0,
    0, 2, 0, "Background:" },
  { CTL_EDIT,     edit_proc,          120, 110, 170, 20,  0, COLOR_WHITE,
    0, 0, 15, newdlg_back_txt },


  { CTL_BUTTON,   button_proc,        10,  160, 100, 30,  0, 0,
    D_EXIT, 0, 0, "OK" },
  { CTL_BUTTON,   button_proc,        120, 160, 100, 30,  0, 0,
    D_EXIT, 0, 0, "Cancel" },
  { CTL_NONE, NULL },
};



/***************************************/
/*** Size dialog ***********************/

char sizedlg_width_txt[16];
char sizedlg_height_txt[16];

DIALOG size_dlg[] = {
  { CTL_WINDOW,   window_proc,        0,   0,   300, 200, 0, 0,
    D_NORESIZE, 0, 0, "Set map size" },

  { CTL_TEXT,     text_proc,          10,  20,  100, 20,  0, 0,
    0, 2, 0, "Width:" },
  { CTL_EDIT,     edit_proc,          120, 20,  170, 20,  0, COLOR_WHITE,
    0, 0, 15, sizedlg_width_txt },
  { CTL_TEXT,     text_proc,          10,  50,  100, 20,  0, 0,
    0, 2, 0, "Height:" },
  { CTL_EDIT,     edit_proc,          120, 50,  170, 20,  0, COLOR_WHITE,
    0, 0, 15, sizedlg_height_txt },

  { CTL_BUTTON,   button_proc,        10,  160, 100, 30,  0, 0,
    D_EXIT, 0, 0, "OK" },
  { CTL_BUTTON,   button_proc,        120, 160, 100, 30,  0, 0,
    D_EXIT, 0, 0, "Cancel" },
  { CTL_NONE, NULL },
};



/***************************************/
/*** Map Info dialog *******************/

char infodlg_author_txt[256];
char infodlg_comment_txt[256];
char infodlg_tile_txt[256];

DIALOG info_dlg[] = {
  { CTL_WINDOW,   window_proc,        0,   0,   300, 170, 0, 0,
    D_NORESIZE, 0, 0, "Map Info" },

  { CTL_TEXT,     text_proc,          10,  20,  100, 20,  0, 0,
    0, 2, 0, "Author:" },
  { CTL_EDIT,     edit_proc,          120, 20,  170, 20,  0, COLOR_WHITE,
    0, 0, 255, infodlg_author_txt },
  { CTL_TEXT,     text_proc,          10,  50,  100, 20,  0, 0,
    0, 2, 0, "Comments:" },
  { CTL_EDIT,     edit_proc,          120, 50,  170, 20,  0, COLOR_WHITE,
    0, 0, 255, infodlg_comment_txt },
  { CTL_TEXT,     text_proc,          10,  80,  100, 20,  0, 0,
    0, 2, 0, "Tiles:" },
  { CTL_EDIT,     edit_proc,          120, 80,  170, 20,  0, COLOR_WHITE,
    0, 0, 255, infodlg_tile_txt },

  { CTL_BUTTON,   button_proc,        10,  130, 100, 30,  0, 0,
    D_EXIT, 0, 0, "OK" },
  { CTL_BUTTON,   button_proc,        120, 130, 100, 30,  0, 0,
    D_EXIT, 0, 0, "Cancel" },
  { CTL_NONE, NULL },
};


/***************************************/
/*** Map Parameters dialog *************/

char parmdlg_maxyspeed_txt[32];
char parmdlg_jumphold_txt[32];
char parmdlg_gravity_txt[32];

char parmdlg_maxxspeed_txt[32];
char parmdlg_accel_txt[32];
char parmdlg_jumpaccel_txt[32];
char parmdlg_attrict_txt[32];

char parmdlg_frameskip_txt[32];

extern DIALOG parm_dlg[];

static int parmdlg_default_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  int ret;

  ret = button_proc(msg, win, dlg, c);
  if (ret & D_CLOSE) {
    sprintf(parmdlg_maxyspeed_txt,  "%d", DEFAULT_MAX_Y_SPEED);
    sprintf(parmdlg_jumphold_txt,   "%d", DEFAULT_JUMP_HOLD);
    sprintf(parmdlg_gravity_txt,    "%d", DEFAULT_GRAVITY);
    sprintf(parmdlg_maxxspeed_txt,  "%d", DEFAULT_MAX_X_SPEED);
    sprintf(parmdlg_accel_txt,      "%d", DEFAULT_WALK_ACCEL);
    sprintf(parmdlg_jumpaccel_txt,  "%d", DEFAULT_JUMP_ACCEL);
    sprintf(parmdlg_attrict_txt,    "%d", DEFAULT_ATTRICT);
    sprintf(parmdlg_frameskip_txt,  "%d", DEFAULT_FRAME_SKIP);

    return D_REDRAW;
  }

  return ret;
}

DIALOG parm_dlg[] = {
  { CTL_WINDOW,   window_proc,        0,   0,   400, 280, 0, 0,
    D_NORESIZE, 0, 0, "Map Parameters" },

  { CTL_TEXT,     text_proc,          10,  20,  150, 20,  0, 0,
    0, 2, 0, "Max Y speed:" },
  { CTL_EDIT,     edit_proc,          170, 20,  170, 20,  0, COLOR_WHITE,
    0, 0, 31, parmdlg_maxyspeed_txt },
  { CTL_TEXT,     text_proc,          10,  45,  150, 20,  0, 0,
    0, 2, 0, "Jump hold:" },
  { CTL_EDIT,     edit_proc,          170, 45,  170, 20,  0, COLOR_WHITE,
    0, 0, 31, parmdlg_jumphold_txt },
  { CTL_TEXT,     text_proc,          10,  70,  150, 20,  0, 0,
    0, 2, 0, "Gravity:" },
  { CTL_EDIT,     edit_proc,          170, 70,  170, 20,  0, COLOR_WHITE,
    0, 0, 31, parmdlg_gravity_txt },
  { CTL_TEXT,     text_proc,          10,  95,  150, 20,  0, 0,
    0, 2, 0, "Max X speed:" },
  { CTL_EDIT,     edit_proc,          170, 95,  170, 20,  0, COLOR_WHITE,
    0, 0, 31, parmdlg_maxxspeed_txt },
  { CTL_TEXT,     text_proc,          10,  120, 150, 20,  0, 0,
    0, 2, 0, "Walk accelleration:" },
  { CTL_EDIT,     edit_proc,          170, 120, 170, 20,  0, COLOR_WHITE,
    0, 0, 31, parmdlg_accel_txt },
  { CTL_TEXT,     text_proc,          10,  145, 150, 20,  0, 0,
    0, 2, 0, "Jump accelleration:" },
  { CTL_EDIT,     edit_proc,          170, 145, 170, 20,  0, COLOR_WHITE,
    0, 0, 31, parmdlg_jumpaccel_txt },
  { CTL_TEXT,     text_proc,          10,  170, 150, 20,  0, 0,
    0, 2, 0, "Attrict:" },
  { CTL_EDIT,     edit_proc,          170, 170, 170, 20,  0, COLOR_WHITE,
    0, 0, 31, parmdlg_attrict_txt },
  { CTL_TEXT,     text_proc,          10,  200, 150, 20,  0, 0,
    0, 2, 0, "Frame skip:" },
  { CTL_EDIT,     edit_proc,          170, 200, 170, 20,  0, COLOR_WHITE,
    0, 0, 31, parmdlg_frameskip_txt },

  { CTL_BUTTON,   button_proc,          10,  240, 100, 30,  0, 0,
    D_EXIT, 0, 0, "OK" },
  { CTL_BUTTON,   button_proc,          120, 240, 100, 30,  0, 0,
    D_EXIT, 0, 0, "Cancel" },
  { CTL_BUTTON,   parmdlg_default_proc, 230, 240, 100, 30,  0, 0,
    D_EXIT, 0, 0, "Default" },
  { CTL_NONE, NULL },
};


/***************************************/
/*** Main dialog ***********************/

char maindlg_tile_txt[64];

static void cur_layer_change(WINDOW *win, DIALOG *dlg)
{
  if (dlg - main_dlg == MAINDLG_RADIOLAYER_FORE) {
    state.cur_layer = 0;
    main_dlg[MAINDLG_RADIOLAYER_FORE].d1 = 1;
    main_dlg[MAINDLG_RADIOLAYER_BACK].d1 = 0;
  } else {
    state.cur_layer = 1;
    main_dlg[MAINDLG_RADIOLAYER_FORE].d1 = 0;
    main_dlg[MAINDLG_RADIOLAYER_BACK].d1 = 1;
  }
  SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_RADIOLAYER_FORE, 0);
  SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_RADIOLAYER_BACK, 0);
}

static void layer_change(WINDOW *win, DIALOG *dlg)
{
  int flag = 0;

  switch (dlg - main_dlg) {
  case MAINDLG_LAYER_FORE: flag = LAYER_FOREGROUND; break;
  case MAINDLG_LAYER_BACK: flag = LAYER_BACKGROUND; break;
  case MAINDLG_LAYER_GRID: flag = LAYER_GRIDLINES; break;
  case MAINDLG_LAYER_BLOCK: flag = LAYER_BLOCKS; break;
  }
  if (dlg->d1)
    state.layers |= flag;
  else
    state.layers &= ~flag;
  SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_MAP, 0);
}

void image_change(DIALOG *dlg)
{
  int max;

  if (state.tool == TOOL_TILES)
    max = state.n_bmps;
  else
    max = state.n_block_bmps;

  if (dlg->d1 >= 0 && dlg->d1 <= max) {
    if (state.tool == TOOL_TILES)
      state.start_tile = dlg->d1;
    else
      state.start_block = dlg->d1;
    update_tile_images();
    SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_TILE_IMG, 0);
  }
}

static void double_change(WINDOW *win, DIALOG *dlg)
{
  if (dlg->d1)
    state.pixel_size = 2;
  else
    state.pixel_size = 1;
  clip_state_pos(&state, main_dlg + MAINDLG_MAP);
  SEND_MESSAGE(MSG_DRAW, main_dlg + MAINDLG_MAP, 0);
}


static char *maindlg_tool_txt[] = {
  "tiles",
  "blocks",
  "objects",
  NULL
};

DIALOG main_dlg[] = {
  { CTL_WINDOW,   window_proc,        0,   0,   640, 565, 0, 0,
    D_NORESIZE, 0, 0, "The LOSER Corps Map Editor" },
  { CTL_MENU,     menu_proc,          0,   0,   640, 24,  0, 0,
    D_AUTOW, 0, 0, main_menu },
  { CTL_TEXT,     text_proc,          320, 1,   319, 22,  0, 0,
    0, 2, 0, maindlg_tile_txt },

  { CTL_USER,     map_proc,           0, 24, MAPDLG_MAX_W, MAPDLG_MAX_H, 0, 0,
    D_WANTMOUSEMOVE, 0, 0, NULL },

  { CTL_USER,     ximage_proc,        5,   422,  64*9, 64, 0, 0,
    0, 0, 0, NULL },
  { CTL_HSCROLL,  hscroll_proc,       5,   408,  64*9, 12, 0, 0,
    0, 0, 0, image_change },

  { CTL_TEXT,     text_proc,          5,   495, 60,  30,  0, 0,
    0, 2, 0, "Tool:" },
  { CTL_USER,     tool_sel_proc,      70,  495, 80,  30,  0, 0,
    0, 0, 0, maindlg_tool_txt },

  { CTL_USER,     toggle_button_proc, 5,   530, 135, 30,  0, 0,
    0, 0, 0, "Double", double_change },

  { CTL_TEXT,     text_proc,          150, 495, 60,  20,  0, 0,
    0, 0, 0, "Layers:" },
  { CTL_USER,     radio_layer_proc,   220, 502, 10,  10,  0, 0,
    0, 1, 0, cur_layer_change },
  { CTL_USER,     radio_layer_proc,   220, 537, 10,  10,  0, 0,
    0, 0, 0, cur_layer_change },
  { CTL_USER,     toggle_button_proc, 250, 495, 135, 30,  0, 0,
    0, 1, 0, "Foreground", layer_change },
  { CTL_USER,     toggle_button_proc, 250, 530, 135, 30,  0, 0,
    0, 1, 0, "Background", layer_change },
  { CTL_USER,     toggle_button_proc, 390, 495, 135, 30,  0, 0,
    0, 1, 0, "Grid lines", layer_change },
  { CTL_USER,     toggle_button_proc, 390, 530, 135, 30,  0, 0,
    0, 1, 0, "Blocks", layer_change },

  { CTL_USER,     map_map_proc,       570, 495, 64,  64,  0, 0,
    0, 0, 0, NULL },

  { CTL_NONE, NULL }
};
