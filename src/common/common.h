/* common.h
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#ifndef COMMON_H_FILE
#define COMMON_H_FILE

#define LOCAL_DATA_DIR  DATADIR "/data/"

#define SRV_VERSION(a,b,c)  ((((a)<<16)&0xff0000) | (((b)<<8)&0xff00) | ((c)&0xff))
#define VERSION1(v)     (((v)&0xff0000)>>16)
#define VERSION2(v)     (((v)&0x00ff00)>>8)
#define VERSION3(v)     ((v)&0x0000ff)

#define SERVER_VERSION   SRV_VERSION(0,9,11)

/* You must change these so that `net_int' is a 16 bit integer
 * and `net_char' is a one byte (8 bits) integer */
typedef short int net_int;
typedef char net_char;


#ifndef INLINE
#ifdef __GNUC__
#define INLINE inline
#else /* __GNUC__ */
#define INLINE
#endif /* __GNUC__ */
#endif /* INLINE */

/* Dummy file descriptors for local network emulation */
#define CLIENT_FD  512
#define SERVER_FD  513




#ifdef _WINDOWS

#include <io.h>
#define F_OK  0     /* for access() */
#define access(path, mode)   _access((path), (mode))
#define open(fd, flags)      _open((fd), (flags))
#define read(fd, buf, len)   _read((fd), (buf), (len))
#define write(fd, buf, len)  _write((fd), (buf), (len))
#define close(fd)            _close(fd)

#endif /* _WINDOWS */



#if (! defined _WINDOWS)

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>

#endif


#if ((! defined __DJGPP__) && (! defined _WINDOWS))

#include <sys/signal.h>

#endif



/*
 * Keys and directories
 */

#if ((defined __DJGPP__) || (defined _WINDOWS))

/* These are defined by Allegro: */
#undef KEY_UP
#undef KEY_DOWN
#undef KEY_LEFT
#undef KEY_RIGHT
#undef MSG_START
#undef MSG_STRING

#define MAPS_DIR        "maps\\"
#define IMAGE_DIR_MASK  "%dbpp\\"
#define DIRECTORY_SEP   '\\'

#else /* ((defined __DJGPP__) || (defined _WINDOWS)) */

#define MAPS_DIR        "maps/"
#define IMAGE_DIR_MASK  "%dbpp/"
#define DIRECTORY_SEP   '/'

#endif /* ((defined __DJGPP__) || (defined _WINDOWS)) */



#define DEFAULT_PORT     5042
#define MAX_STRING_LEN   256    /* Maximum string size in a message */
#define MAX_NPCS         256    /* Maximum # of NPCs, may be increased */
#define MAX_WEAPONS      5      /* Maximum # of weapon types per jack */


enum {       /* NPC types */
  NPC_TYPE_JACK,
  NPC_TYPE_WEAPON,
  NPC_TYPE_ENEMY,
  NPC_TYPE_ITEM,

  NUM_NPC_TYPES
};

enum {       /* NPC State flags */
  NPC_STATE_NORMAL = 0,
  NPC_STATE_SHOOT  = 1<<0,
};

enum {       /* NPC item flags */
  NPC_ITEMFLAG_NONE   = 0,
  NPC_ITEMFLAG_WATER  = 1<<0,
  NPC_ITEMFLAG_WIND   = 1<<1,
};

#define IS_MAP_BLOCK(tile) ((tile) >= MAP_BLOCK && (tile) <= MAP_BLOCK14)

/* Start numbers for NPC codes of objects on the map file.
 * You may change the numbers (but note that doing so will
 * cause all existing map files to be invalid) but
 * DO*NOT*CHANGE*THE*ORDER!!! -- the code depends on it. */
#define MAP_ENEMY_START    1000
#define MAP_ITEM_START     2000
#define MAP_MAP_TEXT       10000


enum {
  MAP_BLOCK,
  MAP_BLOCK1,
  MAP_BLOCK2,
  MAP_BLOCK3,
  MAP_BLOCK4,
  MAP_BLOCK5,
  MAP_BLOCK6,
  MAP_BLOCK7,
  MAP_BLOCK8,
  MAP_BLOCK9,
  MAP_BLOCK10,
  MAP_BLOCK11,
  MAP_BLOCK12,
  MAP_BLOCK13,
  MAP_BLOCK14,

  MAP_SECRET,

  MAP_STARTRIGHT,
  MAP_STARTLEFT,

  N_MAP_BLOCKS,
};

enum {       /* Client types */
  CLIENT_NORMAL,       /* Normal client */
  CLIENT_OWNER,        /* Owner of the server */
  CLIENT_JOINED        /* Connected during a game */
};

