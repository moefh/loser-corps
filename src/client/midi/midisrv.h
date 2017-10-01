/* midisrv.h */

#ifndef MIDISRV_H_FILE
#define MIDISRV_H_FILE

#include "driver.h"


#define MIDI_TRACKS   32

typedef struct MIDI {
  int divisions;
  struct {
    int len;
    unsigned char *data;
  } track[MIDI_TRACKS];
} MIDI;


extern MIDI_DRIVER __midi_awe32_driver, __midi_fm_driver;


MIDI *load_midi(char *filename);
void destroy_midi(MIDI *midi);

void do_play_loop(char *program_name, char *data_dir, char *seq_dev,
		  int pipe_fd, MIDI **list, int n_midis);


#endif /* MIDISRV_H_FILE */
