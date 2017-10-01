/* client.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "game.h"
#include "common.h"
#include "commands.h"
#include "network.h"


int client_send_message(int fd, NETMSG *msg, int use_buffer)
{
  switch (server->type) {
  case CONNECT_NETWORK:
    return send_message(fd, msg, use_buffer);

  case CONNECT_PLAYBACK:
    return 0;

  default:
    return 0;
  }
}

int client_has_message(int fd)
{
  switch (server->type) {
  case CONNECT_NETWORK:
    return has_message(fd);

  case CONNECT_PLAYBACK:
    return 1;

  default:
    return 0;
  }
}

int client_read_message(int fd, NETMSG *msg)
{
  switch (server->type) {
  case CONNECT_NETWORK:
    return read_message(fd, msg);

  case CONNECT_PLAYBACK:
    if (read_message(fd, msg) < 0)
      msg->type = MSGT_QUIT;     /* Simulate a server QUIT */
    return 0;

  default:
    msg->type = MSGT_NOP;
    return 0;
  }
}


static void set_jack(MSG_NPCSTATE *msg)
{
  NPC_INFO *npc;
  JACK *j;

  /* It's a jack we don't know of -- probably
   * because we were out of memory when it was added */
  if (msg->id >= n_jacks)
    return;

  npc = npc_info + jacks[msg->id].npc;
  j = jacks + msg->id;

  /* Shift the trail queue */
  if (game_frame % JACK_TRAIL_MARK == 0) {
    int i;

    for (i = j->trail_len - 1; i >= 0; i--) {
      j->trail_x[i + 1] = j->trail_x[i];
      j->trail_y[i + 1] = j->trail_y[i];
      j->trail_f[i + 1] = j->trail_f[i];
    }
    if (j->trail_len < JACK_TRAIL_LEN-2)
      j->trail_len++;
  }

  j->scr_x = msg->x;
  j->scr_y = msg->y;
  j->y = msg->y - npc->clip_y;
  j->frame = msg->frame;
  j->state_flags = msg->flags;

  if (npc_is_dir_left(msg->frame, npc))
    j->x = msg->x + (npc->clip_x + npc->clip_w - j->bmp[0]->w - 1);
  else
    j->x = msg->x - npc->clip_x;

  if (game_frame % JACK_TRAIL_MARK == 0) {
    j->trail_x[0] = j->x;
    j->trail_y[0] = j->y;
    j->trail_f[0] = (npc->frame[jack->frame]
		     + ((jack->state_flags&NPC_STATE_SHOOT)
			? npc->shoot_frame : 0));
  }
}

void set_jack_name(SERVER *server, int id, char *name)
{
  /* It's a jack we don't know of -- probably
   * because we were out of memory when it was added */
  if (id >= n_jacks)
    return;

  strcpy(jacks[id].name, name);
  clear_xbitmap(jacks[id].name_bmp, transp_color);
  clear_xbitmap(jacks[id].score_bmp, transp_color);
  draw_text(jacks[id].name_bmp, font_name, 0, 0, name);
  draw_text(jacks[id].score_bmp, font_score, 0, 0, "0");
  color_bitmap(jacks[id].name_bmp, JACK_COLOR(id));

  if (server->game_running)
    add_color_bmp(jacks[id].name_bmp, add_color);
}

static void set_jack_score(int id, int score)
{
  static char score_txt[8];

  /* It's a jack we don't know of -- probably
   * because we were out of memory when it was added */
  if (id >= n_jacks)
    return;

  sprintf(score_txt, "%d", score);

  clear_xbitmap(jacks[id].score_bmp, transp_color);
  draw_text(jacks[id].score_bmp, font_score, 0, 0, score_txt);
}

static void set_jack_energy(int id, int energy,
			    int f_tanks, int n_tanks, int power)
{
  if (id == player->id) {
    player->energy = energy;
    player->f_energy_tanks = f_tanks;
    player->n_energy_tanks = n_tanks;
    player->power = power;
  }
}

static void set_jack_invisible(int id, int invisible)
{
  if (id >= n_jacks)
    return;
  jacks[id].invisible = invisible;
}

static void set_jack_invincible(int id, int invincible)
{
  if (id >= n_jacks)
    return;
  jacks[id].invincible = invincible;
}

