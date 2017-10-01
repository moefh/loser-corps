/* eyes.c */

#include <stdio.h>
#include <stdlib.h>

#include <../server/server.h>


static void set_eye_frame(SERVER *server, EVENT *e)
{
  ITEM *it = server->item + e->data[0].i;
  int i = e->data[1].i;

  it->client_frame = i;
  it->changed = 1;
}

static void set_eye_state(SERVER *server, EVENT *e)
{
  ITEM *it = server->item + e->data[0].i;
  int i = e->data[1].i;

  it->state = i;
}


static void program_eye_function(SERVER *server, ITEM *it,
				 void (*func)(SERVER *, EVENT *),
				 int frame, int frames_left)
{
  EVENT *e;

  e = srv_allocate_event(server);
  if (e == NULL) {
    printf("ERROR: can't allocate event\n");
    exit(1);
    return;
  }
  e->frames_left = frames_left;
  e->frames = -1;
  e->func = func;
  e->data[0].i = it->id;
  e->data[1].i = frame;
}

void get_item(SERVER *server, ITEM *it, CLIENT *client)
{
  int frames_left, i, duration;
  NPC_INFO *npc;

  if (server->change_map || it->state)
    return;

  it->state = 1;
  frames_left = 2;
  npc = server->npc_info + it->npc;
  for (i = 1; i < npc->n_stand/2; i++)
    program_eye_function(server, it, set_eye_frame, i, frames_left++);

  if (it->duration >= 0 && (duration = srv_sec2frame(it->duration)) > 0) {
    frames_left += duration;
    for (i = npc->n_stand/2-1; i >= 0; i--)
      program_eye_function(server, it, set_eye_frame, i, frames_left++);
    program_eye_function(server, it, set_eye_state, 0, frames_left);
  }
}
