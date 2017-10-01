/* startup.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 *
 * Code to handle connection, character selection and all
 * other stuff before the game starts.
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
#include <stdarg.h>

#ifndef GRAPH_WIN
#include <dirent.h>
#endif

#include "game.h"
#include "common.h"
#include "commands.h"
#include "network.h"


enum {                /* Main menu options */
  OPTION_CONNECT,
  OPTION_CHARACTER,
  OPTION_MAP,
  OPTION_TEAM,
  OPTION_PLAY,
  OPTION_QUIT,

  NUM_OPTIONS
};

typedef struct MAIN_SCREEN {
  SERVER *server;              /* Server */
  OPTIONS *options;            /* Host & port */

  int redraw;                  /* 1 if must redraw the options */
  int done;                    /* 1 if done */

  int available[256];          /* Available options */
  int selected;                /* Currently selected option */
  char jack_name[256];         /* Current jack name */
  int jack_npc;                /* Current jack NPC */
  char map[256];               /* Current map name */

  int client_type;             /* Current client type */
  int client_type_changed;     /* 1 if client type changed */

  XBITMAP *title, *background;
  XBITMAP *red, *black;
} MAIN_SCREEN;


#if (defined GRAPH_DOS) || (defined GRAPH_WIN)
#define DO_DELAY(usec)
#else  /* GRAPH_DOS || GRAPH_WIN */
#define DO_DELAY(usec)   usleep(usec)
#endif /* GRAPH_DOS || GRAPH_WIN */

static MAIN_SCREEN main_screen, *mains;


char *connect_error_msg(int value)
{
  switch (value) {
  case MSGRET_OK: return "OK.";
  case MSGRET_EMAPFILE: return "can't load map file.";
  case MSGRET_EMAPFMT: return "map file has invalid format.";
  case MSGRET_EMEM: return "out of memory.";
  case MSGRET_EACCES: return "permission denied.";
  case MSGRET_ENONAME: return "must have a name to do this.";
  case MSGRET_ENAMEEXISTS: return "name already exists.";
  case MSGRET_EHASTEAM: return "already part of a team.";
  case MSGRET_EINVNAME: return "invalid player name.";
  default: return "unknown error.";
  }
}

static void send_init_messages(SERVER *server, OPTIONS *options, int type)
{
  NETMSG msg;
  int i;

  if (options->name != NULL) {
    msg.type = MSGT_STRING;
    msg.string.subtype = MSGSTR_JACKNAME;
    strcpy(msg.string.str, options->name);
    send_message(server->fd, &msg, 0);
  }

  if (options->jack[0] != '\0') {
    for (i = 0; i < npc_number.num; i++)
      if (strcmp(npc_info[i].npc_name, options->jack) == 0) {
	msg.type = MSGT_SETJACKNPC;
	msg.set_jack_npc.id = 2;
	msg.set_jack_npc.npc = i;
	send_message(server->fd, &msg, 0);
	break;
      }
    if (npc_info[i].npc_name[0] == '\0')
      display_msg("Invalid jack name.\n");
  }

  if (type == CLIENT_OWNER && options->ask_map[0] != '\0') {
    msg.type = MSGT_STRING;
    msg.string.subtype = MSGSTR_SETMAP;
    strncpy(msg.string.str, options->ask_map, MAX_STRING_LEN-1);
    msg.string.str[MAX_STRING_LEN-1] = '\0';
    client_send_message(server->fd, &msg, 0);
  }

  if (options->do_go != 0)
    switch (type) {
    case CLIENT_OWNER:
      send_go_message(server->fd);
      break;
	  
    case CLIENT_JOINED:
      msg.type = MSGT_JOIN;
      client_send_message(server->fd, &msg, 0);
    }
}

/* Open record file (if any) and try to stabilish connection to the server.
 * Return 0 if the server was successfully opened. */
static int client_startup_conn(SERVER *server, OPTIONS *options)
{
  server->type = options->conn_type;
  strcpy(server->host, options->host);
  server->port = options->port;

  if (options->record_file != NULL) {
    if ((in_record_file = fopen(options->record_file, "wb")) == NULL) {
      display_msg("Can't open `%s':\n%s.\n\n",
		  options->record_file, err_msg());
      return 1;
    }
    write_rec_header(in_record_file);
    options->record_file = NULL;
  }

  switch (options->conn_type) {
  case CONNECT_PLAYBACK:
#ifdef GRAPH_DOS
    server->fd = open(options->playback_file, O_RDONLY | O_BINARY);
#elif (defined GRAPH_WIN)
    server->fd = open(options->playback_file, O_RDONLY | _O_BINARY);
#else
    server->fd = open(options->playback_file, O_RDONLY);
#endif /* GRAPH_DOS || GRAPH_WIN */
    if (server->fd < 0) {
      display_msg("Can't open file `%s':\n%s.\n\n",
		  options->playback_file, err_msg());
      return 1;
    }
    if (read_rec_header(server->fd) != 0) {
      display_msg("Can't read `%s':\nFile has invalid format.\n\n",
		  options->playback_file);
      close(server->fd);
      return 1;
    }
    options->conn_type = CONNECT_NETWORK;
    break;

  case CONNECT_NETWORK:
    if (client_open(server))
      return 1;
    break;

  default:
    display_msg("Invalid connection type: %d\n\n", options->conn_type);
    return 1;
  }

  return 0;
}


/********************************************************************/
/*** Menu ***********************************************************/



