/* spr_data.h */

#ifndef SPR_DATA_H
#define SPR_DATA_H


typedef struct TILESET_FILE {
  char name[32];
  char author[128];
  char comment[128];
  char copyright[128];
} TILESET_FILE;

typedef struct TILESET_INFO {
  int n_tilesets;
  TILESET_FILE *tileset;
} TILESET_INFO;

int parse_tileset_info(TILESET_INFO *tileset_info, char *info_filename);

#endif

