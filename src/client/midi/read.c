/* midi.c
 *
 * This is based on the Allegro library by Shawn Hargreaves.
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "midisrv.h"


/* Free a MIDI structure */
void destroy_midi(MIDI *m)
{
  int i;

  for (i = 0; i < MIDI_TRACKS; i++)
    if (m->track[i].data != NULL)
      free(m->track[i].data);
  free(m);
}

/* Read a word (2 bytes) from a file */
static int get_w(FILE *f)
{
  int i;

  i = fgetc(f);
  i = (i << 8) | (fgetc(f) & 0xff);
  return i;
}

/* Read a double word (4 bytes) from a file */
static int get_l(FILE *f)
{
  int i = 0, j;

  for (j = 0; j < 4; j++)
    i = (i << 8) | (fgetc(f) & 0xff);
  return i;
}


/* Read a midi file into a MIDI structure */
MIDI *load_midi(char *filename)
{
  FILE *f;
  MIDI *m;
  int i, num_tracks;
  char buf[32];

  if ((f = fopen(filename, "rb")) == NULL)
    return NULL;

  m = malloc(sizeof(MIDI));
  if (m == NULL) {
    fclose(f);
    return NULL;
  }

  for (i = 0; i < MIDI_TRACKS; i++) {
    m->track[i].len = 0;
    m->track[i].data = NULL;
  }

  fread(buf, 1, 4, f);                /* MIDI file header */
  if (memcmp(buf, "MThd", 4) != 0)
    goto load_midi_error;

  fread(buf, 1, 4, f);                /* Skip header length */

  i = get_w(f);                       /* MIDI file type */
  if (i != 0 && i != 1)
    goto load_midi_error;

  num_tracks = get_w(f);                /* Number of tracks */
  if (num_tracks < 0 || num_tracks > MIDI_TRACKS)
    goto load_midi_error;

  m->divisions = get_w(f);            /* MIDI divisions */
  if (m->divisions < 0)
    m->divisions = -(m->divisions);

  for (i = 0; i < num_tracks; i++) {
    fread(buf, 1, 4, f);              /* MIDI track header */
    if (memcmp(buf, "MTrk", 4) != 0)
      goto load_midi_error;

    m->track[i].len = get_l(f);
    m->track[i].data = (unsigned char *) malloc(m->track[i].len);
    if (m->track[i].data == NULL)
      goto load_midi_error;

    if (fread(m->track[i].data, 1, m->track[i].len, f) != m->track[i].len)
      goto load_midi_error;
  }

  fclose(f);
  return m;

load_midi_error:    /* We get here if anything wrong happens */
  fclose(f);
  destroy_midi(m);
  return NULL;
}