/* Read up to max_len bytes (including the null-terminator) of
 * text at position (x,y) of the screen. */
static int get_text(int x, int y, char *text, int max_len)
{
  static XBITMAP *empty_bmp;
  int pos, done = 0, key, redraw = 1, i;
  char buf[256];

  if (empty_bmp == NULL) {
    empty_bmp = create_xbitmap(8, 8, SCREEN_BPP);
    if (empty_bmp == NULL)
      return 0;
    clear_xbitmap(empty_bmp,
		  (SCREEN_BPP == 1) ? 16 + add_color :
		  (SCREEN_BPP == 2) ? MAKECOLOR16(0,0,0) : MAKECOLOR32(0,0,0));
  }

  strcpy(buf, text);
  pos = strlen(buf);
  for (i = 0; i < max_len; i++)
    draw_bitmap(empty_bmp, x + 8 * i, y);

  while (! done) {
    DO_DELAY(10000);
    if (redraw) {
      int x_end;

      x_end = x + 8 * strlen(buf);
      if (x_end > x)
	draw_bitmap(empty_bmp, x_end - 8, y);
      gr_printf_selected(1, x, y, font_8x8, "%s", buf);
      draw_bitmap(empty_bmp, x_end, y);
      draw_bitmap_tr_alpha50(font_8x8->bmp[(unsigned char) '_'], x_end, y);
      if (pos < max_len - 1)
	draw_bitmap(empty_bmp, x_end + 8, y);
      display_screen();
      redraw = 0;
    }
    if (poll_keyboard()) {
      mains->done = -1;
      done = -1;
      continue;
    }
    if (! game_kb_has_key())
      continue;
    switch (key = (game_kb_get_key() & 0xff)) {
    case 8: if (pos > 0) { redraw = 1; pos--; buf[pos] = '\0'; } break;
    case 13: done = 1; break;
    case 27: done = -1; break;
    default:
      if (pos < max_len - 1 && key >= ' ' && key <= 0x7f)
	buf[pos++] = key;
      buf[pos] = '\0';
      redraw = 1;
    }
  }
  if (done > 0) {
    strcpy(text, buf);
    return 1;
  }
  return 0;
}


static void update_available_state(void)
{
  int i;

  mains->redraw = 1;

  if (mains->server->open) {
    for (i = 0; i < NUM_OPTIONS; i++)
      mains->available[i] = 1;
    switch (mains->client_type) {
    case CLIENT_OWNER:
      display_msg("You are the owner of the server\n");
      mains->available[OPTION_MAP] = mains->available[OPTION_PLAY] = 1;
      break;

    case CLIENT_JOINED:
      display_msg("You are a joined client\n");
      mains->available[OPTION_MAP] = 0;
      mains->available[OPTION_PLAY] = 1;
      break;

    default:
      display_msg("You are a normal client\n");
      mains->available[OPTION_MAP] = mains->available[OPTION_PLAY] = 0;
      break;
    }
  } else {
    for (i = 0; i < NUM_OPTIONS; i++)
      mains->available[i] = 0;
    mains->available[OPTION_CONNECT] = mains->available[OPTION_QUIT] = 1;
    if (mains->selected != OPTION_QUIT)
      mains->selected = OPTION_CONNECT;
  }
}


