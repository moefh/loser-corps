/* network.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 *
 * Network-dependent code for the server.
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "server.h"
#include "connect.h"
#include "game.h"


/* Open and bind a socket to the server's port */
int srv_open(SERVER *server)
{
  struct sockaddr_in name;
  int true = 1;

  select_loser_data_dir(0);

  server->change_map = 0;

  server->sock = socket(PF_INET, SOCK_STREAM, 0);
  if (server->sock < 0) {
    perror("socket()");
    return 1;
  }
  setsockopt(server->sock, SOL_SOCKET, SO_REUSEADDR,
	     (void *)&true, sizeof(true));

  name.sin_family = AF_INET;
  name.sin_port = htons(server->port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(server->sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
    if (errno == EADDRINUSE)
      fprintf(stderr, "The port %d of this host is already being used.\n",
	      server->port);
    else
      perror("bind()");
    close(server->sock);
    return 1;
  }
  if (listen(server->sock, 1) < 0) {
    perror("listen()");
    close(server->sock);
    return 1;
  }

  return 0;
}

/* Close the connection to all clients and stop accepting connections */
void srv_close(SERVER *server)
{
  srv_reset_server(server);
  close(server->sock);
}

/* Close the connection to a client */
void srv_close_connection(int fd)
{
  close(fd);
}

/* Close the connection to a client */
void srv_close_client(CLIENT *c)
{
  close(c->fd);
}

/* Accept an incoming connection. */
int srv_accept_connection(SERVER *server, char *host, int *port)
{
  struct sockaddr_in client_name;
  int fd;
  socklen_t size;

  size = sizeof(client_name);
  do
    fd = accept(server->sock, (struct sockaddr *) &client_name, &size);
  while (fd < 0 && errno == EINTR);
  if (fd < 0) {
    DEBUG("accept(): %s\n", err_msg());
    return -1;
  }

  strcpy(host, inet_ntoa(client_name.sin_addr));
  *port = ntohs(client_name.sin_port);
  return fd;
}

/* Verify incoming data from the net. Returns:
 *
 *    -2: a client is trying to connect (the
 *        server should accept the connection)
 *
 *    -1: nothing happened
 *
 *  >= 0: a client sent a message;
 *        the return value is the fd of that client
 */
int srv_verify_data(SERVER *server)
{
  struct timeval tv, *ptv;
  fd_set set;
  CLIENT *c;

  FD_ZERO(&set);
  FD_SET(server->sock, &set);
  for (c = server->joined; c != NULL; c = c->next)
    FD_SET(c->fd, &set);

  if (server->game_running) {
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    ptv = &tv;
  } else if (server->max_idle_time > 0) {
    tv.tv_sec = server->max_idle_time;
    tv.tv_usec = 0;
    ptv = &tv;
  } else
    ptv = NULL;

  while (select(FD_SETSIZE, &set, NULL, NULL, ptv) < 0)
    if (errno != EINTR || server->quit) {
      if (errno != EINTR)
	DEBUG("select(): %s\n", err_msg());
      return -1;
    }

  /* Verify client messages */
  for (c = server->joined; c != NULL; c = c->next)
    if (FD_ISSET(c->fd, &set))
      return c->fd;

  /* Verify new connections */
  if (FD_ISSET(server->sock, &set))
    return -2;

  /* Timed out; quit */
  if (server->max_idle_time > 0 && ! server->game_running) {
    DEBUG("Server timed out, quitting...\n");
    server->quit = 1;
  }

  /* Nothing happened */
  return -1;
}
