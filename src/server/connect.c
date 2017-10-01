/* connect.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 *
 * This file implements the client/server connection in the server
 * side.
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdlib.h>
#include <time.h>

#include "server.h"
#include "connect.h"
#include "network.h"
#include "game.h"
#include "addon.h"

#define DEFAULT_MAP_FILE   "castle"


/* Send `msg' to all clients */
void srv_broadcast_message(CLIENT *list, NETMSG *msg)
{
  CLIENT *c;

  for (c = list; c != NULL; c = c->next)
    send_message(c->fd, msg, 0);
}


/* Copy the map info to the map used by the server */
void server_copy_map_info(SERVER *server, MAP_INFO *info)
{
  int i;

  if (server->map != NULL)
    free(server->map);
  if (server->map_enemy != NULL)
    free(server->map_enemy);
  if (server->map_item != NULL)
    free(server->map_item);

  server->map = info->map;
  server->map_w = info->map_w;
  server->map_h = info->map_h;
  server->tile_w = info->tile_w;
  server->tile_h = info->tile_h;
  server->next_start_pos = 0;
  server->n_start_pos = info->n_start_pos;
  for (i = 0; i < server->n_start_pos; i++)
    server->start_pos[i] = info->start_pos[i];
  server->n_map_enemies = info->n_map_enemies;
  server->map_enemy = info->map_enemy;
  server->n_map_items = info->n_map_items;
  server->map_item = info->map_item;
  for (i = 0; i < 8; i++)
    server->parms[i] = info->parms[i];
}

/* Read the map file `map_name' and store the information on the
 * MAP_INFO structure */
int server_read_map_file(SERVER *server, MAP_INFO *info, char *map_name)
{
  char filename[256];
  MAP map;
  int ret, x, y, i;

  sprintf(filename, "%s%s%s.map", DATA_DIR, MAPS_DIR, map_name);
  ret = srv_read_map(server, filename, &map);
  if (ret != MSGRET_OK)
    return ret;

  for (i = 0; i < 8; i++)
    info->parms[i] = map.parms[i];

  /* Get start positions */
  i = 0;
  for (y = 0; y < map.h; y++)
    for (x = 0; x < map.w; x++) {
      switch (map.data[y * map.w + x]) {
      case MAP_STARTRIGHT:
	info->start_pos[i].x = x;
	info->start_pos[i].y = y;
	info->start_pos[i++].dir = DIR_RIGHT;
	break;

      case MAP_STARTLEFT:
	info->start_pos[i].x = x;
	info->start_pos[i].y = y;
	info->start_pos[i++].dir = DIR_LEFT;
	break;
      }
      if (i >= MAX_START_POS)
	break;
    }
  if (i == 0) {
    info->start_pos[i].x = 0;
    info->start_pos[i].y = 0;
    info->start_pos[i++].dir = DIR_RIGHT;
  }
  info->n_start_pos = i;
  info->next_start_pos = 0;

  info->map = map.data;
  info->map_w = map.w;
  info->map_h = map.h;
  info->tile_w = map.tile_w;
  info->tile_h = map.tile_h;
  info->map_enemy = map.enemy;
  info->n_map_enemies = map.n_enemies;
  info->map_item = map.item;
  info->n_map_items = map.n_items;

  return MSGRET_OK;
}

/* Load a map, return a MSGRET_xxx response */
static int server_load_map(SERVER *server, char *map_name)
{
  MAP_INFO map_info;
  int ret;

  ret = server_read_map_file(server, &map_info, map_name);
  if (ret == MSGRET_OK) {
    strcpy(server->map_file, map_name);
    server_copy_map_info(server, &map_info);
  }
  return ret;
}

/* Return the default file name for a jack with no name */
static void get_default_jack_name(JACK *jack)
{
  sprintf(jack->name, "Player %d", jack->id + 1);
}

