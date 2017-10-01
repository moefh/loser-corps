/* jack.h
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#ifndef JACK_H_FILE
#define JACK_H_FILE

#define ENERGY_RECOVER_TIME   32

typedef struct JACK JACK;

typedef struct JACK_ENEMY {
  int npc, id;       /* Enemy NPC, id */
  int energy;        /* Enemy energy */
  int score;         /* Score before death (if died) */
} JACK_ENEMY;

struct JACK {
  /* ID */
  int id;              /* ID # */
  int npc;             /* NPC # (which jack: stick, loser, punk...) */
  int team;            /* Team (< 0 means no team) */
  char name[MAX_STRING_LEN];       /* Jack name */
  
  /* Message */
  char msg_str[MAX_STRING_LEN];    /* Message string to send */
  int msg_to;          /* Who to send the message (-1 = none) */
  
  /* Attributes */
  int dead;             /* 1 if dead */
  int energy;           /* Energy */
  int f_energy_tanks;   /* # of full energy tanks */
  int n_energy_tanks;   /* # of energy tanks */
  int score;            /* Score */
  int shoot_delay;      /* Shoot delay (in frames) */
  int energy_delay;     /* Energy increase delay (in frames) */
  int start_invisible;  /* 1 to start invisibility, -1 to terminate */
  int start_invincible; /* 1 to start invincibility, -1 to terminate */
  int invincible;       /* if >= 0, the # of frames of invincibility left */
  int invisible;        /* if >= 0, the # of frames of invisibility left */
  int power_level;      /* Shoot destruction level */
  
  /* Position and state */
  int state;           /* STATE_xxx */
  int dir;             /* DIR_xxx */
  int x, y;            /* Position of the upper-left corner of rect */
  int dx, dy;          /* Direction (if walking or in a jump) */
  int frame_delay;     /* Frame delay */
  int frame;           /* Current frame */
  int state_flags;     /* State flags (NPC_STATE_*) */
  int client_frame;    /* Frame to send to the client */
  int sound;           /* >= 0 to send corresponding sound */
  int weapon;          /* > 0 if weapon was shot */
  int changed;         /* 1 if state (energy, etc.) has changed */
  int hit_enemy;       /* 1 if hit enemy */
  int using_item;      /* 1 if pressed button to use item */
  int under_water;     /* Used by the `water' items: 1 if touching water */
  int item_flags;      /* Item effect flags (NPC_ITEMFLAG_*) */
  int shoot_keep;      /* Frames left to keep shoot state */
  JACK_ENEMY enemy;
  unsigned char key[NUM_KEYS];
  signed char key_trans[NUM_KEYS];
};


void srv_kill_jack(SERVER *server, JACK *j);
void srv_jack_get_energy(SERVER *server, JACK *jack, int energy);
void srv_hit_jack(SERVER *server, JACK *jack, int damage, int damage_type);
void jack_move(JACK *, NPC_INFO *, SERVER *);
void jack_fix_frame(JACK *, NPC_INFO *);
void jack_input(JACK *, SERVER *);

int calc_movement(SERVER *server, int calc_all,
		  int x, int y, int w, int h, int dx, int dy,
		  int *ret_dx, int *ret_dy);
int calc_jack_movement(SERVER *server, int calc_all,
		       int x, int y, int w, int h,
		       int dx, int dy, int *ret_dx, int *ret_dy);

enum {       /* Directions */
  DIR_UP,
  DIR_RIGHT,
  DIR_DOWN,
  DIR_LEFT
};

enum {       /* Jack states */
  STATE_STAND,        /* Standing */
  STATE_WALK,         /* Walking */
  STATE_JUMP_START,   /* Starting jump (going up) */
  STATE_JUMP_END      /* Ending jump (going down) */
};

enum {      /* Flags returned by calc_movement() */
  CM_X_CLIPPED = 0x01,
  CM_Y_CLIPPED = 0x02
};

static INLINE int point_in_rect(int px, int py, int x, int y, int w, int h)
{
  return (px >= x && py >= y && px < x + w && py < y + h);
}

int is_map_blocked(SERVER *server, int x, int y, int w, int h);
int is_map_enemy_blocked(SERVER *server, int x, int y, int w, int h);


#endif /* JACK_H_FILE */
