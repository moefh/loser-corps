/* connect.h
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#ifndef CONNECT_H_FILE
#define CONNECT_H_FILE

int srv_do_startup(SERVER *server);
void srv_broadcast_message(CLIENT *list, NETMSG *msg);
void srv_do_frame(SERVER *server);

void server_copy_map_info(SERVER *server, MAP_INFO *info);

#endif /* CONNECT_H_FILE */