static void set_jack_message(int id, char *msg)
{
  char *p;
  int text_y;       /* Vertical text offset from the top of the bitmap */

  /* Calculate the text offset */
  {
    int text_h = 0;

    for (p = msg; *p != '\0'; p++)
      if (*p == '|')
	text_h += font_msg->bmp[(unsigned char) *p]->h + 2;
    text_h += font_msg->bmp[(unsigned char) msg[0]]->h + 2;
    if ((text_y = (player->msg_bmp->h - text_h) / 2) < 0)
      text_y = 0;
  }

  if (SCREEN_BPP == 1)
    clear_xbitmap(player->msg_bmp, XBMP8_TRANSP);
  else
    clear_xbitmap(player->msg_bmp,
		  (convert_16bpp_to == 15) ? XBMP15_TRANSP : XBMP16_TRANSP);

  /* Draw the text */
  {
    int x, y, w, h;

    p = msg;
    y = text_y;
    x = 0;
    while (*p != '\0') {
      while (*p != '\0' && (p == msg || *p == '|')) {
	char *tmp;
	int w = 0;

	if (*p == '|') {
	  y += font_msg->bmp[(unsigned char) *p]->h + 2;
	  p++;
	}
	for (tmp = p; *tmp != '|' && *tmp != '\0'; tmp++)
	  w += font_msg->bmp[(unsigned char) *tmp]->w;
	if ((x = (player->msg_bmp->w - w) / 2) < 0)
	  x = 0;
	if (p == msg)
	  break;
      }
      if (*p == '\0')
	break;

      w = font_msg->bmp[(unsigned char) *p]->w;
      h = font_msg->bmp[(unsigned char) *p]->h;
      if (x + w < player->msg_bmp->w && y + h < player->msg_bmp->h)
	copy_xbitmap(player->msg_bmp, x, y,
		     font_msg->bmp[(unsigned char) *p]);
      x += w;
      p++;
    }
  }
  color_bitmap(player->msg_bmp, JACK_COLOR(id) + add_color);

  /* Set the display information */
  player->msg_left = 160;
  player->msg_from = id;
}


void remove_jack(int id)
{
  /* It's a jack we don't know of -- probably
   * because we were out of memory when it was added */
  if (id >= n_jacks)
    return;

  jacks[id].active = 0;
  jacks[id].scr_x = jacks[id].x = -2 * tile[0]->w;
  jacks[id].scr_y = jacks[id].y = -2 * tile[0]->h;
  jacks[id].frame = 0;
}

