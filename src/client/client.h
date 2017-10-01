/* client.h
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#ifndef CLIENT_H_FILE
#define CLIENT_H_FILE

#define DEFAULT_HOST   "localhost"

typedef struct SERVER {
  int open;             /* 1 after opened */
  int type;             /* CONNECT_xxx */
  char host[256];       /* Host name */
  int port;             /* Port */
  int fd;               /* fd of connection */
  int game_running;     /* 1 if the game is running */
  int num_keypress;
} SERVER;

void remove_jack(int id);
void set_jack_name(SERVER *server, int id, char *name);

int client_open_conn(SERVER *server, OPTIONS *options);
int client_wait(SERVER *server, OPTIONS *options);

int client_send_message(int fd, NETMSG *msg, int use_buffer);
int client_has_message(int fd);
int client_read_message(int fd, NETMSG *msg);
char *connect_error_msg(int value);

int client_receive(SERVER *server);
void client_send_key(SERVER *server, int scancode, int press);
void client_send_key_end(SERVER *server);

#endif /* CLIENT_H_FILE */
