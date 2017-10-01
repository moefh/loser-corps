/* srvlib.c */

#include <stdio.h>
#include <malloc.h>

#include "server.h"
#include "connect.h"
#include "network.h"
#include "game.h"


static SERVER srvlib_server;

int srvlib_init(int port)
{
  SERVER *server = &srvlib_server;

  server->min_frame_time = 0;
  server->max_idle_time = 600;     /* 10 minutes */

  server->quit = 0;
  server->reset = 0;
  server->client = server->joined = NULL;
  server->n_npcs = 0;
  server->npc_info = NULL;
  server->n_weapons = 0;
  server->weapon = NULL;
  server->n_events = 0;
  server->event = NULL;
  server->n_sounds = 0;
  server->sound = NULL;

  server->port = port;

  if (srv_open(server))
    return 1;

  return 0;
}

void srvlib_do_step(void)
{
  srv_do_frame(&srvlib_server);
}

void srvlib_close(void)
{
  srv_close(&srvlib_server);
}
