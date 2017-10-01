/* receive.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 *
 * Functions to receive information from all clients.
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "server.h"
#include "game.h"
#include "network.h"


/* Receive and process messages from a client.
 * Return -1 if the client disconnected. */
static int receive_client(SERVER *server, CLIENT *client)
{
  NETMSG msg;

  if (client->state != CLST_ALIVE || server->quit || server->reset)
    return 0;

  while (1) {
    if (read_message(client->fd, &msg) < 0) {
      DEBUG("Client %d (from %s:%d) on error or disconnected\n",
	    client->jack.id, client->host, client->port);
      client->state = CLST_DISCONNECT;
      srv_close_client(client);
      client->fd = -1;
      return 1;
    }

    switch (msg.type) {
    case MSGT_STRING:
      if (msg.string.subtype == MSGSTR_MESSAGE) {
	client->jack.msg_to = msg.string.data;
	strcpy(client->jack.msg_str, msg.string.str);
      }
      break;

    case MSGT_KEYPRESS:
      if (msg.key_press.scancode < 0 || msg.key_press.scancode > NUM_KEYS) {
	DEBUG("client %d sent an invalid key: %d\n",
	      client->jack.id, msg.key_press.scancode);
	break;
      }
      client->jack.key_trans[msg.key_press.scancode] =
	(unsigned char) msg.key_press.press;
      if (msg.key_press.press > 0)
	client->jack.key[msg.key_press.scancode] = 1;
      else
	client->jack.key[msg.key_press.scancode] = 0;
      break;
      
    case MSGT_KEYEND:
      return 0;
      
    case MSGT_QUIT:
      DEBUG("Client %d (from %s:%d) disconnected\n",
	    client->jack.id, client->host, client->port);
      client->state = CLST_DISCONNECT;
      srv_close_client(client);
      client->fd = -1;
      return 1;
      
    default:
      DEBUG("UNHANDLED message (%d) from client %d (%s:%d)\n",
	    msg.type, client->jack.id, client->host, client->port);
    }
  }
}


/* Receive a frame of the game from all clients */
int srv_receive_frame(SERVER *server)
{
  int disc = 0;
  CLIENT *client;

  server->n_disconnected = 0;
  for (client = server->client; client != NULL; client = client->next)
    disc += receive_client(server, client);
  return disc;
}
