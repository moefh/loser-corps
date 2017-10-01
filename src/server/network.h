/* network.h */

#ifndef NETWORK_H_FILE
#define NETWORK_H_FILE

int srv_open(SERVER *);
void srv_close(SERVER *);
void srv_close_connection(int fd);
void srv_close_client(CLIENT *);
int srv_accept_connection(SERVER *server, char *host, int *port);
int srv_verify_data(SERVER *server);

#endif /* NETWORK_H_FILE */