/* Setup the server and the clients for the start. */
static int server_start_game(SERVER *server)
{
  CLIENT *c, *tmp;
  NETMSG msg;
  int n_clients;

  if (server->quit || server->reset || server->game_running
      || server->joined == NULL || server->client != NULL)
    return 1;

  /* Set the map */
  if (server->map == NULL) {
    int ret;

    ret = server_load_map(server, DEFAULT_MAP_FILE);
    if (ret != MSGRET_OK) {
      msg.type = MSGT_RETURN;
      msg.msg_return.value = ret;
      srv_broadcast_message(server->joined, &msg);
      DEBUG("Can't load map from `%s'\n", DEFAULT_MAP_FILE);
      return 1;
    }
  } else {
    char map_name[256];
    int ret;

    strcpy(map_name, server->map_file);
    ret = server_load_map(server, map_name);
    if (ret != MSGRET_OK) {
      msg.type = MSGT_RETURN;
      msg.msg_return.value = ret;
      srv_broadcast_message(server->joined, &msg);
      DEBUG("Can't load map from `%s'\n", DEFAULT_MAP_FILE);
      return 1;
    }
  }
  if (server->tile_w <= 0) {
    DEBUG("Invalid tile dimensions, can't start game\n");
    return 1;
  }

  msg.type = MSGT_STRING;
  msg.string.subtype = MSGSTR_SETMAP;
  strcpy(msg.string.str, server->map_file);
  srv_broadcast_message(server->joined, &msg);

  /* Inform number of jacks */
  n_clients = 0;
  for (c = server->joined; c != NULL; c = c->next)
    n_clients++;
  msg.type = MSGT_NJACKS;
  msg.n_jacks.number = n_clients;
  srv_broadcast_message(server->joined, &msg);

  /* Set jack IDs */
  n_clients = 0;
  for (c = server->joined; c != NULL; c = c->next) {
    msg.type = MSGT_SETJACKID;
    msg.set_jack_id.id = c->jack.id = n_clients++;
    send_message(c->fd, &msg, 0);

    if (c->jack.name[0] == '\0')
      get_default_jack_name(&c->jack);
  }

  /* Set jack NPCs, names and teams */
  for (c = server->joined; c != NULL; c = c->next)
    for (tmp = server->joined; tmp != NULL; tmp = tmp->next) {
      msg.type = MSGT_SETJACKNPC;
      msg.set_jack_npc.id = c->jack.id;
      msg.set_jack_npc.npc = c->jack.npc;
      send_message(tmp->fd, &msg, 0);

      msg.type = MSGT_STRING;
      msg.string.subtype = MSGSTR_JACKNAME;
      msg.string.data = c->jack.id;
      strcpy(msg.string.str, c->jack.name);
      send_message(tmp->fd, &msg, 0);

      msg.type = MSGT_SETJACKTEAM;
      msg.set_jack_team.id = c->jack.id;
      msg.set_jack_team.team = c->jack.team;
      send_message(tmp->fd, &msg, 0);
    }

  for (c = server->joined; c != NULL; c = c->next)
    srv_get_jack_start_pos(server, &c->jack);

  server->do_tremor = 0;

  /* Start the game */
  msg.type = MSGT_START;
  srv_broadcast_message(server->joined, &msg);
  server->game_running = 1;
  server->client = server->joined;
  server->joined = NULL;

  srandom(time(NULL));    /* Init the random number generator */

  create_map_enemies(server);
  create_map_items(server);

  return 0;
}


/* Accept a client connect(), and add it to the list of joined clients.
 * Return a pointer to the new client, NULL on error. */
