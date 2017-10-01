/* send.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 *
 * Functions to send a all information of 1 frame
 * of the game to all clients.
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "server.h"
#include "game.h"


/* If set to 1, jack position is sent to all clients even if the jack
 * is invisible. This allows the client to do the predator effect, but
 * a player could change the client's code and see invisible jacks. */
#define SEND_INVISIBLE_POS  1


/* Send NPCs and jack info to a client */
static int send_client(SERVER *server, CLIENT *client)
{
  CLIENT *c;
  int i;
  NETMSG msg;

  if (server->quit || server->reset)
    return 0;

  /* Inform about disconnected clients */
  for (i = 0; i < server->n_disconnected; i++) {
    msg.type = MSGT_NPCREMOVE;
    msg.npc_state.npc = server->disconnected_npc[i];
    msg.npc_state.id = server->disconnected_id[i];
    send_message(client->fd, &msg, 1);
  }


  if (server->do_tremor) {
    msg.type = MSGT_TREMOR;
    msg.tremor.x = server->tremor.x;
    msg.tremor.y = server->tremor.y;
    msg.tremor.duration = server->tremor.duration;
    msg.tremor.intensity = server->tremor.intensity;
    send_message(client->fd, &msg, 1);
  }
#ifdef HAS_SOUND
  {
    SOUND *s;

    /* Inform about sounds */
    for (s = server->sound, i = 0; i < server->n_sounds; i++, s++)
      if (s->active && (s->client_id < 0 || s->client_id == client->jack.id)) {
	msg.type = MSGT_SOUND;
	msg.sound.sound = s->sfx;
	msg.sound.x = s->x;
	msg.sound.y = s->y;
	send_message(client->fd, &msg, 1);
      }
  }
#endif
  {
    WEAPON *w;

    /* Inform about weapons */
    for (w = server->weapon, i = 0; i < server->n_weapons; i++, w++) {
      if (! w->active)
	continue;

      if (w->create) {
	msg.type = MSGT_NPCCREATE;
	msg.npc_create.npc = w->npc;
	msg.npc_create.id = w->id;
	msg.npc_create.x = w->x;
	msg.npc_create.y = w->y;
	msg.npc_create.dx = w->dx;
	msg.npc_create.dy = w->dy;
	msg.npc_create.dx2 = w->dx2;
	msg.npc_create.level = w->level;
	send_message(client->fd, &msg, 1);
      } else if (w->destroy) {
	msg.type = MSGT_NPCREMOVE;
	msg.npc_remove.npc = w->npc;
	msg.npc_remove.id = w->id;
	send_message(client->fd, &msg, 1);
#ifdef HAS_SOUND
	if (! server->change_map && sfx_explosion >= 0) {
	  msg.type = MSGT_SOUND;
	  msg.sound.sound = sfx_explosion;
	  msg.sound.x = w->x;
	  msg.sound.y = w->y;
	  send_message(client->fd, &msg, 1);
	}
#endif
      }
    }
  }

  {
    ENEMY *e;

    /* Inform about enemies */
    for (e = server->enemy, i = 0; i < server->n_enemies; i++, e++) {
      if (! e->active)
	continue;

      if (e->create) {
	msg.type = MSGT_NPCCREATE;
	msg.npc_create.npc = e->npc;
	msg.npc_create.id = e->id;
	msg.npc_create.x = e->x;
	msg.npc_create.y = e->y;
	msg.npc_create.dx = e->dx;
	msg.npc_create.dy = e->dy;
	msg.npc_create.dx2 = -1;
	msg.npc_create.level = 0;
	send_message(client->fd, &msg, 1);
      } else if (e->destroy) {
	msg.type = MSGT_NPCREMOVE;
	msg.npc_remove.npc = e->npc;
	msg.npc_remove.id = e->id;
	send_message(client->fd, &msg, 1);
      } else {
	msg.type = MSGT_NPCSTATE;
	msg.npc_state.npc = e->npc;
	msg.npc_state.id = e->id;
	msg.npc_state.x = e->x;
	msg.npc_state.y = e->y;
	msg.npc_state.frame = e->client_frame;
	send_message(client->fd, &msg, 1);
      }
    }
  }

  {
    ITEM *it;

    /* Inform about items */
    for (it = server->item, i = 0; i < server->n_items; i++, it++) {
      if (! it->active)
	continue;

      if (it->create) {
	msg.type = MSGT_NPCCREATE;
	msg.npc_create.npc = it->npc;
	msg.npc_create.id = it->id;
	msg.npc_create.x = it->x;
	msg.npc_create.y = it->y;
	msg.npc_create.dx = 0;
	msg.npc_create.dy = 0;
	msg.npc_create.dx2 = -1;
	msg.npc_create.level = 0;
	send_message(client->fd, &msg, 1);
      } else if (it->destroy) {
	msg.type = MSGT_NPCREMOVE;
	msg.npc_remove.npc = it->npc;
	msg.npc_remove.id = it->id;
	send_message(client->fd, &msg, 1);
#ifdef HAS_SOUND
      if (! server->change_map && sfx_blip >= 0) {
	  msg.type = MSGT_SOUND;
	  msg.sound.sound = sfx_blip;
	  msg.sound.x = it->x;
	  msg.sound.y = it->y;
	  send_message(client->fd, &msg, 1);
	}
#endif
     } else if (it->changed) {
	msg.type = MSGT_NPCSTATE;
	msg.npc_state.npc = it->npc;
	msg.npc_state.id = it->id;
	msg.npc_state.x = it->x;
	msg.npc_state.y = it->y;
	msg.npc_state.frame = it->client_frame;
	send_message(client->fd, &msg, 1);
      }
    }
  }

  /* Inform new state */
  if (client->jack.changed) {
    client->jack.changed = 0;
    msg.type = MSGT_JACKSTATE;
    msg.jack_state.energy = client->jack.energy;
    msg.jack_state.n_energy_tanks = client->jack.n_energy_tanks;
    msg.jack_state.f_energy_tanks = client->jack.f_energy_tanks;
    msg.jack_state.score = client->jack.score;
    msg.jack_state.power = client->jack.power_level;
    send_message(client->fd, &msg, 1);
  }

  /* Inform that an enemy was hit */
  if (client->jack.hit_enemy) {
    client->jack.hit_enemy = 0;
    msg.type = MSGT_ENEMYHIT;
    msg.enemy_hit.npc = client->jack.enemy.npc;
    msg.enemy_hit.id = client->jack.enemy.id;
    msg.enemy_hit.energy = client->jack.enemy.energy;
    msg.enemy_hit.score = client->jack.enemy.score;
    send_message(client->fd, &msg, 1);
  }

  /* Inform about other jacks */
  for (c = server->client; c != NULL; c = c->next) {
    if (c->state != CLST_ALIVE)
      continue;       /* Skip dead jacks */

    if (c->jack.msg_to == client->jack.id) {
      msg.type = MSGT_STRING;
      msg.string.subtype = MSGSTR_MESSAGE;
      msg.string.data = c->jack.id;
      strcpy(msg.string.str, c->jack.msg_str);
      send_message(client->fd, &msg, 1);
    }

    if (c->jack.start_invisible != 0) {   /* Start or terminate invisibility */
      msg.type = MSGT_NPCINVISIBLE;
      msg.npc_invisible.npc = c->jack.npc;
      msg.npc_invisible.id = c->jack.id;
      msg.npc_invisible.invisible = (c->jack.start_invisible < 0) ? 0 : 1;
      send_message(client->fd, &msg, 1);
    }
    if (c->jack.start_invincible != 0) { /* Start or terminate invincibility */
      msg.type = MSGT_NPCINVINCIBLE;
      msg.npc_invincible.npc = c->jack.npc;
      msg.npc_invincible.id = c->jack.id;
      msg.npc_invincible.invincible = (c->jack.start_invincible < 0) ? 0 : 1;
      send_message(client->fd, &msg, 1);
    }

    if (c->jack.id == client->jack.id)
      continue;       /* Skip the client's jack */

#ifdef HAS_SOUND
    if (c->jack.sound >= 0) {
      msg.type = MSGT_SOUND;
      msg.sound.sound = c->jack.sound;
      msg.sound.x = c->jack.x;
      msg.sound.y = c->jack.y;
      send_message(client->fd, &msg, 1);
    }
#endif
    /* Send jack position only if not invisible */
#if SEND_INVISIBLE_POS == 0
    if (c->jack.invisible < 0) {
#endif /* SEND_INVISIBLE_POS == 0 */
      msg.type = MSGT_NPCSTATE;
      msg.npc_state.npc = c->jack.npc;
      msg.npc_state.id = c->jack.id;
      msg.npc_state.x = c->jack.x;
      msg.npc_state.y = c->jack.y;
      msg.npc_state.frame = c->jack.client_frame;
      msg.npc_state.flags = c->jack.state_flags;
      send_message(client->fd, &msg, 1);
#if SEND_INVISIBLE_POS == 0
    }
#endif /* SEND_INVISIBLE_POS == 0 */
  }

#ifdef HAS_SOUND
  if (client->jack.sound >= 0) {
    msg.type = MSGT_SOUND;
    msg.sound.sound = client->jack.sound;
    msg.sound.x = client->jack.x;
    msg.sound.y = client->jack.y;
    send_message(client->fd, &msg, 1);
  }
#endif
  /* Inform about player's jack */
  msg.type = MSGT_NPCSTATE;
  msg.npc_state.npc = client->jack.npc;
  msg.npc_state.id = client->jack.id;
  msg.npc_state.x = client->jack.x;
  msg.npc_state.y = client->jack.y;
  msg.npc_state.frame = client->jack.client_frame;
  msg.npc_state.flags = client->jack.state_flags;
  send_message(client->fd, &msg, 1);

  net_flush(client->fd);

  if (client->jack.shoot_keep > 0 && --client->jack.shoot_keep == 0)
    client->jack.state_flags &= ~NPC_STATE_SHOOT;
  return 1;
}


