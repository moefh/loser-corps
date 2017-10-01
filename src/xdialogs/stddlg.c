/* stddlg.c - Standard dialog definitions.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#ifndef NO_FNMATCH
#include <fnmatch.h>
#endif /* NO_FNMATCH */

#include "xdialogs.h"

#ifndef NO_FNMATCH

#define FN_SIZE   1024

typedef struct NAME_LIST {
  int n;
  char **name;
} NAME_LIST;

static NAME_LIST file_list, dir_list;
static char cur_dir_name[FN_SIZE];
static char cur_pattern[FN_SIZE];

static char *get_file_list(int, int *);
static char *get_dir_list(int, int *);

static int edit_file_proc(int, WINDOW *, DIALOG *, int);
static int dir_list_proc(int, WINDOW *, DIALOG *, int);
static int file_list_proc(int, WINDOW *, DIALOG *, int);
static int bt_filter_proc(int, WINDOW *, DIALOG *, int);

#define DF_DIRNAME    1
#define DF_PATTERN    2
#define DF_LISTFILES  3
#define DF_LISTDIRS   4
#define DF_OK         5
#define DF_CANCEL     6

static DIALOG dlg_file[] = {
  { CTL_WINDOW, window_proc,    0,   0,  500, 340,  0, 15, 0,
    0, 0, NULL },
  { CTL_TEXT,   text_proc,      10,  10, 380, 20,   0, 15, D_AUTOH,
    0, 0, cur_dir_name },
  { CTL_EDIT,   edit_file_proc, 10,  50, 380, 20,   0, 15, D_EXIT|D_AUTOH,
    0, FN_SIZE-2, cur_pattern },
  { CTL_LIST,   file_list_proc, 10,  90, 180, 230,  0, 15, D_EXIT|D_AUTOH,
    0, 0, get_file_list },
  { CTL_LIST,   dir_list_proc,  210, 90, 180, 230,  0, 15, D_EXIT|D_AUTOH,
    0, 0, get_dir_list },
  { CTL_BUTTON, bt_filter_proc, 410, 10, 80,  30,   0, 15, D_AUTOH|D_EXIT,
    0, 0, "OK" },
  { CTL_BUTTON, button_proc,    410, 50, 80,  30,   0, 15, D_AUTOH|D_EXIT,
    0, 0, "Cancel" },
  { CTL_NONE, NULL }
};

static void put_slash(char *s)
{
  int i;

  i = strlen(s);
  if (i > 0 && s[i-1] != '/') {
    s[i] = '/';
    s[i+1] = '\0';
  }
}

/* Split the path in `dir' and `file' */
static void split_path(char *path, char *dir, char *file)
{
  int i;
  struct stat st;

  if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
    if (dir != NULL && path != dir)
      strcpy(dir, path);
    if (file != NULL)
      file[0] = '\0';
  } else {
    i = strlen(path);
    while (i >= 0 && path[i] != '/')
      i--;
    if (file != NULL)
      strcpy(file, path + (i + 1));
    if (dir != NULL) {
      if (dir != path)
	strncpy(dir, path, i + 1);
      dir[i + 1] = '\0';
    }
  }
}

/* Returns 1 if `s' is not the name of an existing directory and does
 * not contain shell-style wildcards (? and *). Escaped ? and *
 * (i.e., `\?' and `\*') are allowed. */
static int valid_file_name(char *s)
{
  int i;
  struct stat st;

  if (stat(s, &st) == 0 && S_ISDIR(st.st_mode))
    return 0;

  for (i = 0; s[i] != '\0'; i++)
    if (s[i] == '\\' && s[i + 1] != '\0') {
      i++;
      continue;
    } else if (s[i] == '*' || s[i] == '?')
      return 0;
  return 1;
}

/* Remove all "/../" and "/./" from a path name */
static void canonize_path(char *s)
{
  int i, j;
  char buf[FN_SIZE];

  j = i = 0;
  if (s[i] != '/') {
    if (getcwd(buf, FN_SIZE) == NULL)
      buf[0] = '\0';
    put_slash(buf);
    j = strlen(buf);
  }
  while (s[i] != '\0') {
    if (i == 0 || s[i-1] == '/') {
      if (s[i] == '.' && s[i + 1] == '.' && (s[i + 2] == '/'
					     || s[i + 2] == '\0')) {
	i += 2;
	if (s[i] == '/')
	  i++;
	j -= 2;
	while (j > 0 && buf[j] != '/')
	  j--;
	if (j < 0)
	  j = 0;
	buf[j++] = '/';
      } else if (s[i] == '.' && (s[i + 1] == '/' || s[i + 1] == '\0')) {
	i++;
	if (s[i] == '/')
	  i++;
      } else if (i > 0 && s[i] == '/')
	i++;
      else
	buf[j++] = s[i++];
    } else
      buf[j++] = s[i++];
  }
  if (j < 0)
    j = 0;
  buf[j] = '\0';

  strcpy(s, buf);
}

