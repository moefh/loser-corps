/* etank.c
 *
 * Library for the `energy tank' item.
 */

#include <stdio.h>

#include <../server/server.h>

void get_item(SERVER *server, ITEM *it, CLIENT *client)
{
  client->jack.n_energy_tanks++;
  client->jack.f_energy_tanks++;
  client->jack.changed = 1;
  it->destroy = 1;
}

