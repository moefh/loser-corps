/* sound.c */
#ifdef HAS_SOUND
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"


int sfx_explosion, sfx_bump, sfx_blip;

/* Make a tremor effect */
void srv_do_tremor(SERVER *server, int x, int y, int duration, int intensity)
{
  server->do_tremor = 1;
  server->tremor.x = x;
  server->tremor.y = y;
  server->tremor.duration = duration;
  server->tremor.intensity = intensity;
}

/* Return the SFX ID from a given SFX name */
int sfx_id(SERVER *server, char *name)
{
  int i;

  for (i = 0; i < server->sound_info.n_samples; i++)
    if (strcmp(name, server->sound_info.sample[i].name) == 0)
      return server->sound_info.sample[i].id;
  return -1;    /* Not found */
}


/* Reset a sound structure */
static void reset_sound(SOUND *s)
{
  s->sfx = 0;
  s->client_id = -1;
  s->x = s->y = 0;
}

/* Add an item in the active list, allocating memory if needed */
SOUND *srv_allocate_sound(SERVER *server)
{
  SOUND *s;
  int i, n, old_n;

  for (i = 0; i < server->n_sounds; i++)
    if (server->sound[i].active == 0) {
      server->sound[i].active = 1;
      reset_sound(server->sound + i);
      return server->sound + i;
    }

  if (server->sound == NULL) {
    old_n = 0;
    n = 16;
    s = (SOUND *) malloc(sizeof(SOUND) * n);
  } else {
    old_n = server->n_sounds;
    n = server->n_sounds + 16;
    s = (SOUND *) realloc(server->sound, sizeof(SOUND) * n);
  }
  if (s == NULL)
    return NULL;    /* Out of memory */
  server->n_sounds = n;
  server->sound = s;
  for (i = old_n+1; i < server->n_sounds; i++) {
    server->sound[i].id = i;
    server->sound[i].active = 0;
  }
  server->sound[old_n].id = old_n;
  server->sound[old_n].active = 1;
  reset_sound(server->sound + old_n);
  return server->sound + old_n;
}

/* Remove a item from the active list */
void srv_free_sound(SERVER *server, SOUND *s)
{
  s->active = 0;
}
#endif