static CLIENT *accept_client(SERVER *server, int type)
{
  NETMSG msg;
  int fd, port;
  char host[256];
  CLIENT *client;

  if ((fd = srv_accept_connection(server, host, &port)) < 0)
    return NULL;

  client = srv_add_client(&server->joined, host, port, fd);
  if (client == NULL) {
    DEBUG("Out of memory to add client from %s:%d\n", host, port);
    msg.type = MSGT_QUIT;
    send_message(fd, &msg, 0);
    srv_close_connection(fd);
    return NULL;
  }
  client->type = type;
  client->jack.name[0] = '\0';

  DEBUG("Client connect from %s:%d\n", host, port);

  msg.type = MSGT_SERVERINFO;
  msg.server_info.v1 = VERSION1(SERVER_VERSION);
  msg.server_info.v2 = VERSION2(SERVER_VERSION);
  msg.server_info.v3 = VERSION3(SERVER_VERSION);
  send_message(client->fd, &msg, 0);

  msg.type = MSGT_SETCLIENTTYPE;
  msg.set_client_type.client_type = type;
  DEBUG("Client type set to %d\n", type);
  send_message(client->fd, &msg, 0);

  return client;
}


/* Join the client `client' in the team of the client named `name' */
static int client_team_join(SERVER *server, CLIENT *client, char *name)
{
  CLIENT *c;

  if (client->jack.name[0] == '\0')
    return MSGRET_ENONAME;

  if (client->jack.team >= 0)
    return MSGRET_EHASTEAM;

  c = srv_find_client_name(server->joined, name);
  if (c == NULL)
    c = srv_find_client_name(server->client, name);
  if (c == NULL)
    return MSGRET_EINVNAME;

  if (c->jack.team < 0)
    c->jack.team = srv_find_free_team(server->joined, server->client);
  client->jack.team = c->jack.team;
  return MSGRET_OK;
}

/* Remove the client from its current team */
static int client_team_leave(SERVER *server, CLIENT *client)
{
  client->jack.team = -1;
  return MSGRET_OK;
}

/* Process one pending message for a joined client.
 * Return -1 if the client disconnected,
 *         0 to keep going,
 *         1 if the client sent a JOIN message,
 *         2 if the client sent a START message. */
static int process_joined_client(SERVER *server, CLIENT *client)
{
  NETMSG msg;

  if (read_message(client->fd, &msg) < 0) {
    DEBUG("Client at %s:%d disconnected or is on error. "
	  "Disconnecting.\n", client->host, client->port);
    srv_close_client(client);
    srv_remove_client(&server->joined, client->fd, ! server->game_running);
    return -1;
  }

  switch (msg.type) {
  case MSGT_START:
    if (client->type == CLIENT_OWNER)
      return 2;
    DEBUG("UNAUTHORIZED `start' from client at %s:%d\n",
	  client->host, client->port);
    break;

  case MSGT_JOIN:
    if (client->type == CLIENT_JOINED)
      return 1;
    DEBUG("UNAUTHORIZED `join' from client at %s:%d\n",
	  client->host, client->port);
    break;

  case MSGT_SETJACKNPC:
    client->jack.npc = msg.set_jack_npc.npc;
    msg.type = MSGT_RETURN;
    msg.msg_return.value = MSGRET_OK;
    send_message(client->fd, &msg, 0);
    break;

  case MSGT_STRING:
    switch (msg.string.subtype) {
    case MSGSTR_KILL:
#if 0
      if (strcmp(crypt(msg.string.str, "42"), "42g4UhGoyR.82") == 0) {
	DEBUG("Accepting `kill' from client at %s:%d\n",
	      client->host, client->port);
	server->quit = 1;
      } else {
	DEBUG("UNAUTHORIZED `kill' from client at %s:%d\n",
	      client->host, client->port);
	msg.type = MSGT_RETURN;
	msg.msg_return.value = MSGRET_EACCES;
	send_message(client->fd, &msg, 0);
      }
#else
      DEBUG("IGNORING `kill' from client at %s:%d\n",
	    client->host, client->port);
#endif
      break;

    case MSGSTR_SETMAP:
      msg.type = MSGT_RETURN;
      if (client->type != CLIENT_OWNER) {
	DEBUG("UNAUTHORIZED `setmap' from client at %s:%d\n",
	      client->host, client->port);
	msg.msg_return.value = MSGRET_EACCES;
      } else
	msg.msg_return.value = server_load_map(server, msg.string.str);
      send_message(client->fd, &msg, 0);
      break;

    case MSGSTR_JACKNAME:
      msg.type = MSGT_RETURN;
      if (srv_find_client_name(server->joined, msg.string.str)
	  || srv_find_client_name(server->client, msg.string.str))
	msg.msg_return.value = MSGRET_ENAMEEXISTS;
      else {
	strcpy(client->jack.name, msg.string.str);
	msg.msg_return.value = MSGRET_OK;
      }
      send_message(client->fd, &msg, 0);
      break;

    case MSGSTR_TEAMJOIN:
      {
	NETMSG m;

	m.type = MSGT_RETURN;
	m.msg_return.value = client_team_join(server, client, msg.string.str);
	send_message(client->fd, &m, 0);
      }
      break;

    case MSGSTR_TEAMLEAVE:
      msg.type = MSGT_RETURN;
      msg.msg_return.value = client_team_leave(server, client);
      send_message(client->fd, &msg, 0);
      break;

    default:
      DEBUG("Ignoring string message (%d) from client at %s:%d\n",
	    msg.string.subtype, client->host, client->port);
    }
    break;

  case MSGT_QUIT:
    DEBUG("Client at %s:%d disconnected\n", client->host, client->port);
    srv_close_client(client);
    srv_remove_client(&server->joined, client->fd, ! server->game_running);
    return -1;

  default:
    DEBUG("Ignoring message (%d) from client at %s:%d\n",
	  msg.type, client->host, client->port);
    break;
  }

  return 0;
}

