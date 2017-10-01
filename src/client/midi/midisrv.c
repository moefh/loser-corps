/* midisrv.c */

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "midisrv.h"

#define MAX_MIDIS  256


int read_files(char *prog_name, char *prefix, char **file_list, MIDI **list)
{
  int i;
  char filename[256];

  for (i = 0; i < MAX_MIDIS && file_list[i] != NULL; i++) {
    strcpy(filename, prefix);
    strcat(filename, file_list[i]);

    if ((list[i] = load_midi(filename)) == NULL)
      printf("%s: can't read file `%s'\n", prog_name, filename);
  }
  return i;
}


/* Do the main program loop */

int main(int argc, char *argv[])
{
  MIDI *midi_list[MAX_MIDIS];
  int n_midis;

  if (argc < 4)
    return 0;

  n_midis = read_files(argv[0], argv[2], argv + 3, midi_list);
  if (n_midis == 0) {
    printf("%s: no midi files\n", argv[0]);
    return 0;
  }

  do_play_loop(argv[0], argv[2], argv[1], 0, midi_list, n_midis);

  return 0;
}