static int handle_server_input(SERVER *server, OPTIONS *options, int *type)
{
  NETMSG msg;

  while (client_has_message(server->fd)) {
    if (client_read_message(server->fd, &msg) < 0) {
      client_close(server);
      server->open = 0;
      *type = CLIENT_NORMAL;
      return -1;
    }

    switch (msg.type) {
    case MSGT_SERVERINFO:
      {
	int v1, v2, v3;
	
	v1 = VERSION1(SERVER_VERSION);
	v2 = VERSION2(SERVER_VERSION);
	v3 = VERSION3(SERVER_VERSION);

	if (v1 != msg.server_info.v1
	    || v2 != msg.server_info.v2
	    || v3 != msg.server_info.v3)
	  display_msg("\n"
                      "** WARNING **"
                      "This server has an incompatible version (the right version for\n"
                      "this client is %d.%d.%d, the server reported version %d.%d.%d).\n"
                      "The game may play incorrectly, or may not play at all. Use a\n"
                      "compatible server or proceed at your own risk.\n"
		      "", v1, v2, v3, msg.server_info.v1,
		      msg.server_info.v2, msg.server_info.v3);
	else
	  display_msg("Connected to server version %d.%d.%d\n",
		      msg.server_info.v1,
		      msg.server_info.v2,
		      msg.server_info.v3);
      }
      break;

    case MSGT_SETCLIENTTYPE:
      *type = msg.set_client_type.client_type;
      display_msg("Server informs: client type is `%d'\n", *type);
      send_init_messages(server, options, *type);
      break;

    case MSGT_RETURN:
      display_msg("\nServer: %s\n", connect_error_msg(msg.msg_return.value));
      break;

    case MSGT_STRING:
      switch (msg.string.subtype) {
      case MSGSTR_JACKNAME:
	set_jack_name(server, msg.string.data, msg.string.str);
	break;

      case MSGSTR_SETMAP:
	if (read_map_file(msg.string.str)) {
	  display_msg("Connection closed.\n");
	  msg.type = MSGT_QUIT;
	  client_send_message(server->fd, &msg, 0);
	  client_close(server);
	  server->open = 0;
	  *type = CLIENT_NORMAL;
	  return -1;
	}
	break;

      default:
	display_msg("\nReceived UNHANDLED string\n"
		    "mesage (%d) from the server\n",
		    msg.string.subtype);
      }
      break;

    case MSGT_QUIT:
      client_close(server);
      display_msg("\nConnection closed by server QUIT.\n");
      server->open = 0;
      *type = CLIENT_NORMAL;
      return -1;

    case MSGT_START:
      display_msg("\nServer ready, starting game...\n");
      return 0;

    case MSGT_SETJACKID:
      if (msg.set_jack_id.id >= n_jacks) {
	display_msg("\nUexpected server inconsistency:\n"
		    "invalid jack id (%d)\n",
		    msg.set_jack_id.id);
	player->id = 0;
	jack = jacks;
      } else {
	player->id = msg.set_jack_id.id;
	jack = jacks + msg.set_jack_id.id;
      }
      break;

    case MSGT_SETJACKNPC:
      if (msg.set_jack_npc.id >= n_jacks) {
	display_msg("\nUnexpected server inconsistency:\n"
		    "invalid jack id (%d) while setting NPC %d\n",
		    msg.set_jack_npc.id, msg.set_jack_npc.npc);
      } else {
	jacks[msg.set_jack_npc.id].id = msg.set_jack_npc.id;
	do_read_jack(msg.set_jack_npc.npc,
		     jacks + msg.set_jack_npc.id, msg.set_jack_npc.id);
	reset_jack(jacks + msg.set_jack_npc.id);
	jacks[msg.set_jack_npc.id].npc = msg.set_jack_npc.npc;
	jacks[msg.set_jack_npc.id].active = 1;
      }
      break;

    case MSGT_SETJACKTEAM:
      if (msg.set_jack_team.id >= n_jacks) {
	display_msg("\nUnexpected server inconsistency:\n"
		    "invalid jack id (%d)\n",
		    msg.set_jack_team.id);
      } else
	jacks[msg.set_jack_team.id].team = msg.set_jack_team.team;
      break;

    case MSGT_NJACKS:
      n_jacks = msg.n_jacks.number;
      if (jacks == NULL)
	jacks = (JACK *) malloc(sizeof(JACK) * n_jacks);
      else
	jacks = (JACK *) realloc(jacks, sizeof(JACK) * n_jacks);
      if (jacks == NULL) {
	display_msg("\nOut of memory for players, quitting\n");
	msg.type = MSGT_QUIT;
	send_message(server->fd, &msg, 0);
	client_close(server);
	return 1;
      } else {
	int i;

	for (i = 0; i < n_jacks; i++)
	  jacks[i].id = i;
      }
      break;

    default:
      display_msg("\nReceived UNHANDLED mesage %d\n"
		  "from the server\n", msg.type);
    }
  }
  return -1;
}


/* Check server input, returns 1 if the program must return
 * immediately to the main screen (e.g., if the game will
 * start or if the connection to the server has been broken). */
static int check_server_input(void)
{
  int old_client_type, ret, old_open;

  if (! mains->server->open)
    return 0;

#ifndef NET_PLAY
  client_do_server(mains->server);
#endif /* NET_PLAY */

  old_open = mains->server->open;
  old_client_type = mains->client_type;
  ret = handle_server_input(mains->server, mains->options, &mains->client_type);
  if (ret == 1) {    /* abort */
    display_msg("Aborted\n");
    mains->done = -1;
    update_available_state();
    return 1;
  }

  if (old_open != mains->server->open) {
    display_msg("Connect status changed\n");
    update_available_state();
    return 1;
  }

  if (old_client_type != mains->client_type) {
    display_msg("Type status change\n");
    update_available_state();
  }

  if (ret == 0) {
    mains->done = 1;
    return 1;
  }
  return 0;
}


