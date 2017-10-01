/* teleport.c
 *
 * Copyright (C) 1999 Ricardo R. Massaro
 *
 * Library for the `teleport' item.
 */

#include <stdio.h>
#include <string.h>

#include <../server/server.h>


void get_item(SERVER *server, ITEM *it, CLIENT *client)
{
  int sfx_teleport = -1;

  if (it->text[0] != '\0') {
    if (server_read_map_file(server, &server->new_map_info, it->text)
	!= MSGRET_OK) {
      DEBUG("Can't read map `%s'\n", it->text);
      it->target_x = -1;
      it->target_y = -1;
      it->text[0] = '\0';      /* Don't try to read this map again */
    } else {
      server->change_map = 2;
      strcpy(server->map_file, it->text);
      sfx_teleport = sfx_id(server, "teleport3");
    }
  } else {
    if (it->target_x >= 0 && it->target_y >= 0) {
      client->jack.x = it->target_x;
      client->jack.y = it->target_y;
      client->jack.dx = client->jack.dy = 0;
      client->jack.state = STATE_JUMP_END;
      sfx_teleport = sfx_id(server, "teleport1");
    }
  }
  if (sfx_teleport >= 0) {
    SOUND *s;
    s = srv_allocate_sound(server);
    if (s != NULL) {
      s->sfx = sfx_teleport;
      s->x = client->jack.x;
      s->y = client->jack.y;
    }
  }
}
