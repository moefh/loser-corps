/* energy.c
 *
 * Library for the `energy' item.
 */

#include <stdio.h>

#include <../server/server.h>

void get_item(SERVER *server, ITEM *it, CLIENT *client)
{
  srv_jack_get_energy(server, &client->jack, 20);
  it->destroy = 1;
}