enum {       /* Keys */
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_JUMP,
  KEY_SHOOT,
  KEY_USE,

  NUM_KEYS
};


typedef struct MSG_NOP {
  net_int type;
} MSG_NOP;

typedef struct MSG_STRING {
  net_int type;
  net_int subtype;
  net_int data;
  net_char str[MAX_STRING_LEN];
} MSG_STRING;

typedef struct MSG_RETURN {
  net_int type;
  net_int value;            /* MSGRET_xxx: return value from request */
} MSG_RETURN;

typedef struct MSG_START {
  net_int type;
} MSG_START;

typedef struct MSG_JOIN {
  net_int type;
} MSG_JOIN;

typedef struct MSG_QUIT {
  net_int type;
} MSG_QUIT;

typedef struct MSG_SERVERINFO {
  net_int type;
  net_int v1;              /* Major version number */
  net_int v2;              /* Middle version number */
  net_int v3;              /* Minor version number */
} MSG_SERVERINFO;

typedef struct MSG_SETCLIENTTYPE {
  net_int type;
  net_int client_type;        /* CLIENT_xxx */
} MSG_SETCLIENTTYPE;

typedef struct MSG_SETJACKID {
  net_int type;
  net_int id;                 /* Jack ID */
} MSG_SETJACKID;

typedef struct MSG_SETJACKNPC {
  net_int type;
  net_int id;                 /* Jack ID */
  net_int npc;                /* Jack NPC */
} MSG_SETJACKNPC;

typedef struct MSG_SETJACKTEAM {
  net_int type;
  net_int id;                 /* Jack ID */
  net_int team;               /* Jack team */
} MSG_SETJACKTEAM;

typedef struct MSG_NPCCREATE {
  net_int type;
  net_int npc;          /* NPC_xxx */
  net_int id;           /* NPC id */
  net_int x, y;         /* Initial position */
  net_int dx, dy, dx2;  /* Speed & accel (only for missiles) */
  net_int level;        /* Power level (only for missiles) */
} MSG_NPCCREATE;

typedef struct MSG_JACKSTATE {
  net_int type;
  net_int energy;             /* Jack energy */
  net_int f_energy_tanks;     /* # of full energy tanks */
  net_int n_energy_tanks;     /* # of energy tanks */
  net_int score;              /* Jack score */
  net_int power;              /* # of power-ups */
} MSG_JACKSTATE;

typedef struct MSG_ENEMYHIT {
  net_int type;
  net_int npc;          /* Enemy NPC */
  net_int id;           /* Enemy ID */
  net_int energy;       /* Enemy energy after hit */
  net_int score;        /* Enemy score (before dying, if will die) */
} MSG_ENEMYHIT;

typedef struct MSG_NPCSTATE {
  net_int type;
  net_int npc;          /* NPC_xxx */
  net_int id;           /* NPC id (which jack, etc.) */
  net_int x, y;         /* Position */
  net_int frame;        /* Frame */
  net_int flags;        /* Additional state flags NPC_STATE_* */
} MSG_NPCSTATE;

typedef struct MSG_NPCINVISIBLE {
  net_int type;
  net_int npc;          /* NPC */
  net_int id;           /* NPC id */
  net_int invisible;    /* 0 if visible, 1 if invisible */
} MSG_NPCINVISIBLE;

typedef struct MSG_NPCINVINCIBLE {
  net_int type;
  net_int npc;          /* NPC */
  net_int id;           /* NPC id */
  net_int invincible;
} MSG_NPCINVINCIBLE;

typedef struct MSG_NPCREMOVE {
  net_int type;
  net_int npc;          /* NPC_xxx */
  net_int id;           /* NPC id */
} MSG_NPCREMOVE;

typedef struct MSG_NJACKS {
  net_int type;
  net_int number;       /* Number of jacks in game */
} MSG_NJACKS;

typedef struct MSG_TREMOR {
  net_int type;
  net_int duration;     /* Tremor duration (in frames) */
  net_int intensity;    /* Tremor intensity */
  net_int x, y;         /* Position of the source ((-1,-1) if no source) */
} MSG_TREMOR;

typedef struct MSG_SOUND {
  net_int type;
  net_int sound;        /* Sound # */
  net_int x, y;         /* Position where the sound was generated */
} MSG_SOUND;

typedef struct MSG_KEYPRESS {
  net_int type;
  net_int scancode;     /* Scan code */
  net_int press;        /* 1 if pressed, -1 if released */
} MSG_KEYPRESS;

typedef struct MSG_KEYEND {
  net_int type;
} MSG_KEYEND;



