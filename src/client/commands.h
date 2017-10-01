/* commands.h */

#ifndef COMMANDS_H_FILE
#define COMMANDS_H_FILE

typedef struct COMMAND {
  char *name;
  int (*func)(SERVER *, int *, char *, char **);
} COMMAND;

extern COMMAND command[];

void send_go_message(int fd);

#endif /* COMMANDS_H_FILE */
