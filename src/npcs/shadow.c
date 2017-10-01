/* shadow.c
 *
 * Library for the `invisibility' item.
 */

#include <stdio.h>

#include <../server/server.h>

void get_item(SERVER *server, ITEM *it, CLIENT *client)
{
  client->jack.start_invisible = 1;
  client->jack.invisible = srv_sec2frame(it->duration);
  it->destroy = 1;
}
