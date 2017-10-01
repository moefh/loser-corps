/* key_data.h */

#ifndef KEY_DATA_H
#define KEY_DATA_H

typedef struct KEY_CONFIG {
  /* Keys sent to the server: */
  int left;              /* Move left */
  int right;             /* Move right */
  int jump;              /* Jump */
  int down;              /* Unused */
  int shoot;             /* Use weapon */
  int use;               /* Use item */
  
  /* For local use in the client: */
  int show_fps;
  int show_map;
  int show_parallax;
  int show_credits;
  int pause_credits;
  int screen_follow;
  int screen_up;
  int screen_down;
  int screen_left;
  int screen_right;
} KEY_CONFIG;

int parse_key_info(KEY_CONFIG *info, char *filename);

#endif /* KEY_DATA_H */
