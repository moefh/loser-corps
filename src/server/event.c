/* event.c */

#include <stdio.h>
#include <malloc.h>

#include "server.h"


static void reset_event(EVENT *e)
{
  int i;

  e->frames_left = 0;
  e->frames = -1;
  e->func = NULL;
  for (i = 0; i < EVENT_DATA_SIZE; i++)
    e->data[i].p = NULL;
}

/* Add an event in the active list, allocating memory if needed */
EVENT *srv_allocate_event(SERVER *server)
{
  EVENT *e;
  int i, n, old_n;

  for (i = 0; i < server->n_events; i++)
    if (server->event[i].active == 0) {
      server->event[i].active = 1;
      reset_event(server->event + i);
      return server->event + i;
    }

  if (server->event == NULL) {
    old_n = 0;
    n = 16;
    e = (EVENT *) malloc(sizeof(EVENT) * n);
  } else {
    old_n = server->n_events;
    n = server->n_events + 16;
    e = (EVENT *) realloc(server->event, sizeof(EVENT) * n);
  }
  if (e == NULL)
    return NULL;    /* Out of memory */
  server->n_events = n;
  server->event = e;
  for (i = old_n+1; i < server->n_events; i++) {
    server->event[i].id = i;
    server->event[i].active = 0;
  }
  server->event[old_n].id = old_n;
  server->event[old_n].active = 1;
  reset_event(server->event + old_n);
  return server->event + old_n;
}

/* Remove an event from the active list */
void srv_free_event(SERVER *server, EVENT *e)
{
  e->active = 0;
}

/* Process events for the server */
void srv_process_events(SERVER *server)
{
  EVENT *e;
  int i;

  for (i = 0; i < server->n_events; i++) {
    e = server->event + i;
    if (! e->active)
      continue;
    if (--e->frames_left <= 0) {
      if (e->func != NULL)
	e->func(server, e);
      else
	DEBUG("ERROR: event with NULL function!\n");
      if (e->frames > 0)
	e->frames_left = e->frames;
      else
	srv_free_event(server, e);
    }
  }
}
