/* torch.c
 *
 * Animated image "enemy".
 */

#include <stdio.h>

#include <../server/server.h>

void move_enemy(SERVER *server, ENEMY *e, NPC_INFO *npc)
{
  if (server->change_map)
    return;
  e->frame++;
}
