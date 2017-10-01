/* water.c
 * Copyright (C) 1999 Ricardo R. Massaro
 *
 * Library for the `water' item.
 */

#include <stdio.h>

#include <../server/server.h>

void get_item(SERVER *server, ITEM *it, CLIENT *client)
{
  if (client->jack.item_flags & NPC_ITEMFLAG_WATER)
    return;
  client->jack.item_flags |= NPC_ITEMFLAG_WATER;

  if (! client->jack.under_water) {
    SOUND *s;
    int sfx_water_id;

    if ((sfx_water_id = sfx_id(server, "water")) >= 0) {
      s = srv_allocate_sound(server);
      if (s != NULL) {
	s->sfx = sfx_water_id;
	s->x = client->jack.x;
	s->y = client->jack.y;
      }
    }
  }
  client->jack.under_water = 2;

  if (it->duration < 0)
    return;
  if ((--client->jack.energy_delay) <= 0) {
    srv_hit_jack(server, &client->jack, 1, 0);
    client->jack.energy_delay = it->duration;
  }
}