static void exec_connect_option(void)
{
  int field, redraw, done, key;
  char port_num[32];
  char server_cmd[256];

  field = 0;
  redraw = 1;
  done = 0;
  sprintf(port_num, "%d", server->port);
  sprintf(server_cmd, "loser-s -maxfps 40 -port %d", server->port);

  while (! (mains->done || done)) {
    DO_DELAY(10000);
    if (check_server_input())
      return;
    if (poll_keyboard()) {
      mains->done = -1;
      return;
    }

    if (redraw) {
      clear_screen();
      if (mains->background != NULL
	  && mains->background->w <= SCREEN_W
	  && mains->background->h <= SCREEN_H + DISP_Y)
	draw_bitmap(mains->background, DISP_X, DISP_Y);

      gr_printf_selected(field == 0, DISP_X + 20, DISP_Y + 50,
			 font_8x8, "Server:  %s", server->host);
      gr_printf_selected(field == 1, DISP_X + 20, DISP_Y + 70,
			 font_8x8, "Port:    %s", port_num);
      gr_printf_selected(field == 2, DISP_X + 20, DISP_Y + 90, font_8x8,
			 (server->open) ? "Disconnect" : "Connect");
      gr_printf_selected(field == 3, DISP_X + 20, DISP_Y + 130,
			 font_8x8, "Server:  %s", server_cmd);
      gr_printf_selected(field == 4, DISP_X + 20, DISP_Y + 150, font_8x8,
			 "Run server");
      display_screen();
      redraw = 0;
    }

    for (key = 0; key < 256; key++)
      if (game_key_trans[key] > 0) {
	game_key_trans[key] = 0;
	switch (key) {
	case SCANCODE_TAB:
	case SCANCODE_CURSORBLOCKDOWN:
	  field = (field + 1) % 5;
	  redraw = 1;
	  break;

	case SCANCODE_CURSORBLOCKUP:
	  field--;
	  if (field < 0)
	    field += 5;
	  redraw = 1;
	  break;

	case SCANCODE_ESCAPE:
	  done = -1;
	  break;

	case SCANCODE_ENTER:
	  switch (field) {
	  case 0:  /* host */
	    get_text(DISP_X + 92, DISP_Y + 50, server->host, 40);
	    redraw = 1;
	    break;
	    
	  case 1:  /* port */
	    get_text(DISP_X + 92, DISP_Y + 70, port_num, 6);
	    redraw = 1;
	    break;
	    
	  case 2:  /* Connect/disconnect */
	    if (server->open) {
	      client_close(server);
	      server->open = 0;
	    } else {
	      server->port = atoi(port_num);
	      if (client_open(server) == 0)
		server->open = 1;
	    }
	    update_available_state();
	    done = 1;
	    break;

	  case 3:  /* Server command */
	    get_text(DISP_X + 92, DISP_Y + 130, server_cmd, 25);
	    redraw = 1;
	    break;

	  case 4:  /* Run server */ 
#if (defined GRAPH_LINUX) || (defined GRAPH_X11)
	    switch (fork()) {
	    case -1:    /* Can't fork */
	      break;

	    case 0:
	      {
		char *path, *new_path;

		/* Add the current directory to the end of the PATH */
		path = getenv("PATH");
		if (path != NULL) {
		  new_path = malloc(strlen(path) + 8);
		  if (new_path != NULL) {
		    sprintf(new_path, "PATH=%s:.", path);
		    putenv(new_path);
		  }
		} else
		  putenv("PATH=/bin:.");
		execl("/bin/sh", "sh", "-c", server_cmd, NULL);
		printf("Can't exec `/bin/sh -c \"%s\"'\n", server_cmd);
		_exit(1);   /* Error */
	      }

	    default:
              ;
	    }
#else
	    system(server_cmd);
#endif
	    redraw = 1;
	    break;
	  }
	}
      }
  }
}

static void exec_character_option(void)
{
  char file_name[256];
  int jack_available[256], secret_len[256], secret_pos = 0;
  NETMSG msg;
  int done, jack, key, field, redraw;
  XBITMAP *jack_bmp[MAX_NPCS];

  {
    int i, j;

    for (i = 0; i < npc_number.num_jacks; i++) {
      char tmp[256];

      sprintf(file_name, "start-up|%s.spr", npc_info[i].file);
      strcpy(tmp, img_file_name(file_name));
      jack_bmp[i] = read_xbitmap(tmp);
      if (jack_bmp[i] == NULL) {
	printf("Can't read file `%s'\n", tmp);
	for (i--; i >= 0; i--)
	  destroy_xbitmap(jack_bmp[i]);
	return;
      }
      for (j = 0; npc_info[i].secret[j] != 255; j++)
	;
      secret_len[i] = j;
      jack_available[i] = (secret_len[i] == 0);
    }
  }
  add_color_bmps(jack_bmp, npc_number.num_jacks, add_color);

  redraw = 1;
  field = 0;
  done = 0;
  jack = mains->jack_npc;

  while (! (mains->done || done)) {
    DO_DELAY(10000);
    if (check_server_input())
      return;
    if (poll_keyboard()) {
      mains->done = -1;
      return;
    }

    if (redraw) {
      clear_screen();
      if (mains->background != NULL
	  && mains->background->w <= SCREEN_W
	  && mains->background->h <= SCREEN_H + DISP_Y)
	draw_bitmap(mains->background, DISP_X, DISP_Y);

      if (field == 0)
	draw_bitmap_tr(jack_bmp[jack], DISP_X + 10, DISP_Y + 10);
      else
	draw_bitmap_tr_alpha75(jack_bmp[jack], DISP_X + 10, DISP_Y + 10);
      gr_printf_selected(field == 1, DISP_X + 180, DISP_Y + 30,
			 font_8x8, "Name:");
      gr_printf_selected(field == 1, DISP_X + 180, DISP_Y + 50,
			 font_8x8, "%s", mains->jack_name);
      gr_printf_selected(field == 2, DISP_X + SCREEN_W - 40,
			 DISP_Y + SCREEN_H - 16, font_8x8, "Done");
      redraw = 0;
      display_screen();
    }

    for (key = 0; key < 256; key++)
      if (game_key_trans[key] > 0) {
	game_key_trans[key] = 0;

	{
	  int found, i;

	  found = 0;
	  for (i = 0; i < npc_number.num_jacks; i++)
	    if (secret_pos < secret_len[i]
		&& key == npc_info[i].secret[secret_pos]) {
	      secret_pos++;
	      found = 1;
	      if (secret_pos == secret_len[i]) {
		jack_available[i] = 1;
		secret_pos = 0;
	      }
	      break;
	    }
	  if (! found)
	    secret_pos = 0;
	}

	switch (key) {
	case SCANCODE_ESCAPE:
	  done = -1;
	  break;

	case SCANCODE_TAB:
	  field = (field + 1) % 3;
	  redraw = 1;
	  break;

	case SCANCODE_ENTER:
	  switch (field) {
	  case 0:    /* Jack */
	    do
	      jack = (jack + 1) % npc_number.num_jacks;
	    while (! jack_available[jack]);
	    redraw = 1;
	    break;

	  case 1:    /* Name */
	    mains->jack_name[20] = '\0';
	    get_text(DISP_X + 180, DISP_Y + 50, mains->jack_name, 16);
	    redraw = 1;
	    break;

	  case 2:    /* Done */
	    mains->jack_npc = jack;
	    done = 1;
	    break;
	  }
	  break;
	}
      }
  }

  {
    int i;

    for (i = 0; i < npc_number.num_jacks; i++)
      destroy_xbitmap(jack_bmp[i]);
  }

  if (done > 0) {
    msg.type = MSGT_SETJACKNPC;
    msg.set_jack_npc.id = 2;
    msg.set_jack_npc.npc = jack;
    client_send_message(server->fd, &msg, 0);

    msg.type = MSGT_STRING;
    msg.string.subtype = MSGSTR_JACKNAME;
    msg.string.data = 0;
    strcpy(msg.string.str, mains->jack_name);
    client_send_message(server->fd, &msg, 0);
  }
}


