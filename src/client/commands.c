/* commands.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "game.h"
#include "commands.h"

#define CMD_ARGS   SERVER *server, int *type, char *cmd_line, char **args

#define CHECK_SERVER_OPEN(server)		\
  do						\
    if (! (server)->open) {			\
      display_msg("Not connected.\n");		\
      return 0;					\
    }						\
  while (0)


void send_go_message(int fd)
{
  NETMSG msg;

  /* send_npc_info(fd); */
  msg.type = MSGT_START;
  client_send_message(fd, &msg, 0);
}

#if 0
static int get_name(char *name, char **args)
{
  int i, len, truncated = 0;

  name[len = 0] = '\0';
  for (i = 0; args[i] != NULL; i++) {
    if (len + strlen(args[i]) + 1 >= MAX_STRING_LEN) {
      truncated = 1;
      strncpy(name + len, args[i], MAX_STRING_LEN - 1 - len);
      name[MAX_STRING_LEN-1] = '\0';
      break;
    }
    strcat(name, args[i]);
    len += strlen(args[i]);
    if (args[i+1] != NULL) {
      strcat(name, " ");
      len++;
    }
  }

  return truncated;
}



/************************************/
/** Command handlers ****************/


int cmd_open(CMD_ARGS)
{
  if (server->open) {
    display_msg("Already connected.\n");
    return 0;
  }

  if (args[1] != NULL) {
    strcpy(server->host, args[1]);

    if (args[2] != NULL)
      server->port = atoi(args[2]);
  }

  if (client_open(server) == 0)
    server->open = 1;
  return 0;
}

int cmd_close(CMD_ARGS)
{
  CHECK_SERVER_OPEN(server);

  client_close(server);
  server->open = 0;
  display_msg("Connection closed.\n");
  return 0;
}

int cmd_go(CMD_ARGS)
{
  CHECK_SERVER_OPEN(server);

  if (*type == CLIENT_OWNER)
    send_go_message(server->fd);
  else
    display_msg("Sorry, you don't own this server\n");
  return 0;
}

int cmd_join(CMD_ARGS)
{
  CHECK_SERVER_OPEN(server);

  if (*type == CLIENT_JOINED) {
    NETMSG msg;

    msg.type = MSGT_JOIN;
    client_send_message(server->fd, &msg, 0);
  } else
    display_msg("There's no game in progress.\n");
  return 0;
}

int cmd_map(CMD_ARGS)
{
  CHECK_SERVER_OPEN(server);

  if (*type == CLIENT_OWNER) {
    if (args[1] == NULL)
      display_msg("Please specify the map filename\n");
    else {
      NETMSG msg;

      msg.type = MSGT_STRING;
      msg.string.subtype = MSGSTR_SETMAP;
      strncpy(msg.string.str, args[1], MAX_STRING_LEN-1);
      msg.string.str[MAX_STRING_LEN-1] = '\0';
      client_send_message(server->fd, &msg, 0);
    }
  } else
    display_msg("Sorry, you don't own this server.\n");
  return 0;
}

int cmd_setmap(CMD_ARGS)
{
  return cmd_map(server, type, cmd_line, args);
}

int cmd_kill(CMD_ARGS)
{
  NETMSG msg;
  char txt[MAX_STRING_LEN];

  CHECK_SERVER_OPEN(server);

  msg.type = MSGT_STRING;
  msg.string.subtype = MSGSTR_KILL;

  display_msg("Password:");
  while (! read_password(txt, MAX_STRING_LEN-1))
    ;
  strncpy(msg.string.str, txt, MAX_STRING_LEN - 1);
  msg.string.str[MAX_STRING_LEN-1] = '\0';
  client_send_message(server->fd, &msg, 0);
  memset(txt, 0, MAX_STRING_LEN);
  memset(msg.string.str, 0, MAX_STRING_LEN);
  return 0;
}

int cmd_jack(CMD_ARGS)
{
  NETMSG msg;
  int i;

  CHECK_SERVER_OPEN(server);

  if (args[1] == NULL) {
    display_msg("Please give one of the names:\n\n");
    for (i = 0; i < NPC_NUMJACKS && npc_file[i] != NULL; i++)
      display_msg("  %s\n", npc_file[i]);
    display_msg("\n");
    return 0;
  }

  for (i = 0; i < NPC_NUMJACKS && npc_file[i] != NULL; i++)
    if (strcmp(npc_file[i], args[1]) == 0) {
      msg.type = MSGT_SETJACKNPC;
      msg.set_jack_npc.id = 2;
      msg.set_jack_npc.npc = i;
      send_message(server->fd, &msg, 0);
      return 0;
    }
  display_msg("Invalid jack name.\n"
	      "Use `jack' to see the name list.\n");
  return 0;
}

