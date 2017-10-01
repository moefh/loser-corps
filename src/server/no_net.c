/* no_net.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 *
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "server.h"
#include "connect.h"
#include "common.h"
#include "game.h"


int srv_open(SERVER *server)
{
  server->change_map = 0;
  server->sock = 0;

  return 0;
}

void srv_close(SERVER *server)
{
  srv_reset_server(server);
}

void srv_close_connection(int fd)
{
#ifndef _WINDOWS
  close(fd);
#endif /* _WINDOWS */
}

void srv_close_client(SERVER *server) { }

int srv_accept_connection(SERVER *server, char *host, int *port)
{
  strcpy(host, "local");
  *port = 42;
  return SERVER_FD;
}

int srv_verify_data(SERVER *server)
{
  if (! server->game_running && server->joined == NULL)
    return -2;

  if (! server->game_running && has_message(SERVER_FD))
    return SERVER_FD;
  return -1;
}
