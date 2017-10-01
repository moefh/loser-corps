/* info.c
 *
 * Copyright (C) 1999 Ricardo R. Massaro
 *
 * This is the library for the `info' item.
 */

#include <stdio.h>
#include <string.h>

#include <../server/server.h>




/* This gets executed when a lever is touched */
void get_item(SERVER *server, ITEM *it, CLIENT *client)
{
  if (server->change_map || ! it->pressed || it->map_id < 0)
    return;

  client->jack.msg_to = client->jack.id;
  strcpy(client->jack.msg_str, it->text);
}
