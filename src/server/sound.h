/* sound.h */

#ifndef SOUND_H
#define SOUND_H

#include "snd_data.h"

typedef struct SOUND {
  int id;                /* Sound ID */
  int active;            /* 1 if active */
  int sfx;               /* Sound Effect ID (SFX_*) */
  int x, y;              /* Position of the sound */
  int client_id;         /* client id to send the sound to (-1=everybody) */
} SOUND;

SOUND *srv_allocate_sound(SERVER *server);
void srv_free_sound(SERVER *server, SOUND *s);

int sfx_id(SERVER *server, char *name);

void srv_do_tremor(SERVER *server, int x, int y, int duration, int intensity);

/* Some frequently used sounds: */
extern int sfx_explosion, sfx_bump, sfx_blip;

#endif