/* Receive messages for the client. Return 1 if the server disconnected. */
int client_receive(SERVER *server)
{
  NETMSG msg;
  int done = 0;

  unset_missiles();
  do {
    if (client_read_message(server->fd, &msg) < 0) {
      printf("Server disconnected or is on error.\n");
      return 1;
    }

    switch (msg.type) {
    case MSGT_TREMOR:
      start_tremor(msg.tremor.x, msg.tremor.y,
		   msg.tremor.duration, msg.tremor.intensity);
      break;

    case MSGT_SOUND:
#ifdef HAS_SOUND
      if (jack->sound >= 0)
	play_game_sound(jack->sound, jack->sound_x, jack->sound_y);
      jack->sound = msg.sound.sound;
      jack->sound_x = msg.sound.x;
      jack->sound_y = msg.sound.y;
#endif /* HAS_SOUND */
      break;

    case MSGT_ENEMYHIT:
      player->hit_enemy = ENEMY_ENERGY_FRAMES;
      player->enemy_npc = msg.enemy_hit.npc;
      player->enemy_id = msg.enemy_hit.id;
      player->enemy_energy = msg.enemy_hit.energy;
      player->enemy_score = msg.enemy_hit.score;
      if (IS_NPC_JACK(player->enemy_npc)) {
	set_jack_energy(player->enemy_id, player->enemy_energy, -1, -1, -1);
	set_jack_score(player->enemy_id, player->enemy_score);
	jacks[player->enemy_id].blink = 1;
      } else {
	NPC *n;

	n = find_npc_id(msg.enemy_hit.npc, msg.enemy_hit.id);
	if (n != NULL)
	  n->blink = 1;
      }
      break;

    case MSGT_SETJACKTEAM:
      if (msg.set_jack_team.id >= n_jacks)
	printf("\nUnexpected server inconsistency: invalid jack id (%d)\n",
	       msg.set_jack_team.id);
      else
	jacks[msg.set_jack_team.id].team = msg.set_jack_team.team;
      break;

    case MSGT_JACKSTATE:
      set_jack_energy(player->id, msg.jack_state.energy,
		      msg.jack_state.f_energy_tanks,
		      msg.jack_state.n_energy_tanks,
		      msg.jack_state.power);
      if (player->score != msg.jack_state.score) {
        player->score = msg.jack_state.score;
        set_jack_score(player->id, player->score);
      }
      break;

    case MSGT_NPCINVISIBLE:
      if (IS_NPC_JACK(msg.npc_invisible.npc))
	set_jack_invisible(msg.npc_invisible.id, msg.npc_invisible.invisible);
      break;

    case MSGT_NPCINVINCIBLE:
      if (IS_NPC_JACK(msg.npc_invincible.npc))
	set_jack_invincible(msg.npc_invincible.id,
			    msg.npc_invincible.invincible);
      break;

    case MSGT_STRING:
      switch (msg.string.subtype) {
      case MSGSTR_SETMAP:
	if (client_change_map(msg.string.str))
	  return 1;
	break;

      case MSGSTR_JACKNAME:
	set_jack_name(server, msg.string.data, msg.string.str);
	break;

      case MSGSTR_MESSAGE:
	set_jack_message(msg.string.data, msg.string.str);
	break;
      }
      break;

    case MSGT_NPCCREATE:
      if (IS_NPC_JACK(msg.npc_create.npc)) {
	if (join_jack(msg.npc_create.id, msg.npc_create.npc))
	  printf("OUT OF MEMORY to add player\n");
      } else {
	NPC *npc;

	npc = allocate_npc(msg.npc_create.npc, msg.npc_create.id);
	if (npc == NULL)
	  printf("OUT OF MEMORY for NPC (npc,id) = (%d,%d)\n",
		 msg.npc_create.npc, msg.npc_create.id);
	else {
	  npc->dx = msg.npc_create.dx;
	  npc->dy = msg.npc_create.dy;
	  npc->dx2 = msg.npc_create.dx2;
	  npc->level = msg.npc_create.level;
	  npc->moved = 1;
	  npc->s_frame = 0;
	  npc->frame = 0;
	  npc->blink = 0;
	  set_npc_pos(npc, msg.npc_create.npc,
		      msg.npc_create.x, msg.npc_create.y);
	}
      }
      break;

    case MSGT_NPCSTATE:
      if (IS_NPC_JACK(msg.npc_state.npc)) {
	set_jack(&msg.npc_state);
	if (msg.npc_state.id == player->id)
	  done = 1;
      } else
	set_npc(&msg);
      break;

    case MSGT_NPCREMOVE:
      if (IS_NPC_JACK(msg.npc_remove.npc))
	remove_jack(msg.npc_remove.id);
      else
	free_npc(msg.npc_remove.npc, msg.npc_remove.id);
      break;

    case MSGT_QUIT:
      return 1;

    default:
      printf("Ignoring message %d\n", msg.type);
    }
  } while (! done);

  move_missiles();

  return 0;
}

/* Send a key to the server */
void client_send_key(SERVER *server, int scancode, int press)
{
  MSG_KEYPRESS msg;

  if (! server->game_running)
    return;

  if (scancode == key_config.right)
    msg.scancode = KEY_RIGHT;
  else if (scancode == key_config.left)
    msg.scancode = KEY_LEFT;
  else if (scancode == key_config.jump)
    msg.scancode = KEY_JUMP;
  else if (scancode == key_config.down)
    msg.scancode = KEY_DOWN;
  else if (scancode == key_config.shoot)
    msg.scancode = KEY_SHOOT;
  else if (scancode == key_config.use)
    msg.scancode = KEY_USE;
  else
    return;

  msg.type = MSGT_KEYPRESS;
  msg.press = press;
  client_send_message(server->fd, (NETMSG *) &msg, 1);
}

/* Tell the server we're done sending keys */
void client_send_key_end(SERVER *server)
{
  MSG_KEYEND msg;

  msg.type = MSGT_KEYEND;
  client_send_message(server->fd, (NETMSG *) &msg, 1);
}
