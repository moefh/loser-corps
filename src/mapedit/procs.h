/* procs.h */

#ifndef PROCS_H_FILE
#define PROCS_H_FILE

int menu_new(WINDOW *, int);
int menu_open(WINDOW *, int);
int menu_save(WINDOW *, int);
int menu_save_as(WINDOW *, int);
int menu_read_tiles(WINDOW *, int);
int menu_exit(WINDOW *, int);
int menu_undo(WINDOW *, int);
int menu_setsize(WINDOW *, int);
int menu_info(WINDOW *, int);
int menu_parms(WINDOW *, int);
int menu_about(WINDOW *, int);

void update_tile_images(void);

void prepare_dialog_colors(DIALOG *dlg);
void clip_state_pos(STATE *s, DIALOG *dlg);
int popup_proc(int msg, WINDOW *win, DIALOG *dlg, int c);
int radio_layer_proc(int msg, WINDOW *win, DIALOG *dlg, int c);
int ximage_proc(int msg, WINDOW *win, DIALOG *dlg, int c);
int tool_sel_proc(int msg, WINDOW *win, DIALOG *dlg, int c);
int toggle_button_proc(int, WINDOW *, DIALOG *, int);
int map_proc(int, WINDOW *, DIALOG *, int);
int map_map_proc(int, WINDOW *, DIALOG *, int);

void about_box(WINDOW *parent, char *title,
	       char *prog_name, char *prog_info, char *copyright, char **info);

#endif /* PROCS_H_FILE */