static int map_color(MAP *tile)
{
  if (IS_MAP_BLOCK(tile->block) || tile->block == MAP_SECRET)
    switch (SCREEN_BPP) {
    case 1: return MAKECOLOR8(0,0,255);
    case 2: return MAKECOLOR16(0,0,255);
    default: return MAKECOLOR32(0,0,255);
    }
  else
    switch (SCREEN_BPP) {
    case 1: return MAKECOLOR8(0,0,0);
    case 2: return MAKECOLOR16(0,0,0);
    default: return MAKECOLOR32(0,0,0);
    }
}

static void draw_map_pixel(XBITMAP *bmp, int px, int py, int zoom, int color)
{
  int x, y;

  px *= zoom;
  py *= zoom;
  for (y = py; y < py + zoom; y++)
    for (x = px; x < px + zoom; x++)
      if (SCREEN_BPP == 1)
	bmp->line[y][x] = color;
      else if (SCREEN_BPP == 2)
	((int2bpp *) bmp->line[y])[x] = color;
      else
	((int4bpp *) bmp->line[y])[x] = color;
}

static void draw_map(XBITMAP *bmp, MAP *map_data, int map_w, int map_h)
{
  int x, y, w, h;
  int zoom;

  w = map_w;
  h = map_h;
  if (bmp->w < w)
    w = bmp->w;
  if (bmp->h < h)
    h = bmp->h;

  if (w == 0 || h == 0)
    return;

  zoom = bmp->w / w;
  if (zoom > bmp->h / h)
    zoom = bmp->h / h;

  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++)
      draw_map_pixel(bmp, x, y, zoom, map_color(map_data + y * map_w + x));
}

static int get_map_bmp(XBITMAP *bmp, char *name, int *p_map_w, int *p_map_h,
		       char *author, char *desc, char *tiles,
		       char *tiles_author)
{
  MAP *map_data;               /* Current Map data */
  int map_w, map_h;
  char filename[256];

  *author = *desc = *tiles = *tiles_author = '\0';

  sprintf(filename, "%s%s%s.map", DATA_DIR, MAPS_DIR, name);
  map_data = read_map(filename, &map_w, &map_h, author, desc, tiles);
  if (map_data == NULL)
    return 1;
  {
    char *p;
    int i;

    if ((p = strchr(tiles, ':')) != NULL)
      *p = '\0';

    for (i = 0; i < tileset_info->n_tilesets; i++)
      if (strcmp(tileset_info->tileset[i].name, tiles) == 0 &&
	  *tileset_info->tileset[i].author != '\0')
	sprintf(tiles_author, "(by %s)", tileset_info->tileset[i].author);
  }

  *p_map_w = map_w;
  *p_map_h = map_h;
  clear_xbitmap(bmp,
		(SCREEN_BPP == 1) ? XBMP8_TRANSP + add_color :
		(SCREEN_BPP == 2) ? XBMP16_TRANSP : XBMP32_TRANSP);
  draw_map(bmp, map_data, map_w, map_h);
  free(map_data);
  return 0;
}

/* Get all file names ending with `.map' on the specified directory. */
static int get_maps_from_dir(char *dir_name, char *prefix,
			     char **list, int max)
{
#ifndef GRAPH_WIN
  DIR *dir;
  struct dirent *dirent;
  char name[256];
  int n, i;

  if ((dir = opendir(dir_name)) == NULL)
    return -1;

  n = 0;
  while ((dirent = readdir(dir)) != NULL && n < max - 1) {
    strcpy(name, dirent->d_name);
    for (i = strlen(name) - 1; i >= 0; i--)
      if (name[i] == '.')
	break;
    if (i <= 0)
      continue;      /* File has no '.' or begins with '.' */
    if (strcmp(name + i, ".map") == 0) {
      name[i] = '\0';
      list[n] = malloc(strlen(prefix) + strlen(name) + 1);
      sprintf(list[n], "%s%s", prefix, name);
      if (list[n] == NULL) {
	for (n--; n >= 0; n--)
	  free(list[n]);
	return -1;
      }
      n++;
    }
  }
  closedir(dir);
  return n;
#else
  return 0;    /* Not implemented for windows */
#endif
}

static int compare_map_name(const void *p1, const void *p2)
{
  return strcmp(*(const char **)p1, *(const char **)p2);
}

