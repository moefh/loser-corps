/* powerup.c
 *
 * Library for the `power up' item.
 */

#include <stdio.h>

#include <../server/server.h>


void get_item(SERVER *server, ITEM *it, CLIENT *client)
{
  if (client->jack.power_level >= WEAPON_LEVELS - 1)
    return;          /* Already full */
  client->jack.power_level++;
  client->jack.changed = 1;
  it->destroy = 1;
}

