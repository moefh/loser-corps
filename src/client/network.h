/* network.h */

#ifndef NETWORK_H_FILE
#define NETWORK_H_FILE

int client_open(SERVER *);
void client_close(SERVER *);

#ifndef NET_PLAY
void client_do_server(SERVER *);
#endif /* NET_PLAY */

#endif /* NETWORK_H_FILE */
