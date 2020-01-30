/* config.h */

#ifndef CONFIG_H_FILE
#define CONFIG_H_FILE

#define DEFAULT_DATA_DIR       "./loser/data"
#define DEFAULT_MAP_FILE       "maps/castle.map"
#define DEFAULT_BLOCK_BMP_FILE "mapedit/block.spr"
#define DEFAULT_BACK_BMP_FILE  "mapedit/back.spr"
#define DEFAULT_FONT           "-*-helvetica-bold-r-*-*-14-*-*-*-*-*-iso8859-*"
#define DEFAULT_USE_SHM    1

typedef struct OPTIONS {
  char data_dir[256];        /* Base data directory */
  char map_file[256];        /* Startup map file */
  char block_bmp_file[256];  /* Block bitmap file */
  char back_bmp_file[256];   /* Back bitmap file */
  char font[256];            /* Font name */
#ifdef USE_SHM
  int use_shm;                /* 1 to use shared memory */
#endif
} OPTIONS;

void fill_default_options(OPTIONS *options);
int read_options(char *filename, OPTIONS *options);


#endif /* CONFIG_H_FILE */
