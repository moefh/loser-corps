/* wind.c
 * Copyright (C) 1999 Ricardo R. Massaro
 *
 * The `wind' item library.
 */

#include <stdio.h>
#include <../server/server.h>

#define ABS(x)  (((x) < 0) ? -(x) : (x))
#define DIR(x)  (((x) < 0) ? -1 : 1)


void get_item(SERVER *server, ITEM *it, CLIENT *client)
{
  if (client->jack.item_flags & NPC_ITEMFLAG_WIND)
    return;
  client->jack.item_flags |= NPC_ITEMFLAG_WIND;

  if (client->jack.state == STATE_WALK
      && ABS(client->jack.dx) < 2 * DEC_WALK_SPEED)
    ;
  else if (client->jack.state == STATE_STAND)
    client->jack.dx += (DEC_WALK_SPEED + 100) * DIR(it->level);
  else
    client->jack.dx += (DEC_WALK_SPEED - 50) * DIR(it->level);
}
