/* jack.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#include <stdio.h>

#include "server.h"


static SERVER *server;

/* Drain all energy from a jack.  This must be used only when it's
 * REALLY necessary.  An example would be an anvil hitting the jack
 * when he's in a pit */
void srv_kill_jack(SERVER *server, JACK *j)
{
  j->energy = j->f_energy_tanks = 0;
  j->dead = 1;
}

/* Give energy to a jack */
void srv_jack_get_energy(SERVER *server, JACK *jack, int energy)
{
#if 0
  if (jack->energy >= 100 && jack->f_energy_tanks == jack->n_energy_tanks)
    return;
#endif

  jack->changed = 1;
  jack->energy += energy;
  if (jack->energy > 100) {
    if (jack->f_energy_tanks < jack->n_energy_tanks) {
      jack->energy -= 100;
      jack->f_energy_tanks++;
    } else
      jack->energy = 100;
  }
}

/* Hit a jack */
void srv_hit_jack(SERVER *server, JACK *jack, int damage, int damage_type)
{
  if (jack->invincible >= 0)
    return;

  jack->changed = 1;
  jack->energy -= damage;
  if (jack->energy <= 0) {
    if (jack->f_energy_tanks > 0) {
      jack->f_energy_tanks--;
      jack->energy += 100;
    } else
      jack->dead = 1;
  }
}

/* Set the current jack frame correctly */
void jack_fix_frame(JACK *jack, NPC_INFO *npc)
{
  switch (jack->state) {
  case STATE_STAND:
    jack->frame %= (npc->n_stand/2);
    jack->client_frame = (jack->dir == DIR_LEFT) ? (npc->n_stand / 2) : 0;
    jack->client_frame += npc->s_stand + jack->frame;
    break;

  case STATE_WALK:
    jack->frame %= (npc->n_walk/2);
    jack->client_frame = (jack->dir == DIR_LEFT) ? (npc->n_walk / 2) : 0;
    jack->client_frame += npc->s_walk + jack->frame;
    break;

  case STATE_JUMP_START:
  case STATE_JUMP_END:
    jack->frame %= (npc->n_jump/2);
    jack->client_frame = (jack->dir == DIR_LEFT) ? (npc->n_jump / 2) : 0;
    jack->client_frame += npc->s_jump + jack->frame;
    break;

  default:
    jack->client_frame = 0;
  }
}


static void decrease_horizontal_speed(JACK *jack, int amount)
{
  int tmp;

  if (jack->dx >= 0)
    tmp = 1;
  else {
    jack->dx = -jack->dx;
    tmp = -1;
  }
  jack->dx -= amount;
  if (jack->dx <= 0) {
    jack->dx = 0;
    if (jack->state == STATE_WALK)
      jack->state = STATE_STAND;
  } else if (tmp < 0)
    jack->dx = -jack->dx;
}

/* Calculate the jack movement */
void jack_move(JACK *jack, NPC_INFO *npc, SERVER *server)
{
  int move_dx, move_dy, flags;

  if (jack->state == STATE_WALK || jack->state == STATE_STAND) {
    /* Check the floor under jack */
    calc_jack_movement(server, 0, jack->x, jack->y, npc->clip_w, npc->clip_h,
		       0, 1, &move_dx, &move_dy);
    if (move_dy > 0)
      jack->state = STATE_JUMP_END;    /* Start falling */
  }
  if (jack->state != STATE_WALK && jack->state != STATE_STAND)
    jack->dy += DEC_JUMP_SPEED;

  flags = calc_jack_movement(server, 0,
			     jack->x, jack->y, npc->clip_w, npc->clip_h,
			     jack->dx / 1000, jack->dy / 1000,
			     &move_dx, &move_dy);
  if (flags & CM_Y_CLIPPED) {
    if (jack->state != STATE_WALK && jack->state != STATE_STAND)
#ifdef HAS_SOUND
    if (sfx_bump >= 0)
	jack->sound = sfx_bump;
#endif
    if (jack->dy > 0) {      /* Hit the ground */
      jack->state = ((FRAME_DELAY < 1
		     || jack->key[KEY_LEFT] || jack->key[KEY_RIGHT])
		     ? STATE_WALK : STATE_STAND);
      jack->dy = 0;
    } else
      jack->dy = -jack->dy;
  }
  if (flags & CM_X_CLIPPED)
    decrease_horizontal_speed(jack, DEC_WALK_SPEED / 2);

  jack->x += move_dx;
  jack->y += move_dy;
  if (jack->x < 0)
    jack->x = 0;
  if (jack->y < 0)
    jack->y = 0;
  if (++jack->frame_delay >= FRAME_DELAY) {
    jack->frame++;
    jack->frame_delay = 0;
  }

  switch (jack->state) {
  case STATE_STAND:  /* ??? */
  case STATE_WALK:
    decrease_horizontal_speed(jack, DEC_WALK_SPEED);
    break;

  case STATE_JUMP_START:
    if (jack->dy < 0 && jack->dy > -DEC_JUMP_SPEED)
      jack->state = STATE_JUMP_END;
    if (jack->dy > MAX_JUMP_SPEED)
      jack->dy = MAX_JUMP_SPEED;
    break;

  case STATE_JUMP_END:
    if (jack->dy > MAX_JUMP_SPEED)
      jack->dy = MAX_JUMP_SPEED;
    break;
  }
}