int cmd_name(CMD_ARGS)
{
  NETMSG msg;
  char name[MAX_STRING_LEN];

  CHECK_SERVER_OPEN(server);

  if (args[1] == NULL) {
    display_msg("Please specify the name\n");
    return 0;
  }

  if (get_name(name, args + 1))
    display_msg("Name too long, truncated\n");

  msg.type = MSGT_STRING;
  msg.string.subtype = MSGSTR_JACKNAME;
  msg.string.data = 0;
  strcpy(msg.string.str, name);
  client_send_message(server->fd, &msg, 0);
  return 0;
}

int cmd_team(CMD_ARGS)
{
  int error = 0;
  NETMSG msg;

  CHECK_SERVER_OPEN(server);

  if (args[1] == NULL) {
    display_msg("Please specify the team option\n");
    return 0;
  }

  msg.type = MSGT_STRING;
  if (strcmp(args[1], "join") == 0) {
    if (args[2] == NULL) {
      error = 1;
      display_msg("Please specify argument\n"
		  "to command `team join'\n");
    } else {
      char name[MAX_STRING_LEN];

      if (get_name(name, args + 2))
	display_msg("Name too long, truncated\n");

      msg.string.subtype = MSGSTR_TEAMJOIN;
      strncpy(msg.string.str, name, MAX_STRING_LEN-1);
      msg.string.str[MAX_STRING_LEN-1] = '\0';
    }
  } else if (strcmp(args[1], "leave") == 0)
    msg.string.subtype = MSGSTR_TEAMLEAVE;
  else {
    display_msg("Unknown team option: `%s'\n", args[1]);
    error = 1;
  }
  
  if (! error)
    send_message(server->fd, &msg, 0);
  return 0;
}

int cmd_exit(CMD_ARGS)
{
  NETMSG msg;

  if (server->open) {
    msg.type = MSGT_QUIT;
    client_send_message(server->fd, &msg, 0);
    client_close(server);
    server->open = 0;
    *type = CLIENT_NORMAL;
  }
  return 1;
}

int cmd_shell(CMD_ARGS)
{
  char *p;

  for (p = cmd_line; isspace(*p); p++)
    ;
  while (! isspace(*p) && *p != '\0')
    p++;
  if (*p == '\0') {
    display_msg("Please type a command.\n");
    return 0;
  }
  while (isspace(*p))
    p++;
  system(p);
  return 0;
}

int cmd_info(CMD_ARGS)
{
  if (server->open) {
    display_msg("Connected to server\nat %s:%d\n", server->host, server->port);
    display_msg("Currently you are a ");
    switch (*type) {
    case CLIENT_NORMAL: display_msg("NORMAL"); break;
    case CLIENT_JOINED: display_msg("JOINED"); break;
    case CLIENT_OWNER: display_msg("OWNER"); break;
    default: display_msg("(unknown type!!!)"); break;
    }
    display_msg(" client.\n");
  } else
    display_msg("No connection.\n");
  return 0;
}

int cmd_help(CMD_ARGS)
{
  display_msg("\nAvailable commands:\n\
\n\
help            - show this help\n\
info            - show info\n\
shell COMMAND   - execute `COMMAND'\n\
open [H [P]]    - open server at H:P\n\
close           - close connection\n\
exit            - disconnect and exit\n\
kill            - kill the server\n\
name NAME       - set your name\n\
jack [JACK]     - set your jack\n\
team join NAME  - join a player's team\n\
team leave      - leave your team\n");

  switch (*type) {
  case CLIENT_OWNER:
    display_msg("go              - start the game\n");
    display_msg("map MAP         - use the map `MAP'\n");
    break;

  case CLIENT_JOINED:
    display_msg("join            - join the game\n");
    break;
  }

  display_msg("\n");
  return 0;
}




#define MAKE_COMM(c)    { #c, cmd_##c }

COMMAND command[] = {
  MAKE_COMM(open),
  MAKE_COMM(close),
  MAKE_COMM(info),
  MAKE_COMM(go),
  MAKE_COMM(join),
  MAKE_COMM(setmap),
  MAKE_COMM(map),
  MAKE_COMM(kill),
  MAKE_COMM(help),
  MAKE_COMM(jack),
  MAKE_COMM(name),
  MAKE_COMM(team),
  MAKE_COMM(exit),
  MAKE_COMM(shell),

  { NULL }
};

#endif /* 0 */