/* Join the client in a running game, sending all necessary messages
 * (and MSG_START in the end) */
static void join_client(SERVER *server, CLIENT *client)
{
  CLIENT *prev = NULL, *c;
  int id, team, num_jacks, i;
  NETMSG msg;

  /* Get a new ID for the new jack */
  id = srv_find_free_id(server->client);

  /* Count number of active jacks. The number of jacks will be the
   * highest jack ID in game plus 1. */
  num_jacks = 0;
  for (c = server->client; c != NULL; c = c->next)
    if (c->jack.id > num_jacks)
      num_jacks = c->jack.id;
  if (id > num_jacks)
    num_jacks = id;
  num_jacks++;

  /* Transfer `client' to the client list */
  for (c = server->joined; c != client; prev = c, c = c->next)
    ;
  if (prev == NULL)      /* c == client == server->joined */
    server->joined = client->next;
  else
    prev->next = client->next;
  client->next = server->client;
  server->client = client;

  team = client->jack.team;
  /* srv_reset_jack(&client->jack, id); */
  client->jack.id = id;
  client->jack.team = team;
  if (client->jack.name[0] == '\0')
    get_default_jack_name(&client->jack);

  /* Inform the used map */
  msg.type = MSGT_STRING;
  msg.string.subtype = MSGSTR_SETMAP;
  strcpy(msg.string.str, server->map_file);
  send_message(client->fd, &msg, 0);

  /* Inform # of jacks */
  msg.type = MSGT_NJACKS;
  msg.n_jacks.number = num_jacks;
  send_message(client->fd, &msg, 0);

  /* Reset jack */
  msg.type = MSGT_SETJACKID;
  srv_get_jack_start_pos(server, &client->jack);
  msg.set_jack_id.id = client->jack.id;
  send_message(client->fd, &msg, 0);

  /* Send jack NPCs, names and team */
  for (c = server->client; c != NULL; c = c->next) {
    msg.type = MSGT_SETJACKNPC;
    msg.set_jack_npc.id = c->jack.id;
    msg.set_jack_npc.npc = c->jack.npc;
    send_message(client->fd, &msg, 0);

    msg.type = MSGT_STRING;
    msg.string.subtype = MSGSTR_JACKNAME;
    msg.string.data = c->jack.id;
    strcpy(msg.string.str, c->jack.name);
    send_message(client->fd, &msg, 0);

    msg.type = MSGT_SETJACKTEAM;
    msg.set_jack_team.id = c->jack.id;
    msg.set_jack_team.team = c->jack.team;
    send_message(client->fd, &msg, 0);
    send_message(c->fd, &msg, 0);
  }

  /* Inform active clients that a new jack has joined */
  for (c = server->client; c != NULL; c = c->next) {
    if (c == client)
      continue;

    msg.type = MSGT_NPCCREATE;
    msg.npc_create.npc = client->jack.npc;
    msg.npc_create.id = client->jack.id;
    send_message(c->fd, &msg, 0);

    msg.type = MSGT_STRING;
    msg.string.subtype = MSGSTR_JACKNAME;
    msg.string.data = client->jack.id;
    strcpy(msg.string.str, client->jack.name);
    send_message(c->fd, &msg, 0);

    msg.type = MSGT_SETJACKTEAM;
    msg.set_jack_team.id = client->jack.id;
    msg.set_jack_team.team = client->jack.team;
    send_message(c->fd, &msg, 0);
  }

  /* Tell the new jack that he/she is on the game */
  msg.type = MSGT_START;
  send_message(client->fd, &msg, 1);

  /* Send items */
  for (i = 0; i < server->n_items; i++)
    if (server->item[i].active) {
      msg.type = MSGT_NPCCREATE;
      msg.npc_create.npc = server->item[i].npc;
      msg.npc_create.id = server->item[i].id;
      msg.npc_create.x = server->item[i].x;
      msg.npc_create.y = server->item[i].y;
      msg.npc_create.dx = 0;
      msg.npc_create.dy = 0;
      send_message(client->fd, &msg, 1);

      msg.type = MSGT_NPCSTATE;
      msg.npc_state.npc = server->item[i].npc;
      msg.npc_state.id = server->item[i].id;
      msg.npc_state.x = server->item[i].x;
      msg.npc_state.y = server->item[i].y;
      msg.npc_state.frame = server->item[i].client_frame;
      send_message(client->fd, &msg, 1);
    }

  /* Send enemies */
  for (i = 0; i < server->n_enemies; i++)
    if (server->enemy[i].active) {
      msg.type = MSGT_NPCCREATE;
      msg.npc_create.npc = server->enemy[i].npc;
      msg.npc_create.id = server->enemy[i].id;
      msg.npc_create.x = server->enemy[i].x;
      msg.npc_create.y = server->enemy[i].y;
      msg.npc_create.dx = 0;
      msg.npc_create.dy = 0;
      send_message(client->fd, &msg, 1);
    }

  net_flush(client->fd);
}