static void input_stand(JACK *jack)
{
  int s, made_dir = 0;

  for (s = 0; s < NUM_KEYS; s++)
    if (jack->key[s])
      switch (s) {
      case KEY_RIGHT:
	if (made_dir)
	  break;
	made_dir = 1;
	jack->state = STATE_WALK;
	jack->dir = DIR_RIGHT;
	jack->dx += START_WALK_SPEED;
	jack->frame = 0;
	break;

      case KEY_LEFT:
	if (made_dir)
	  break;
	made_dir = 1;
	jack->state = STATE_WALK;
	jack->dir = DIR_LEFT;
	jack->dx -= START_WALK_SPEED;
	jack->frame = 0;
	break;

      case KEY_JUMP:
	if (jack->key_trans[KEY_JUMP] <= 0)
	  break;
	made_dir = 1;
	jack->key_trans[KEY_JUMP] = 0;
	jack->state = STATE_JUMP_START;
	jack->dy = -START_JUMP_SPEED;
	jack->frame = 0;
	break;
      }
}

static void input_walk(JACK *jack)
{
  int s, made_dir = 0;

  for (s = 0; s < NUM_KEYS; s++)
    if (jack->key[s])
      switch (s) {
      case KEY_RIGHT:
	if (made_dir)
	  break;
	made_dir = 1;
	jack->dir = DIR_RIGHT;
	jack->dx += INC_WALK_SPEED;
	if (jack->dx > MAX_WALK_SPEED)
	  jack->dx = MAX_WALK_SPEED;
	break;

      case KEY_LEFT:
	if (made_dir)
	  break;
	made_dir = 1;
	jack->dir = DIR_LEFT;
	jack->dx -= INC_WALK_SPEED;
	if (jack->dx < -MAX_WALK_SPEED)
	  jack->dx = -MAX_WALK_SPEED;
	break;

      case KEY_JUMP:
	if (jack->key_trans[KEY_JUMP] <= 0)
	  break;
	made_dir = 1;
	jack->key_trans[KEY_JUMP] = 0;
	jack->state = STATE_JUMP_START;
	jack->dy = -START_JUMP_SPEED;
	jack->frame = 0;
	break;
      }

  if (FRAME_DELAY > 0) {
    if (jack->key_trans[KEY_RIGHT] < 0) {
      jack->key_trans[KEY_RIGHT] = 0;
      jack->state = STATE_STAND;    /* ??? */
    }
    if (jack->key_trans[KEY_LEFT] < 0) {
      jack->key_trans[KEY_LEFT] = 0;
      jack->state = STATE_STAND;    /* ??? */
    }
  }
}

static void input_jump_start(JACK *jack)
{
  int s, made_dir = 0;

  for (s = 0; s < NUM_KEYS; s++)
    if (jack->key[s])
      switch (s) {
      case KEY_RIGHT:
	if (made_dir)
	  break;
	made_dir = 1;
	jack->dir = DIR_RIGHT;
	jack->dx += INC_WALK_JUMP_SPEED;
	if (jack->dx > MAX_WALK_SPEED)
	  jack->dx = MAX_WALK_SPEED;
	break;

      case KEY_LEFT:
	if (made_dir)
	  break;
	made_dir = 1;
	jack->dir = DIR_LEFT;
	jack->dx -= INC_WALK_JUMP_SPEED;
	if (jack->dx < -MAX_WALK_SPEED)
	  jack->dx = -MAX_WALK_SPEED;
	break;

      case KEY_JUMP:
	jack->dy -= INC_JUMP_SPEED;
	break;
      }

  if (jack->key_trans[KEY_JUMP] < 0) {
    jack->key_trans[KEY_JUMP] = 0;
    jack->state = STATE_JUMP_END;
  }
}

static void input_jump_end(JACK *jack)
{
  int s, made_dir = 0;

  for (s = 0; s < NUM_KEYS; s++)
    if (jack->key[s])
      switch (s) {
      case KEY_RIGHT:
	if (made_dir)
	  break;
	made_dir = 1;
	jack->dir = DIR_RIGHT;
	jack->dx += INC_WALK_JUMP_SPEED;
	if (jack->dx > MAX_WALK_SPEED)
	  jack->dx = MAX_WALK_SPEED;
	break;

      case KEY_LEFT:
	if (made_dir)
	  break;
	made_dir = 1;
	jack->dir = DIR_LEFT;
	jack->dx -= INC_WALK_JUMP_SPEED;
	if (jack->dx < -MAX_WALK_SPEED)
	  jack->dx = -MAX_WALK_SPEED;
	break;
      }
}

/* Handle the input for a JACK structure. */
void jack_input(JACK *jack, SERVER *srv)
{
  server = srv;

#if 0
  /* This is for autofire */
  if (jack->shoot_delay > 0)
    jack->shoot_delay--;

  if (jack->key[KEY_SHOOT] && jack->shoot_delay <= 0) {
    jack->shoot_delay = 8;
    jack->weapon = 1;
  }
#else
  /* This is for normal shooting */
  if (jack->key_trans[KEY_SHOOT] > 0) {
    jack->key_trans[KEY_SHOOT] = 0;
    jack->weapon = 1;
  }
#endif

  switch (jack->state) {
  case STATE_STAND: input_stand(jack); break;
  case STATE_WALK: input_walk(jack); break;
  case STATE_JUMP_START: input_jump_start(jack); break;
  case STATE_JUMP_END: input_jump_end(jack); break;
  }
}
