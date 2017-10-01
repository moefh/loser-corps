/* main.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "server.h"
#include "connect.h"
#include "network.h"
#include "game.h"


static SERVER server;

static void interrupt(int sig)
{
  server.quit = 1;
  signal(sig, interrupt);
}

static void reset(int sig)
{
  server.reset = 1;
  signal(sig, reset);
}

static void print_help(char *name)
{
  printf("The L.O.S.E.R Corps game server version %d.%d.%d\n\
\n\
%s [options]\n\
\n\
options:\n\
  -help          show this information\n\
  -port N        bind to port N (default: %d)\n\
  -debug         print messages and do not fork to release the tty\n\
  -nobuf         don't buffer network writes (slower but safer)\n\
  -maxfps N      set the maximum number of frames per second\n\
  -maxidle N     set the maximum idle time to N seconds (default: 600)\n\
  -keep          set the maximum idle time to infinity\n\
\n\
",
	 VERSION1(SERVER_VERSION),
	 VERSION2(SERVER_VERSION),
	 VERSION3(SERVER_VERSION),
	 name, DEFAULT_PORT);
}


static void do_game(SERVER *server)
{
  DEBUG("Server accepting connections\n");
  server->game_running = 0;

  while (! server->quit)
    srv_do_frame(server);
}


int main(int argc, char *argv[])
{
  int i, debug = 0;

  use_out_buffer = 1;          /* Use output buffer */
  server.port = DEFAULT_PORT;

  server.min_frame_time = 0;
  server.max_idle_time = 600;     /* 10 minutes */

  server.quit = 0;
  server.reset = 0;
  server.client = server.joined = NULL;
  server.n_npcs = 0;
  server.npc_info = NULL;
  server.n_weapons = 0;
  server.weapon = NULL;
  server.n_enemies = 0;
  server.enemy = NULL;
  server.n_events = 0;
  server.event = NULL;
  server.n_sounds = 0;
  server.sound = NULL;

  for (i = 1; i < argc; i++)
    if (strcmp(argv[i], "-help") == 0) {
      print_help(argv[0]);
      exit(1);
    } else if (strcmp(argv[i], "-nobuf") == 0)
      use_out_buffer = 0;
    else if (strcmp(argv[i], "-debug") == 0)
      debug = 1;
    else if (strcmp(argv[i], "-keep") == 0)
      server.max_idle_time = -1;
    else if (strcmp(argv[i], "-maxidle") == 0) {
      if (argv[i+1] == NULL) {
	fprintf(stderr, "%s: expected: port number\n", argv[0]);
	exit(1);
      }
      server.max_idle_time = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-port") == 0) {
      if (argv[i+1] == NULL) {
	fprintf(stderr, "%s: expected: port number\n", argv[0]);
	exit(1);
      }
      server.port = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-maxfps") == 0) {
      int maxfps;

      if (argv[i+1] == NULL) {
	fprintf(stderr, "%s: expected: maximum fps value\n", argv[0]);
	exit(1);
      }
      if ((maxfps = atoi(argv[++i])) <= 0) {
	fprintf(stderr, "%s: invalid maximum fps value\n", argv[0]);
	exit(1);
      }
      server.min_frame_time = 1000000 / maxfps;
    } else {
      fprintf(stderr, "%s: illegal option: `%s'\n", argv[0], argv[i]);
      exit(1);
    }

  if (debug)
    debug_file = stderr;

  if (srv_open(&server))
    exit(1);

  if (! debug) {
    pid_t pid;

    pid = fork();
    if (pid < 0) {
      perror("fork");
      exit(1);
    }
    if (pid > 0)
      exit(0);       /* Parent returns */
    setsid();        /* No error check: it's not really important */
  }

  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, interrupt);
  signal(SIGTERM, interrupt);
  signal(SIGUSR1, reset);

  do_game(&server);

  DEBUG("Server exitting...\n");
  srv_close(&server);

  DEBUG("Number of bytes sent:      %d\n", msg_sent_bytes);
  DEBUG("Number of bytes received:  %d\n", msg_received_bytes);

  return 0;
}