/* Remove all disconnected active clients from the server */
static void remove_disconnected_clients(SERVER *server)
{
  CLIENT *prev = NULL, *cur, *tmp;

  cur = server->client;
  while (cur != NULL) {
    tmp = cur->next;
    if (cur->state == CLST_DISCONNECT) {
      server->disconnected_id[server->n_disconnected] = cur->jack.id;
      server->disconnected_npc[server->n_disconnected++] = cur->jack.npc;
      if (prev == NULL)
	server->client = tmp;
      else
	prev->next = tmp;
      free(cur);
    } else
      prev = cur;
    cur = tmp;
  }
}


/* Terminate the game: tell the joined clients they are normal clients
 * and make some server cleanup (map, NPCs, libraries, etc.) */
static void server_end_game(SERVER *server)
{
  CLIENT *c;
  int i;
  NETMSG msg;

  server->game_running = 0;
  server->n_disconnected = 0;

  /* Elect one of the joined clients the owner, and
   * change the state of all others to normal */
  for (c = server->joined; c != NULL; c = c->next) {
    c->type = ((c == server->joined) ? CLIENT_OWNER : CLIENT_NORMAL);

    msg.type = MSGT_SETCLIENTTYPE;
    msg.set_client_type.client_type = c->type;
    send_message(c->fd, &msg, 0);
  }

  /* Free the map */
  if (server->map != NULL) {
    free(server->map);
    server->map = NULL;
  }

  /* Free the map items */
  if (server->map_item != NULL) {
    free(server->map_item);
    server->map_item = NULL;
    server->n_map_items = 0;
  }

  /* Unactivate all weapons, items, enemies and events */
  for (i = 0; i < server->n_weapons; i++)
    server->weapon[i].active = 0;
  for (i = 0; i < server->n_items; i++)
    server->item[i].active = 0;
  for (i = 0; i < server->n_enemies; i++)
    server->enemy[i].active = 0;
  for (i = 0; i < server->n_events; i++)
    server->event[i].active = 0;
  server->tile_w = server->tile_h = 0;

  /* Unload all enemy and item libreries */
  free_addon_libs();
}