enum {      /* Message types */
  MSGT_NOP,                /*  No-op                server <-> client    */
  MSGT_STRING,             /*  String               server <-> client    */
  MSGT_RETURN,             /*  Return request       server  -> client    */
  MSGT_START,              /*  Start the game       server <-> client    */
  MSGT_JOIN,               /*  Join current game    server <-  client    */
  MSGT_QUIT,               /*  Quit the game        server <-> client    */
  MSGT_SERVERINFO,         /*  Server information   server  -> client    */
  MSGT_SETCLIENTTYPE,      /*  Set client type      server  -> client    */
  MSGT_SETJACKID,          /*  Set jack id          server  -> client    */
  MSGT_SETJACKTEAM,        /*  Set jack team        server  -> client    */
  MSGT_SETJACKNPC,         /*  Set jack NPC         server <-  client    */
  MSGT_NPCCREATE,          /*  Create NPC           server  -> client    */
  MSGT_NPCSTATE,           /*  Send NPC state       server  -> client    */
  MSGT_NPCINVISIBLE,       /*  Set NPC invisible    server  -> client    */
  MSGT_NPCINVINCIBLE,      /*  Set NPC invincible   server  -> client    */
  MSGT_NPCREMOVE,          /*  Remove NPC           server  -> client    */
  MSGT_JACKSTATE,          /*  Send jack state      server  -> client    */
  MSGT_ENEMYHIT,           /*  Enemy was hit        server  -> client    */
  MSGT_NJACKS,             /*  Number of jacks      server  -> client    */
  MSGT_TREMOR,             /*  Screen tremor        server  -> client    */
  MSGT_SOUND,              /*  Make sound           server  -> client    */
  MSGT_KEYPRESS,           /*  Key was pressed      server <-  client    */
  MSGT_KEYEND              /*  No more keys         server  -> client    */
};


enum {
  MSGRET_OK,              /* OK, request accepted */
  MSGRET_EMAPFILE,        /* Map file not found */
  MSGRET_EMAPFMT,         /* Map file has invalid format */
  MSGRET_EACCES,          /* Permission denied */
  MSGRET_EMEM,            /* Out of memory */
  MSGRET_ENAMEEXISTS,     /* Name already used */
  MSGRET_EHASTEAM,        /* Already in a team */
  MSGRET_ENONAME,         /* Must have name to do this */
  MSGRET_EINVNAME         /* Invalid client name */
};

enum {      /* Types for MSGT_STRING messages */
  MSGSTR_SETMAP,           /* Set current map */
  MSGSTR_KILL,             /* Kill server (sending password) */
  MSGSTR_JACKNAME,         /* Set jack name (data=jack id) */
  MSGSTR_TEAMJOIN,         /* Join to a team */
  MSGSTR_TEAMLEAVE,        /* Leave a team (no string) */
  MSGSTR_MESSAGE           /* Inter-client message */
};


typedef union NETMSG {
  net_int type;

  MSG_NOP nop;
  MSG_STRING string;
  MSG_RETURN msg_return;
  MSG_START start;
  MSG_JOIN join;
  MSG_QUIT quit;
  MSG_SERVERINFO server_info;
  MSG_SETCLIENTTYPE set_client_type;
  MSG_SETJACKID set_jack_id;
  MSG_SETJACKTEAM set_jack_team;
  MSG_SETJACKNPC set_jack_npc;
  MSG_NPCCREATE npc_create;
  MSG_JACKSTATE jack_state;
  MSG_NPCINVISIBLE npc_invisible;
  MSG_NPCINVINCIBLE npc_invincible;
  MSG_ENEMYHIT enemy_hit;
  MSG_NPCSTATE npc_state;
  MSG_NPCREMOVE npc_remove;
  MSG_NJACKS n_jacks;
  MSG_TREMOR tremor;
  MSG_SOUND sound;
  MSG_KEYPRESS key_press;
  MSG_KEYEND key_end;
} NETMSG;


extern FILE *debug_file, *in_record_file, *out_record_file;
extern int msg_sent_bytes;
extern int msg_received_bytes;
extern int use_out_buffer;       /* 1 to use output buffer */

#define DATA_DIR  loser_data_dir
extern char loser_data_dir[];

char *err_msg(void);
void DEBUG(const char *fmt, ...);

int has_message(int fd);
int read_message(int fd, NETMSG *msg);
int accept_message(int fd, NETMSG *msg);
int send_message(int fd, NETMSG *msg, int use_buffer);
int net_flush(int fd);

void select_loser_data_dir(int force_local);

#endif /* COMMON_H_FILE */
