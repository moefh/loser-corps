/* enemylib.h */

#ifndef ADDONLIB_H
#define ADDONLIB_H

typedef struct ADDON_LIBRARY {
  void *handle;
  void (*function)();
} ADDON_LIBRARY;


ADDON_LIBRARY *load_addon_library(char *name, char *func_name);
void free_addon_library(char *name);
void free_addon_libs(void);

/* Low-level functions: */
int do_load_library(char *name, ADDON_LIBRARY *lib, char *func_name);
void do_close_library(ADDON_LIBRARY *lib);

#endif