/* Get the list of maps from the default map directories. Return the
 * number of files found. */
static int get_map_list(char **list, int max)
{
  static char *dir_list[] = { "", "death-match", "multiplayer", "usr", NULL };
  int i, n, num_maps;
  char maps_dir[256], prefix[256];

  num_maps = 0;
  for (i = 0; dir_list[i] != NULL; i++) {
    if (dir_list[i][0] != '\0')
      sprintf(prefix, "%s%c", dir_list[i], DIRECTORY_SEP);
    else
      prefix[0] = '\0';
    sprintf(maps_dir, "%s%s%s", DATA_DIR, MAPS_DIR, prefix);
    n = get_maps_from_dir(maps_dir, prefix, list + num_maps, max - num_maps);
    if (n > 0) {
      qsort(list + num_maps, n, sizeof(char *), compare_map_name);
      num_maps += n;
    }
  }

  return num_maps;
}

static void free_map_list(char **list, int n)
{
  int i;

  for (i = 0; i < n; i++)
    free(list[i]);
}

static void exec_map_option(void)
{
  char *map_list[1024];        /* List of map names */
  int n_maps;                  /* Number of maps in the list */
  XBITMAP *map_bmp;            /* Current map bitmap */
  char *default_map;
  char author[256], comments[256], tiles[256], tiles_author[256];
  int map_w, map_h;
  int done, redraw, rebuild, cur_map, key;

  map_w = map_h = 0;
  
  if (mains->map[0] != '\0')
    default_map = mains->map;
  else
    default_map = "castle";

  map_bmp = create_xbitmap(150, 150, SCREEN_BPP);
  if (map_bmp == NULL) {
    printf("Out of memory for map bitmap\n");
    return;
  }
  n_maps = get_map_list(map_list, 1024);
  if (n_maps < 0) {
    printf("Can't read maps directory\n");
    return;
  }
  if (n_maps < 0) {
    printf("No maps!\n");
    return;
  }
  done = 0;
  rebuild = redraw = 1;
  for (cur_map = 0; cur_map < n_maps; cur_map++)
    if (strcmp(map_list[cur_map], default_map) == 0)
      break;
  if (cur_map >= n_maps)
    cur_map = 0;

  while (! (mains->done || done)) {
    DO_DELAY(10000);
    if (check_server_input())
      return;
    if (poll_keyboard()) {
      mains->done = -1;
      return;
    }

    if (rebuild) {
      if (get_map_bmp(map_bmp, map_list[cur_map], &map_w, &map_h,
		      author, comments, tiles, tiles_author) != 0)
	clear_xbitmap(map_bmp, add_color);
      rebuild = 0;
      redraw = 1;
    }
    if (redraw) {
      char map_author[256];

      if (*author)
	sprintf(map_author, "(by %s)", author);
      else
	*map_author = '\0';
      clear_screen();
      if (mains->background != NULL
	  && mains->background->w <= SCREEN_W
	  && mains->background->h <= SCREEN_H + DISP_Y)
	draw_bitmap(mains->background, DISP_X, DISP_Y);
      draw_bitmap_tr(map_bmp, DISP_X + 10, DISP_Y + 10);
      gr_printf_selected(0, DISP_X + 10, DISP_Y + 170, font_8x8,
			 "     Map: %s %s", map_list[cur_map], map_author);
#if 0
      /* 170, 190, 205, 220 */
      gr_printf_selected(0, DISP_X + 10, DISP_Y + 190, font_8x8,
			 "  Author: %s", author);
#endif
      gr_printf_selected(0, DISP_X + 10, DISP_Y + 190, font_8x8,
			 "Comments: %s", comments);
      gr_printf_selected(0, DISP_X + 10, DISP_Y + 205, font_8x8,
			 "   Tiles: %s %s", tiles, tiles_author);
      gr_printf_selected(0, DISP_X + 170, DISP_Y + 10, font_8x8,
			 "Size: (%dx%d)", map_w, map_h);
      display_screen();
      redraw = 0;
    }

    for (key = 0; key < 128; key++)
      if (game_key_trans[key] > 0) {
	game_key_trans[key] = 0;
	switch (key) {
	case SCANCODE_CURSORBLOCKUP:
	case SCANCODE_CURSORBLOCKLEFT:
	  if (cur_map <= 0)
	    cur_map = n_maps - 1;
	  else
	    cur_map--;
	  rebuild = 1;
	  break;

	case SCANCODE_CURSORBLOCKDOWN:
	case SCANCODE_CURSORBLOCKRIGHT:
	  cur_map = (cur_map + 1) % n_maps;
	  rebuild = 1;
	  break;

	case SCANCODE_ENTER: done = 1; break;
	case SCANCODE_ESCAPE: done = -1; break;
	}
      }
  }

  if (done > 0) {
    NETMSG msg;

    strcpy(mains->map, map_list[cur_map]);

    msg.type = MSGT_STRING;
    msg.string.subtype = MSGSTR_SETMAP;
    strncpy(msg.string.str, map_list[cur_map], MAX_STRING_LEN-1);
    msg.string.str[MAX_STRING_LEN-1] = '\0';
    client_send_message(server->fd, &msg, 0);
  }
  destroy_xbitmap(map_bmp);
  free_map_list(map_list, n_maps);
}

