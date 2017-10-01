/* sndsrv.c */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "sndsrv.h"

#define MAX_SAMPLES   256

int read_files(char *prog_name, char *prefix, char **name_list, SAMPLE **list)
{
  int i;
  char filename[256];

  for (i = 0; i < MAX_SAMPLES && name_list[i] != NULL; i++) {
    strcpy(filename, prefix);
    strcat(filename, name_list[i]);
    if ((list[i] = load_wav(filename)) == NULL)
      printf("%s: can't read file `%s', ignoring\n", prog_name, filename);
  }
  return i;
}

int main(int argc, char *argv[])
{
  SAMPLE *sample_list[MAX_SAMPLES];
  int n_samples;

  if (argc < 4)
    return 0;

  n_samples = read_files(argv[0], argv[2], argv + 3, sample_list);
  if (n_samples == 0) {
    printf("%s: no valid samples\n", argv[0]);
    return 0;
  }

  do_play_loop(argv[0], argv[1], 0, sample_list, n_samples);

  return 0;
}