/* Send a frame of the game to all clients */
void srv_send_frame(SERVER *server)
{
  CLIENT *client;
  int i;

  /* Send clients */
  for (client = server->client; client != NULL; client = client->next)
    send_client(server, client);
  server->n_disconnected = 0;

  /* Remove sounds and tremor effect*/
  server->do_tremor = 0;
#ifdef HAS_SOUND
  for (i = 0; i < server->n_sounds; i++)
    if (server->sound[i].active)
      srv_free_sound(server, server->sound + i);
#endif
  /* Unmark weapons to create/destroy */
  for (i = 0; i < server->n_weapons; i++)
    if (server->weapon[i].active) {
      if (server->weapon[i].create)
	server->weapon[i].create = 0;
      else if (server->weapon[i].destroy)
	srv_free_weapon(server, server->weapon + i);
    }

  /* Unmark enemies to create/destroy */
  for (i = 0; i < server->n_enemies; i++)
    if (server->enemy[i].active) {
      if (server->enemy[i].create)
	server->enemy[i].create = 0;
      else if (server->enemy[i].destroy)
	srv_free_enemy(server, server->enemy + i);
    }

  /* Unmark items to create/destroy/change */
  for (i = 0; i < server->n_items; i++)
    if (server->item[i].active) {
      if (server->item[i].create)
	server->item[i].create = 0;
      else if (server->item[i].destroy)
	srv_free_item(server, server->item + i);
      else if (server->item[i].changed)
	server->item[i].changed = 0;
    }

  /* Mark all messages sent */
  for (client = server->client; client != NULL; client = client->next)
    client->jack.msg_to = -1;
}
