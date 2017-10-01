/* parse.h */

#ifndef PARSE_H
#define PARSE_H

typedef struct PARSE_FILE_STATE {
  char filename[1024];
  FILE *f;
  int line;
  char buffer[1024];
  char *pos;
  int end_of_file;
  void *user_data;
} PARSE_FILE_STATE;

PARSE_FILE_STATE *open_parse_file(char *filename);
int read_parse_token(PARSE_FILE_STATE *state, char *token);
void close_parse_file(PARSE_FILE_STATE *state);

#endif
