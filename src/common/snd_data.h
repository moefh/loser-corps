/* snd_data.h */

#ifndef SND_DATA_H
#define SND_DATA_H


typedef struct SOUND_FILE {
  int id;
  char name[32];
  char file[64];
  char author[128];
  char comment[128];
  char copyright[128];
} SOUND_FILE;

typedef struct SOUND_INFO {
  int n_samples;
  SOUND_FILE *sample;
  int n_musics;
  SOUND_FILE *music;
} SOUND_INFO;

int parse_sound_info(SOUND_INFO *sound_info, char *info_filename);

#endif

