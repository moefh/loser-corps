/* lowlib.c
 *
 * Low level library loading functions.
 */

#include <stdio.h>
#include <dlfcn.h>

#include "server.h"
#include "addon.h"


/* Load a library */
int do_load_library(char *name, ADDON_LIBRARY *lib, char *func_name)
{
  char filename[256];

  snprintf(filename, sizeof(filename), LIBDIR "/libs/npcs/lib%s.so", name);

  lib->handle = dlopen(filename, RTLD_NOW);
  if (lib->handle == NULL) {
    printf ("%s\n", dlerror());
    DEBUG("%s\n", dlerror());
    return 1;
  }

  lib->function = dlsym(lib->handle, func_name);
  if (lib->function == NULL) {
    DEBUG("Can't find function `move_enemy()' in library `%s'\n", filename);
    return 1;
  }

  return 0;
}

/* Unload a library */
void do_close_library(ADDON_LIBRARY *lib)
{
  dlclose(lib->handle);
}
