/* addon.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "addon.h"


#define LIB_HASH_NUM   17

typedef struct LIB_ITEM LIB_ITEM;

struct LIB_ITEM {
  char name[32];                /* Library name */
  ADDON_LIBRARY lib;            /* Library info */
  int ref;                      /* # of references */
  LIB_ITEM *next;
};


/* The hash of loaded librearies */
static LIB_ITEM *hash[LIB_HASH_NUM];


/* Return the hash index for the name */
static int calc_hash(char *name)
{
  int i, h = 0;

  for (i = 0; name[i] != '\0'; i++)
    h = (h ^ name[i]) % LIB_HASH_NUM;

  return h;
}


/* Add a library in the hash */
static LIB_ITEM *add_in_hash(char *name, ADDON_LIBRARY lib)
{
  LIB_ITEM *item;
  int h;

  h = calc_hash(name);
  item = (LIB_ITEM *) malloc(sizeof(LIB_ITEM));
  strcpy(item->name, name);
  item->lib = lib;
  item->ref = 0;

  item->next = hash[h];
  hash[h] = item;

  return item;
}

static LIB_ITEM *find_in_hash(char *name)
{
  int h;
  LIB_ITEM *item;

  h = calc_hash(name);
  for (item = hash[h]; item != NULL; item = item->next)
    if (strcmp(item->name, name) == 0)
      return item;
  return NULL;
}

static void remove_from_hash(char *name)
{
  int h;
  LIB_ITEM *item, *prev;

  h = calc_hash(name);
  prev = NULL;
  for (item = hash[h]; item != NULL; prev = item, item = item->next)
    if (strcmp(item->name, name) == 0)
      break;

  if (item == NULL)
    return;          /* Not found */

  if (prev == NULL)
    hash[h] = item->next;
  else
    prev->next = item->next;
  free(item);
}

static LIB_ITEM *get_hash_item(void)
{
  int i;

  for (i = 0; i < LIB_HASH_NUM; i++)
    if (hash[i] != NULL)
      return hash[i];
  return NULL;
}


/* Load a library or increase the ref count if already loaded */
ADDON_LIBRARY *load_addon_library(char *name, char *func_name)
{
  ADDON_LIBRARY lib;
  LIB_ITEM *item;

  item = find_in_hash(name);
  if (item != NULL) {
    item->ref++;
    return &item->lib;
  }

  if (do_load_library(name, &lib, func_name))
    return NULL;
  item = add_in_hash(name, lib);
  item->ref = 1;

  DEBUG("Loaded library `%s'\n", name);

  return &item->lib;
}


/* Decrease the ref count of the library, and unload it if it reaches 0 */
void free_addon_library(char *name)
{
  LIB_ITEM *item;

  item = find_in_hash(name);
  if (item == NULL) {
    DEBUG("WARNING: trying to free unloaded library `%s'\n", name);
    return;
  }

  item->ref--;
  if (item->ref == 0) {     /* Really unload if ref count reached 0 */
    do_close_library(&item->lib);
    remove_from_hash(name);
  }
}

/* Free all addon libraries */
void free_addon_libs(void)
{
  LIB_ITEM *item;

  while ((item = get_hash_item()) != NULL) {
    do_close_library(&item->lib);
    remove_from_hash(item->name);
  }
}