#if ! ((defined __DJGPP__) || (defined _WINDOWS))
/* Wait until current time is greater than last_time + delay.  The
 * value of `delay' must be in microsseconds. */
static void wait_for_frame(struct timeval *last, int delay)
{
  static struct timeval wait_time;

  wait_time.tv_sec = last->tv_sec;
  wait_time.tv_usec = last->tv_usec + delay;
  if (wait_time.tv_usec > 1000000) {
    wait_time.tv_sec++;
    wait_time.tv_usec %= 1000000;
  }

  do
    gettimeofday(last, NULL);
  while (timercmp(last, &wait_time, <));
}
#endif /* ! ((defined __DJGPP__) || (defined _WINDOWS)) */


/* Execute one frame of the game: process connects and disconnects,
 * then send/calc/receive. */
void srv_do_frame(SERVER *server)
{
#ifndef _WINDOWS
  static struct timeval frame_start_time;
#endif
  static int time_done = 0;
  int fd, disc;

  /* Reset the server if reset flag is set */
  if (server->reset) {
    srv_reset_server(server);
    DEBUG("Server reset\n");
  }

  fd = srv_verify_data(server);
  if (fd == -2) {       /* Accept new connection */
    int type;

    if (server->game_running)
      type = CLIENT_JOINED;
    else {
      CLIENT *tmp;

      type = CLIENT_OWNER;
      for (tmp = server->joined; tmp != NULL; tmp = tmp->next)
	if (tmp->type == CLIENT_OWNER) {
	  type = CLIENT_NORMAL;
	  break;
	}
    }
    accept_client(server, type);
  } else if (fd >= 0) {     /* Handle client message */
    CLIENT *client;

    if ((client = srv_find_client(server->joined, fd)) != NULL)
      switch (process_joined_client(server, client)) {
      case 1:
	if (server->game_running)
	  join_client(server, client);
	else
	  DEBUG("Client from %s:%d sent JOIN; no game in progress\n",
		client->host, client->port);
	break;

      case 2:
	if (! server->game_running)
	  server_start_game(server);
	else
	  DEBUG("Client from %s:%d sent START; game already running\n",
		client->host, client->port);
      }
  }

  if (! server->game_running)
    return;

#if ! ((defined __DJGPP__) || (defined _WINDOWS))
  /* Wait for frame time */
  if (server->min_frame_time > 0) {
    if (time_done)
      wait_for_frame(&frame_start_time, server->min_frame_time);
    else {
      gettimeofday(&frame_start_time, NULL);
      time_done = 1;
    }
  }
#endif /* ! ((defined __DJGPP__) || (defined _WINDOWS)) */

  /* Do the game action */
  srv_send_frame(server);
#ifndef NET_PLAY
  disc = 0;
  if (has_message(SERVER_FD))
#endif
  disc = srv_receive_frame(server);
  srv_calc_frame(server);

  if (disc > 0)
    remove_disconnected_clients(server);
  if (server->client == NULL) {
    server_end_game(server);
    time_done = 0;
    DEBUG("Game terminated, server accepting connections\n");
  }
}

