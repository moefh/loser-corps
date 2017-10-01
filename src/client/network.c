/* network.c */

#include <stdio.h>
#include <errno.h>

#include "game.h"
#include "common.h"
#include "commands.h"

#ifdef NET_PLAY

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

static int init_sockaddr(struct sockaddr_in *name,
			 const char *hostname, unsigned short int port)
{
  struct hostent *hostinfo;
     
  name->sin_family = AF_INET;
  name->sin_port = htons(port);
  hostinfo = gethostbyname(hostname);
  if (hostinfo == NULL) {
    display_msg("Unknown host `%s'.\n", hostname);
    return 1;
  }
  name->sin_addr = *(struct in_addr *) hostinfo->h_addr;
  return 0;
}


/* Open a connection to the server */
int client_open(SERVER *server)
{
  struct sockaddr_in servername;

#if 0
  debug_file = stdout;
#endif

  /* Create the socket */
  server->fd = socket(PF_INET, SOCK_STREAM, 0);
  if (server->fd < 0) {
    display_msg("socket(): %s\n", err_msg());
    return 1;
  }
     
  /* Connect to the server */
  if (init_sockaddr(&servername, server->host, server->port)) {
    close(server->fd);
    return 1;
  }
  if (connect(server->fd,
	      (struct sockaddr *) &servername, sizeof(servername)) < 0) {
    if (errno == ECONNREFUSED)
      display_msg("Connection refused.\n"
		  "Please make sure that a game server\n"
		  "is running at %s:%d.\n\n",
		  server->host, server->port);
    else
      display_msg("connect(): %s\n", err_msg());
    close(server->fd);
    return 1;
  }

  return 0;
}

/* Close the connection to the server */
void client_close(SERVER *server)
{
  close(server->fd);
}


#else /* NET_PLAY */

#include "srvlib.h"


int client_open(SERVER *server)
{
  server->fd = CLIENT_FD;

  if (srvlib_init(42)) {
    display_msg("Out of memory to start server\n");
    return 1;
  }
  return 0;
}

void client_do_server(SERVER *server)
{
  srvlib_do_step();
}

void client_close(SERVER *server)
{
  srvlib_close();
}

#endif /* NET_PLAY */
