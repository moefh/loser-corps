/* dialogs.h */

#ifndef DIALOGS_H_FILE
#define DIALOGS_H_FILE

#define MAPDLG_MAX_W  640
#define MAPDLG_MAX_H  380


extern char objdlg_npc_txt[];
extern char objdlg_x_txt[];
extern char objdlg_y_txt[];
extern char objdlg_respawn_txt[];
extern char objdlg_dir_txt[];
extern char objdlg_level_txt[];
extern char objdlg_duration_txt[];
extern char *objdlg_target_txt[];

extern DIALOG obj_dialog[];

enum {
  OBJDLG_WINDOW,

  OBJDLG_NPC_LABEL,
  OBJDLG_NPC_LIST,
  OBJDLG_X_LABEL,
  OBJDLG_X_TEXT,
  OBJDLG_Y_LABEL,
  OBJDLG_Y_TEXT,
  OBJDLG_RESPAWN_LABEL,
  OBJDLG_RESPAWN_TEXT,
  OBJDLG_DIR_LABEL,
  OBJDLG_DIR_TEXT,
  OBJDLG_LEVEL_LABEL,
  OBJDLG_LEVEL_TEXT,
  OBJDLG_DURATION_LABEL,
  OBJDLG_DURATION_TEXT,
  OBJDLG_TARGET_LABEL,
  OBJDLG_TARGET_TEXT,
  OBJDLG_VULNERABILITY_LABEL,
  OBJDLG_VULNERABILITY_TEXT,
  OBJDLG_MAP_LABEL,
  OBJDLG_MAP_TEXT,

  OBJDLG_BUT_OK,
  OBJDLG_BUT_CANCEL,

  OBJDLG_END,
};

void set_object_attributes(WINDOW *win, MAP_OBJECT *obj, int id);



extern char newdlg_width_txt[];
extern char newdlg_height_txt[];
extern char newdlg_fore_txt[];
extern char newdlg_back_txt[];

extern DIALOG new_dlg[];

enum {
  NEWDLG_WINDOW,
  NEWDLG_WIDTH_LABEL,
  NEWDLG_WIDTH_TEXT,
  NEWDLG_HEIGHT_LABEL,
  NEWDLG_HEIGHT_TEXT,
  NEWDLG_FORE_LABEL,
  NEWDLG_FORE_TEXT,
  NEWDLG_BACK_LABEL,
  NEWDLG_BACK_TEXT,
  NEWDLG_BUT_OK,
  NEWDLG_BUT_CANCEL,

  NEWDLG_END
};


extern char sizedlg_width_txt[];
extern char sizedlg_height_txt[];

extern DIALOG size_dlg[];

enum {
  SIZEDLG_WINDOW,

  SIZEDLG_WIDTH_LABEL,
  SIZEDLG_WIDTH_TXT,
  SIZEDLG_HEIGHT_LABEL,
  SIZEDLG_HEIGHT_TXT,

  SIZEDLG_BUT_OK,
  SIZEDLG_BUT_CANCEL,

  SIZEDLG_END,
};



extern char infodlg_author_txt[];
extern char infodlg_comment_txt[];
extern char infodlg_tile_txt[];

extern DIALOG info_dlg[];

enum {
  INFODLG_WINDOW,
  INFODLG_AUTHOR_LABEL,
  INFODLG_AUTHOR_TXT,
  INFODLG_COMMENT_LABEL,
  INFODLG_COMMENT_TXT,
  INFODLG_TILE_LABEL,
  INFODLG_TILE_TXT,

  INFODLG_BUT_OK,
  INFODLG_BUT_CANCEL,

  INFODLG_END
};


extern char parmdlg_maxyspeed_txt[];
extern char parmdlg_jumphold_txt[];
extern char parmdlg_gravity_txt[];
extern char parmdlg_maxxspeed_txt[];
extern char parmdlg_accel_txt[];
extern char parmdlg_jumpaccel_txt[];
extern char parmdlg_attrict_txt[];
extern char parmdlg_frameskip_txt[];

extern DIALOG parm_dlg[];

enum {
  PARMDLG_WINDOW,

  PARMDLG_MAXYSPEED_LABEL,
  PARMDLG_MAXYSPEED,
  PARMDLG_JUMPHOLD_LABEL,
  PARMDLG_JUMPHOLD,
  PARMDLG_GRAVITY_LABEL,
  PARMDLG_GRAVITY,
  PARMDLG_MAXXSPEED_LABEL,
  PARMDLG_MAXXSPEED,
  PARMDLG_ACCEL_LABEL,
  PARMDLG_ACCEL,
  PARMDLG_JUMPACCEL_LABEL,
  PARMDLG_JUMPACCEL,
  PARMDLG_ATTRICT_LABEL,
  PARMDLG_ATTRICT,
  PARMDLG_FRAMESKIP_LABEL,
  PARMDLG_FRAMESKIP,

  PARMDLG_BUT_OK,
  PARMDLG_BUT_CANCEL,
  PARMDLG_BUT_DEFAULT,
};


extern char maindlg_tile_txt[];

extern DIALOG main_dlg[];

enum {
  MAINDLG_WINDOW,
  MAINDLG_MENU,
  MAINDLG_TILE_TXT,
  MAINDLG_MAP,

  MAINDLG_TILE_IMG,
  MAINDLG_TILE_SCROLL,

  MAINDLG_TOOL_LABEL,
  MAINDLG_TOOL,
  MAINDLG_DOUBLE,

  MAINDLG_LAYER_LABEL,
  MAINDLG_RADIOLAYER_FORE,
  MAINDLG_RADIOLAYER_BACK,
  MAINDLG_LAYER_FORE,
  MAINDLG_LAYER_BACK,
  MAINDLG_LAYER_GRID,
  MAINDLG_LAYER_BLOCK,

  MAINDLG_MAP_MAP,

  MAINDLG_END
};


void image_change(DIALOG *dlg);

#endif /* DIALOGS_H_FILE */
