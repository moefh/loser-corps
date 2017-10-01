/* game.h
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#ifndef GAME_H_FILE
#define GAME_H_FILE

int srv_receive_frame(SERVER *server);
void srv_send_frame(SERVER *server);
void srv_calc_frame(SERVER *server);

#endif /* GAME_H_FILE */