/* Compare two file names for sorting */
static int cmp_name(const void *a, const void *b)
{
  return strcasecmp(*(char **)a, *(char **)b);
}

/* Return a list of regular files and directories of the given directory */
static int read_dir(char *dir_name, char *patt,
		    NAME_LIST *dirs, NAME_LIST *files)
{
  DIR *dir;
  struct dirent *ent;
  struct stat st;
  int nf, nd, dir_size;
  char buf[FN_SIZE];

  if ((dir = opendir(dir_name)) == NULL) {
    dirs->n = 0;
    files->n = 0;
    return 1;
  }

  strcpy(buf, dir_name);
  put_slash(buf);
  dir_size = strlen(buf);

  nf = nd = 0;
  while ((ent = readdir(dir)) != NULL) {
    strcpy(buf + dir_size, ent->d_name);
    stat(buf, &st);
    if (S_ISDIR(st.st_mode)) {
      nd++;
      continue;
    }
    if (fnmatch(patt, ent->d_name, FNM_PERIOD|FNM_PATHNAME) != 0)
      continue;
    if (S_ISREG(st.st_mode))
      nf++;
#if DEBUG
    else if (S_ISLNK(st.st_mode))
      fprintf(stderr, "WARNING: discarding link `%s' from file list\n", buf);
#endif
  }

  files->n = nf;
  files->name = (char **) malloc(nf * sizeof(char *));
  dirs->n = nd;
  dirs->name = (char **) malloc(nd * sizeof(char *));
  rewinddir(dir);

  nf = nd = 0;
  while ((ent = readdir(dir)) != NULL) {
    strcpy(buf + dir_size, ent->d_name);
    stat(buf, &st);
    if (S_ISDIR(st.st_mode)) {
      dirs->name[nd++] = strdup(ent->d_name);
      continue;
    }
    if (fnmatch(patt, ent->d_name, FNM_PERIOD|FNM_PATHNAME) == 0
	&& S_ISREG(st.st_mode))
      files->name[nf++] = strdup(ent->d_name);
  }
  closedir(dir);

  /* Sort out the lists */
  qsort(dirs->name, dirs->n, sizeof(char **), cmp_name);
  qsort(files->name, files->n, sizeof(char **), cmp_name);

  return 0;
}

/* Free a name list allocated by read_dir() */
static void free_name_list(NAME_LIST *l)
{
  if (l->n != 0) {
    int i;

    for (i = 0; i < l->n; i++)
      free(l->name[i]);
    free(l->name);
    l->n = 0;
  }
}

/* Return the file list */
static char *get_file_list(int index, int *num)
{
  if (index < 0) {
    if (num != NULL)
      *num = file_list.n;
    return NULL;
  } else
    return file_list.name[index];
}

/* Return the dir list */
static char *get_dir_list(int index, int *num)
{
  if (index < 0) {
    *num = dir_list.n;
    return NULL;
  } else
    return dir_list.name[index];
}

static void do_filter(void)
{
  NAME_LIST f, d;
  char buf[FN_SIZE];

  canonize_path(cur_pattern);
  split_path(cur_pattern, cur_dir_name, buf);
  if (cur_dir_name[0] == '\0') {
    strcpy(cur_dir_name, ".");
    canonize_path(cur_dir_name);
  }
  put_slash(cur_dir_name);
  if (buf[0] == '\0')
    strcpy(buf, "*");

  strcpy(cur_pattern, cur_dir_name);
  put_slash(cur_pattern);
  strcat(cur_pattern, buf);
  
  read_dir(cur_dir_name, buf, &d, &f);
  free_name_list(&file_list);
  free_name_list(&dir_list);
  file_list = f;
  dir_list = d;

  dlg_file[DF_LISTFILES].d1 = dlg_file[DF_LISTFILES].d2 = 0;
  dlg_file[DF_LISTDIRS].d1 = dlg_file[DF_LISTDIRS].d2 = 0;
  dlg_file[DF_PATTERN].d1 = strlen(cur_pattern);
  SEND_MESSAGE(MSG_DRAW, dlg_file + DF_LISTFILES, 0);
  SEND_MESSAGE(MSG_DRAW, dlg_file + DF_LISTDIRS, 0);
  SEND_MESSAGE(MSG_DRAW, dlg_file + DF_PATTERN, 0);
  SEND_MESSAGE(MSG_DRAW, dlg_file + DF_DIRNAME, 0);
}

static int edit_file_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  int ret;

  if (msg == MSG_INIT)
    do_filter();

  if (msg == MSG_END) {
    free_name_list(&file_list);
    free_name_list(&dir_list);
  }
  ret = edit_proc(msg, win, dlg, c);

  if (ret == D_CLOSE) {
    if (valid_file_name(cur_pattern))
      return D_CLOSE;
    do_filter();
    return D_O_K;
  }

  return ret;
}