static void exec_team_option(void)
{
  int done, redraw, field, key;
  char team_player[256];

  field = 0;
  done = 0;
  redraw = 1;
  team_player[0] = '\0';
  while (! (mains->done || done)) {
    DO_DELAY(10000);
    if (check_server_input())
      return;
    if (poll_keyboard()) {
      mains->done = -1;
      return;
    }

    if (redraw) {
      clear_screen();
      if (mains->background != NULL
	  && mains->background->w <= SCREEN_W
	  && mains->background->h <= SCREEN_H + DISP_Y)
	draw_bitmap(mains->background, DISP_X, DISP_Y);
      gr_printf_selected(field == 0, DISP_X + 10, DISP_Y + 80, font_8x8,
			 "Player:");
      gr_printf_selected(field == 0, DISP_X + 80, DISP_Y + 80, font_8x8,
			 "%s", team_player);
      gr_printf_selected(field == 1, DISP_X + 10, DISP_Y + 110, font_8x8,
			 "Join team");
      gr_printf_selected(field == 2, DISP_X + 10, DISP_Y + 130, font_8x8,
			 "Leave team");
      display_screen();
      redraw = 0;
    }

    for (key = 0; key < 128; key++)
      if (game_key_trans[key] > 0) {
	game_key_trans[key] = 0;
	switch (key) {
	case SCANCODE_CURSORBLOCKUP:
	case SCANCODE_CURSORBLOCKLEFT:
	  if (field <= 0)
	    field = 2;
	  else
	    field--;
	  redraw = 1;
	  break;

	case SCANCODE_CURSORBLOCKDOWN:
	case SCANCODE_CURSORBLOCKRIGHT:
	case SCANCODE_TAB:
	  field = (field + 1) % 3;
	  redraw = 1;
	  break;

	case SCANCODE_ENTER:
	  if (field == 0) {
	    get_text(DISP_X + 80, DISP_Y + 80, team_player, 20);
	    redraw = 1;
	  } else
	    done = 1;
	  break;

	case SCANCODE_ESCAPE:
	  done = -1;
	  break;
	}
      }
  }

  if (done > 0) {
    NETMSG msg;

    msg.type = MSGT_STRING;
    strncpy(msg.string.str, team_player, MAX_STRING_LEN-1);
    msg.string.str[MAX_STRING_LEN-1] = '\0';
    if (field == 1)
      msg.string.subtype = MSGSTR_TEAMJOIN;
    else
      msg.string.subtype = MSGSTR_TEAMLEAVE;
    send_message(server->fd, &msg, 0);
  }
}

static void exec_play_option(void)
{
  if (mains->client_type == CLIENT_OWNER)
    send_go_message(mains->server->fd);
  else {
    NETMSG msg;

    msg.type = MSGT_JOIN;
    client_send_message(mains->server->fd, &msg, 0);
  }
}

static void exec_selected_option(void)
{
  switch (mains->selected) {
  case 0: exec_connect_option(); break;
  case 1: exec_character_option(); break;
  case 2: exec_map_option(); break;
  case 3: exec_team_option(); break;
  case 4: exec_play_option(); break;

  case 5:   /* Quit */
    mains->done = -1;
  }
}

static void draw_options(void)
{
  static int pos_y_sel[] = { 66, 88, 112, 137, 160, 206 };
  static int pos_y_dis[] = { 66, 89, 112, 135, 158, 206 };
  int x_start, y_start, i;

  x_start = DISP_X + (SCREEN_W - mains->title->w) / 2;
  y_start = DISP_Y + (SCREEN_H - mains->title->h) / 2;

  clear_screen();
  if (mains->title->w <= DISP_X + SCREEN_W
      && mains->title->h <= DISP_Y + SCREEN_H)
    draw_bitmap(mains->title, x_start, y_start);
  for (i = 0; i < NUM_OPTIONS; i++) {
    if (mains->selected == i)
      draw_bitmap_tr_alpha25(mains->red, x_start + 7, y_start + pos_y_sel[i]);
    else if (mains->available[i] == 0)
      draw_bitmap_tr_alpha50(mains->black, x_start + 7, y_start + pos_y_dis[i]);
  }
  display_screen();
}


static int read_startup_bmps(void)
{
  char file[256];

  mains->red = create_xbitmap(138, 23, SCREEN_BPP);
  if (mains->red == NULL) {
    printf("Out of memory for bitmap\n");
    return 1;
  }
  mains->black = create_xbitmap(138, 23, SCREEN_BPP);
  if (mains->black == NULL) {
    printf("Out of memory for bitmap\n");
    destroy_xbitmap(mains->red);
    return 1;
  }
  if (SCREEN_BPP == 4) {
    clear_xbitmap(mains->red, MAKECOLOR32(255,0,0));
    clear_xbitmap(mains->black, MAKECOLOR32(0,0,0));
  } else if (SCREEN_BPP == 2) {
    clear_xbitmap(mains->red, MAKECOLOR16(255,0,0));
    clear_xbitmap(mains->black, MAKECOLOR16(0,0,0));
  } else {
    int n, r, w;
    unsigned char *p;

    clear_xbitmap(mains->red, add_color + MAKECOLOR8(255,255,255));
    clear_xbitmap(mains->black, add_color + MAKECOLOR8(0,0,0));
    p = mains->black->line[0];
    for (n = mains->black->h * mains->black->w; n > 0; n--)
      if ((n + (n / mains->black->w)) & 1)
	*p++ = XBMP8_TRANSP;
      else
	p++;


    w = mains->red->w;
    p = mains->red->line[1];
    for (n = (mains->red->h - 2) * w; n > 0; n--)
      if ((r = n % w) != 0 && r != 1)
	*p++ = XBMP8_TRANSP;
      else
	p++;
  }

  strcpy(file, img_file_name("start-up|title.spr"));
  mains->title = read_xbitmap(file);
  if (mains->title == NULL) {
    destroy_xbitmap(mains->red);
    destroy_xbitmap(mains->black);
    printf("Error reading `%s'\n", file);
    return 1;
  }
  add_color_bmp(mains->title, add_color);

  mains->background = read_xbitmap(img_file_name("start-up|back.spr"));
  if (mains->background != NULL)
    add_color_bmp(mains->background, add_color);
			       
  return 0;
}


