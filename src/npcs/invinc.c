/* invinc.c
 *
 * Library for the `invincibility' item.
 */

#include <stdio.h>

#include <../server/server.h>


void get_item(SERVER *server, ITEM *it, CLIENT *client)
{
  client->jack.start_invincible = 1;
  client->jack.invincible = srv_sec2frame(it->duration);
  it->destroy = 1;
}