static int file_list_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  int ret;

  ret = list_proc(msg, win, dlg, c);

  if (ret == D_CLOSE) {
    if (dlg->d1 >= 0 && file_list.n > 0 && dlg->d1 < file_list.n) {
      strcpy(cur_pattern, cur_dir_name);
      put_slash(cur_pattern);
      strcat(cur_pattern, file_list.name[dlg->d1]);
      return D_CLOSE;
    }
    return D_O_K;
  }
  return ret;
}

static int dir_list_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  int ret;

  ret = list_proc(msg, win, dlg, c);

  if (ret == D_CLOSE) {
    if (dlg->d1 >= 0 && dir_list.n > 0 && dlg->d1 < dir_list.n) {
      char buf[FN_SIZE];

      split_path(cur_pattern, NULL, buf);
      strcpy(cur_pattern, cur_dir_name);
      put_slash(cur_pattern);
      strcat(cur_pattern, dir_list.name[dlg->d1]);
      put_slash(cur_pattern);
      strcat(cur_pattern, buf);
      do_filter();
    }
    return D_O_K;
  }
  return ret;
}

/* Controls the "Filter" button */
static int bt_filter_proc(int msg, WINDOW *win, DIALOG *dlg, int c)
{
  int ret;

  ret = button_proc(msg, win, dlg, c);

  if (ret == D_CLOSE && (! valid_file_name(cur_pattern))) {
    do_filter();
    return D_O_K;
  }
  return ret;
}

/* Select a file. */
int file_select(WINDOW *parent, char *title, char *path, char *ext)
{
  int result;

  dlg_file[0].dp = title;
  strcpy(cur_pattern, path);
  put_slash(cur_pattern);
  if (ext[0] != '*')
    strcat(cur_pattern, "*");
  strcat(cur_pattern, ext);

  centre_dialog(dlg_file, NULL);
  dlg_file->flags |= D_NORESIZE;
  set_dialog_flag(dlg_file, D_3D, xd_stddlg_3d);
  set_dialog_color(dlg_file, gui_fg_color, gui_bg_color);
  set_dialog_flag(dlg_file, D_GOTFOCUS, 0);
  dlg_file[DF_PATTERN].flags |= D_GOTFOCUS;
  dlg_file[DF_LISTFILES].d1 = dlg_file[DF_LISTFILES].d2 = 0;
  dlg_file[DF_LISTDIRS].d1 = dlg_file[DF_LISTDIRS].d2 = 0;

  dlg_file[0].bg = (xd_stddlg_3d) ? xd_back_3d : gui_bg_color;
  result = do_dialog_window(parent, dlg_file, 1);

  if (result == DF_OK || result == DF_PATTERN || result == DF_LISTFILES) {
    strcpy(path, cur_pattern);
    return 1;
  }
  return 0;
}

#endif /* NO_FNMATCH */


static DIALOG alert_dlg[] = {
  { CTL_WINDOW, window_proc,  0,   0,   400, 180, 0, 7, 0,      0, 0, NULL },
  { CTL_TEXT,   text_proc,    10,  10,  380, 30,  0, 7, 0,      1, 0, NULL },
  { CTL_TEXT,   text_proc,    10,  50,  380, 30,  0, 7, 0,      1, 0, NULL },
  { CTL_TEXT,   text_proc,    10,  90,  380, 30,  0, 7, 0,      1, 0, NULL },
  { CTL_BUTTON, button_proc,  90,  130, 100, 30,  0, 7, D_EXIT, 0, 0, NULL },
  { CTL_BUTTON, button_proc,  210, 130, 100, 30,  0, 7, D_EXIT, 0, 0, NULL },
  { CTL_NONE, NULL }
};

/* Display an alert box. Note that the DIALOG structure is not global
 * because there can be more than one alert boxes at a time.
 */
int alert(WINDOW *parent, char *title, char *s1, char *s2, char *s3,
	  char *b1, char *b2)
{
  int ret;

  alert_dlg[0].dp = ((title != NULL) ? title : "Alert");
  alert_dlg[1].dp = ((s1 != NULL) ? s1 : "");
  alert_dlg[2].dp = ((s2 != NULL) ? s2 : "");
  alert_dlg[3].dp = ((s3 != NULL) ? s3 : "");
  alert_dlg[4].dp = b1;
  if (b2 == NULL) {
    alert_dlg[4].x = 150;
    alert_dlg[5].proc = NULL;
    alert_dlg[5].type = CTL_NONE;
  } else {
    alert_dlg[4].x = 90;
    alert_dlg[5].x = 210;
    alert_dlg[5].type = CTL_BUTTON;
    alert_dlg[5].proc = button_proc;
    alert_dlg[5].dp = b2;
  }

  centre_dialog(alert_dlg, parent);
  alert_dlg->flags |= D_NORESIZE;
  set_dialog_flag(alert_dlg, D_3D, xd_stddlg_3d);
  set_dialog_color(alert_dlg, gui_fg_color, gui_bg_color);
  set_dialog_flag(alert_dlg, D_GOTFOCUS, 0);

  ret = do_dialog_window(parent, alert_dlg, 1);

  if (ret == 4)
    return 1;
  return 2;
}