static int display_msg_box(const char *fmt, ...)
{
  static char str[1024];
#if 0
  static int in_message = 0, pos_x = 0, pos_y = 0;
  unsigned char *p;
  int x, y;
  va_list ap;

  va_start(ap, fmt);
  vsprintf(str, fmt, ap);
  va_end(ap);

  if (! in_message)
    clear_screen();
  x = pos_x;
  y = pos_y;
  for (p = str; *p != '\0'; p++) {
    if (*p == '\n') {
      x = 0;
      y++;
    } else {
      gr_printf(DISP_X + 8*x + 8, DISP_Y + 10*y + 10, font_8x8, "%c", *p);
      x++;
    }
  }
  display_screen();
  pos_x = x;
  pos_y = y;
  if (in_message)
    return 0;

  in_message = 1;
  while (game_key_trans[SCANCODE_ENTER] <= 0) {
    if (poll_keyboard()) {
      mains->done = -1;
      break;
    }
    check_server_input();
  }
  game_key_trans[SCANCODE_ENTER] = 0;
  in_message = 0;
  pos_x = pos_y = 0;
#else
  va_list ap;

  va_start(ap, fmt);
  vsprintf(str, fmt, ap);
  va_end(ap);
#endif
  return 0;
}


int client_wait(SERVER *server, OPTIONS *options)
{
  int i;

  mains = &main_screen;

  if (options->debug)
    display_msg = printf;
  else
    display_msg = display_msg_box;

  if (SCREEN_BPP == 1 && read_palette(img_file_name("start-up.pal")) != 0) {
    printf("Can't read palette from `%s'\n", img_file_name("start-up.pal"));
    return 1;
  }
  install_palette();
  read_startup_bmps();

#ifdef HAS_SOUND
  {
    int music_id;

    music_id = get_music_id("theme");
    if (music_id >= 0)
      snd_start_music(music_id, 0);
  }
#endif /* HAS_SOUND */

  *mains->map = *mains->jack_name = '\0';
  mains->redraw = 1;
  mains->server = server;
  mains->options = options;
  mains->done = 0;
  mains->selected = 0;
  if (options->ask_map[0] != '\0')
    strcpy(mains->map, options->ask_map);
  if (options->name != NULL)
    strcpy(mains->jack_name, options->name);
  mains->jack_npc = 0;
  if (options->jack[0] != '\0')
    for (i = 0; i < npc_number.num_jacks - 1; i++)
      if (strcmp(npc_info[i].npc_name, options->jack) == 0) {
	mains->jack_npc = i;
	break;
      }
  for (i = 0; i < NUM_OPTIONS; i++)
    mains->available[i] = 0;
  mains->available[OPTION_CONNECT] = mains->available[OPTION_QUIT] = 1;
  mains->selected = 0;

  if (client_startup_conn(server, options) == 0)
    server->open = 1;
  update_available_state();

  while (! mains->done) {
    DO_DELAY(10000);

    if (server->open)
      check_server_input();
    else
      mains->client_type = CLIENT_NORMAL;

    if (mains->redraw) {
      draw_options();
      mains->redraw = 0;
    }
    if (poll_keyboard()) {
      mains->done = -1;
      continue;
    }
    for (i = 0; i < 128; i++)
      if (game_key_trans[i] > 0) {
	game_key_trans[i] = 0;
	switch (i) {
	case SCANCODE_CURSORBLOCKUP:
	  do {
	    mains->selected--;
	    if (mains->selected < 0)
	      mains->selected += NUM_OPTIONS;
	  } while (! mains->available[mains->selected]);
	  mains->redraw = 1;
	  break;

	case SCANCODE_CURSORBLOCKDOWN:
	  do {
	    mains->selected++;
	    mains->selected %= NUM_OPTIONS;
	  } while (! mains->available[mains->selected]);
	  mains->redraw = 1;
	  break;

	case SCANCODE_ESCAPE:
	  mains->selected = OPTION_QUIT;
	  mains->redraw = 1;
	  break;

	case SCANCODE_ENTER:
	  exec_selected_option();
	  mains->redraw = 1;
	  break;
	}
      }
  }

  destroy_xbitmap(mains->title);
  destroy_xbitmap(mains->red);
  destroy_xbitmap(mains->black);
  if (mains->background != NULL)
    destroy_xbitmap(mains->background);

  if (mains->done > 0 && server->open) {
    strcpy(options->ask_map, mains->map);
    if (mains->jack_npc >= 0 && mains->jack_npc < npc_number.num_jacks)
      strcpy(options->jack, npc_info[mains->jack_npc].npc_name);
    return 0;
  }
  return 1;
}
