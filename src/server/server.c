/* server.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 *
 * Server management functions. This file has functions to
 * add, remove and search for clients and to reset the server.
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#include "server.h"
#include "network.h"
#include "connect.h"


static void rec_free_client_list(CLIENT *list)
{
  if (list == NULL)
    return;
  rec_free_client_list(list->next);
  free(list);
}

/* Remove a list of clients */
void srv_free_client_list(CLIENT **list)
{
  rec_free_client_list(*list);
  *list = NULL;
}


static int team_used(CLIENT *list, int team)
{
  CLIENT *c;

  for (c = list; c != NULL; c = c->next)
    if (c->jack.team == team)
      return 1;
  return 0;
}

/* Return the first free team number */
int srv_find_free_team(CLIENT *l1, CLIENT *l2)
{
  int team = 0;

  while (team_used(l1, team) || team_used(l2, team))
    team++;
  return team;
}

/* Return the first free Jack ID number */
int srv_find_free_id(CLIENT *list)
{
  CLIENT *client;
  int id = 0;

  client = list;
  while (client != NULL)
    if (client->jack.id == id) {
      id++;
      client = list;
    } else
      client = client->next;
  return id;
}

/* Reset a jack structure for a client */
void srv_reset_jack(JACK *jack, int id)
{
  /* ID, message */
  jack->id = id;
  jack->npc = 0;
  jack->msg_to = -1;

  /* Weapon */
  jack->shoot_delay = 0;
  jack->weapon = 0;
  jack->hit_enemy = 0;
  jack->team = -1;

  /* Attributes */
  jack->energy = 100;
  jack->n_energy_tanks = 0;
  jack->f_energy_tanks = 0;
  jack->dead = 0;
  jack->energy_delay = 0;
  jack->score = 0;
  jack->power_level = 0;
  jack->invincible = -1;
  jack->invisible = -1;
  jack->start_invisible = 0;
  jack->start_invincible = 0;

  /* Position & status */
  jack->state_flags = 0;
  jack->dx = jack->dy = 0;
  jack->sound = -1;
  jack->under_water = 0;
  jack->using_item = 0;
  jack->frame_delay = 0;
  jack->client_frame = jack->frame = 0;
  jack->changed = 1;
  memset(jack->key, 0, sizeof(jack->key));
  memset(jack->key_trans, 0, sizeof(jack->key_trans));
}

/* Get a new start position to the jack */
void srv_get_jack_start_pos(SERVER *server, JACK *jack)
{
  NPC_INFO *npc;

  npc = server->npc_info + jack->npc;
  jack->x = (server->tile_w * server->start_pos[server->next_start_pos].x
	     + (server->tile_w - npc->clip_w) / 2);
  jack->y = (server->tile_h * server->start_pos[server->next_start_pos].y
	     + server->tile_h - npc->clip_h - 1);
  jack->dx = jack->dy = 0;
  jack->dir = server->start_pos[server->next_start_pos].dir;
  jack->state = STATE_JUMP_END;
  server->next_start_pos = (server->next_start_pos+1) % server->n_start_pos;
}


/* Add a new client.
 * Returns a pointer to the added client, NULL on error. */
CLIENT *srv_add_client(CLIENT **list, char *host, int port, int fd)
{
  CLIENT *c;

  c = (CLIENT *) malloc(sizeof(CLIENT));
  if (c == NULL)
    return NULL;

  /* Add the client */
  c->type = CLIENT_NORMAL;
  c->state = CLST_ALIVE;
  c->port = port;
  strcpy(c->host, host);
  c->fd = fd;
  c->next = *list;
  *list = c;

  srv_reset_jack(&c->jack, -1);    /* Unassigned ID */

  return c;
}


/* Remove a client from the `list' given its `fd'. If `elect_new_owner'
 * is not zero and the removed client is the owner, another client is
 * elected to be the new owner. */
void srv_remove_client(CLIENT **list, int fd, int elect_new_owner)
{
  CLIENT *cur, *prev = NULL;

  /* Look for the client in the list */
  for (cur = *list; cur != NULL; prev = cur, cur = cur->next)
    if (cur->fd == fd)
      break;

  if (cur == NULL)
    return;          /* Not found */

  if (prev == NULL)
    *list = cur->next;
  else
    prev->next = cur->next;

  /* Elect another owner (if there's a client left) */
  if (elect_new_owner && *list != NULL && cur->type == CLIENT_OWNER) {
    NETMSG msg;

    (*list)->type = CLIENT_OWNER;

    msg.type = MSGT_SETCLIENTTYPE;
    msg.set_client_type.client_type = CLIENT_OWNER;
    send_message((*list)->fd, &msg, 0);
  }

  free(cur);
}

/* Return the client in `list' whose fd is `fd' */
CLIENT *srv_find_client(CLIENT *list, int fd)
{
  CLIENT *c;

  for (c = list; c != NULL; c = c->next)
    if (c->fd == fd)
      return c;
  return NULL;
}

/* Return the client in `list' whose jack's id is `id' */
CLIENT *srv_find_client_id(CLIENT *list, int id)
{
  CLIENT *c;

  for (c = list; c != NULL; c = c->next)
    if (c->jack.id == id)
      return c;
  return NULL;
}

CLIENT *srv_find_client_name(CLIENT *list, char *name)
{
  CLIENT *c;

  for (c = list; c != NULL; c = c->next)
    if (strcmp(c->jack.name, name) == 0)
      return c;
  return NULL;
}

/* Reset the server: disconnect all clients, free all memory. */
void srv_reset_server(SERVER *server)
{
  NETMSG msg;
  CLIENT *c;

  /* Disconnect clients */
  msg.quit.type = MSGT_QUIT;
  srv_broadcast_message(server->client, &msg);
  srv_broadcast_message(server->joined, &msg);
  for (c = server->client; c != NULL; c = c->next)
    if (c->fd >= 0)
      srv_close_client(c);
  for (c = server->joined; c != NULL; c = c->next)
    if (c->fd >= 0)
      srv_close_client(c);

  /* Free clients */
  srv_free_client_list(&server->client);
  srv_free_client_list(&server->joined);

  server->change_map = 0;
  server->game_running = 0;
  server->reset = 0;
}
