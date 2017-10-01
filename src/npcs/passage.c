/* passage.c
 *
 * Copyright (C) 1999 Ricardo R. Massaro
 *
 * Library for the `passage' item.
 */

#include <stdio.h>
#include <string.h>

#include <../server/server.h>


static void set_jack_pos_to_item(SERVER *server, JACK *jack, ITEM *it)
{
  int bx, by;
  NPC_INFO *npc;

  npc = server->npc_info + jack->npc;

  bx = (it->target_x + server->tile_w / 2) / server->tile_w;
  by = (it->target_y + server->tile_h / 2) / server->tile_h;

  jack->x = (server->tile_w * bx + (server->tile_w - npc->clip_w) / 2);
  jack->y = (server->tile_h * by + server->tile_h - npc->clip_h - 1);
}

void get_item(SERVER *server, ITEM *it, CLIENT *client)
{
  int sfx_teleport = -1;

  if (! it->pressed)
    return;

  /* This prevents re-teleporting, if our target is a passage */
  client->jack.using_item = 0;

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
      set_jack_pos_to_item(server, &client->jack, it);
      client->jack.dx = client->jack.dy = 0;
      client->jack.state = STATE_JUMP_END;
      /* sfx_teleport = sfx_id(server, "teleport1"); */
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

