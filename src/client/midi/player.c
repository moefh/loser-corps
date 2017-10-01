/* player.c
 *
 * This is based on `playmidi', Copyright (C) 1994-1997 Nathan I. Laredo.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <malloc.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#if USE_AWE32_DRIVER
#include <linux/awe_voice.h>
#endif

#include "midisrv.h"

#define DEFAULT_TEMPO   500000
#define TEMPO_SKEW      1.0

static volatile int midi_quit, midi_reset;
static char *prog_name, *sequencer_dev;
static int seqfd, dev, n_voices;
unsigned long int __midi_ticks;

static unsigned long tempo, lasttime;
static unsigned int lowtime, len;
static unsigned char *data;
static double current, dtime;


enum {      /* MIDI drivers */
#if USE_AWE32_DRIVER
  AWE32_DRIVER,
#endif
  FM_DRIVER,
};

/* MIDI drivers, in the same order of the above enumeration */
static MIDI_DRIVER *midi_drivers[] = {
#if USE_AWE32_DRIVER
  &__midi_awe32_driver,
#endif
  &__midi_fm_driver,
};
static MIDI_DRIVER *driver;      /* The driver we will be using */


SEQ_DEFINEBUF(128);

static struct {
  int index;
  int running_st;
  unsigned long ticks;
} seq[MIDI_TRACKS];


void seqbuf_dump(void)
{
  if (midi_reset)
    return;
  if (_seqbufptr) {
    if (write(seqfd, _seqbuf, _seqbufptr) == -1 && errno != EINTR) {
      printf("write to %s: %s\n", sequencer_dev, strerror(errno));
      close(seqfd);
      exit(1);
    }
  }
  _seqbufptr = 0;
}

/* Read a variable length value from a midi track */
static unsigned long read_val(MIDI *m, int i)
{
  unsigned long int value = 0;
  unsigned char c;

  if (m->track[i].data == NULL)
    return 0;

  if (seq[i].index < m->track[i].len
      && ((value = m->track[i].data[seq[i].index++]) & 0x80) != 0) {
    value &= 0x7f;
    do {
      if (seq[i].index >= m->track[i].len)
	c = 0;
      else
	value = ((value << 7)
		 + ((c = m->track[i].data[seq[i].index++]) & 0x7f));
    } while (c & 0x80);
  }
  return value;
}

/* Length of the MIDI commands */
static int cmd_len[16] = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  2, 2, 2, 2,
  1, 1, 2, 0
};

#define CMD   seq[track].running_st
#define TIME  seq[track].ticks
#define CHN   (CMD & 0x0f)
#define NOTE  data[0]
#define VEL   data[1]


static void seq_reset(void)
{
  midi_reset = 0;
  _seqbufptr = __midi_ticks = 0;
  driver->reset(seqfd, dev, n_voices);

  {
    int i;

    tempo = DEFAULT_TEMPO;
    lasttime = 0;
    current = dtime = 0.0;
    for (i = 0; i < MIDI_TRACKS; i++) {
      seq[i].index = seq[i].running_st = 0;
      seq[i].ticks = 0;
    }
  }
}

/***********/

/* Initialize the midi player: open the sequencer device */
int install_midi(char *data_dir)
{
  int i, n_synths;
  struct synth_info card;

  seqfd = open(sequencer_dev, O_WRONLY);
  if (seqfd < 0) {
    printf("%s: %s: %s\n", prog_name, sequencer_dev, strerror(errno));
    return 1;
  }
  if (ioctl(seqfd, SNDCTL_SEQ_NRSYNTHS, &n_synths) == -1) {
    printf("%s: can't read number of synths\n", prog_name);
    close(seqfd);
    return 1;
  }

  dev = -1;
  for (i = 0; i < n_synths; i++) {
    card.device = i;
    if (ioctl(seqfd, SNDCTL_SYNTH_INFO, &card) == -1) {
      close(seqfd);
      printf("%s: can't get info on synth %d\n", prog_name, i);
      return 1;
    }

#if USE_AWE32_DRIVER
    if (card.synth_type == SYNTH_TYPE_SAMPLE
	&& card.synth_subtype == SAMPLE_TYPE_AWE32) {
      dev = i;
      driver = midi_drivers[AWE32_DRIVER];
    } else
#endif
    if (dev < 0 && card.synth_type == SYNTH_TYPE_FM) {
      dev = i;
      driver = midi_drivers[FM_DRIVER];
      n_voices = 12;
    }
  }

  if (dev < 0) {
    close(seqfd);
    printf("%s: no suitable synth found\n", prog_name);
    return 1;
  }

  if (driver->load(seqfd, dev, data_dir)) {
    close(seqfd);
    printf("%s: can't load patches\n", prog_name);
    return 1;
  }

  return 0;
}

#if 0
int load_prog_sample(int prog, SAMPLE *s,
		     int key, int loop_start, int loop_end)
{
  return driver->load_prog_sample(seqfd, dev, prog, s,
				  key, loop_start, loop_end);
}

int unload_prog_samples(void)
{
  return driver->unload_prog_samples(seqfd, dev);
}
#endif

/***********************************/
/***********************************/


/* Prepare to play a music */
static void start_play(MIDI *midi)
{
  int track, chan;

  tempo = DEFAULT_TEMPO;
  lasttime = 0;
  current = dtime = 0.0;

  seq_reset();
  for (track = 0; track < MIDI_TRACKS && midi->track[track].data != NULL;
       track++) {
    seq[track].index = seq[track].running_st = 0;
    seq[track].ticks = read_val(midi, track);
  }
  for (chan = 0; chan < 16; chan++) {
    driver->control(seqfd, dev, chan, CTL_BANK_SELECT, 0);
    driver->control(seqfd, dev, chan, CTL_EXT_EFF_DEPTH, 0 /* reverb */);
    driver->control(seqfd, dev, chan, CTL_CHORUS_DEPTH, 0 /* chorus */);
    driver->control(seqfd, dev, chan, CTL_MAIN_VOLUME, 127);
    driver->chan_pressure(seqfd, dev, chan, 127);
    driver->control(seqfd, dev, chan, 0x4a, 127);
  }

  SEQ_START_TIMER();
  SEQ_DUMPBUF();
}


/* Execute a step in the music.
 * Return 1 to continue playing, 0 if music is terminated. */
static int step_play(MIDI *midi)
{
  int track, chan;
  int playing = 1;

  if (midi_reset)
    return 1;

  lowtime = ~0;
  for (chan = track = 0;
       track < MIDI_TRACKS && midi->track[track].data != NULL;
       track++)
    if (TIME < lowtime) {
      chan = track;
      lowtime = TIME;
    }
  if (lowtime == ~0)
    return 0;              /* End of all tracks */
  track = chan;

  /* Read the command and its length */
  if (seq[track].index < midi->track[track].len
      && (midi->track[track].data[seq[track].index] & 0x80))
    CMD = midi->track[track].data[seq[track].index++];
  if (seq[track].running_st == 0xff
      && seq[track].index < midi->track[track].len)
    CMD = midi->track[track].data[seq[track].index++];
  if (CMD > 0xf7)
    len = 0;
  else if ((len = cmd_len[(CMD & 0xf0) >> 4]) == 0)
    len = read_val(midi, track);

  if (seq[track].index + len < midi->track[track].len) {
    data = midi->track[track].data + seq[track].index;
    if (CMD == 0x51  /* set tempo */)
      tempo = ((data[0] << 16) | (data[1] << 8) | data[2]);
    if (TIME > lasttime) {
      dtime = ((double) ((TIME - lasttime) * (tempo / 10000))
	       / (double) (midi->divisions)) * TEMPO_SKEW;
      current += dtime;
      lasttime = seq[track].ticks;

      if (dtime > 4096.0)     /* Stop if more than 40 sec of nothing */
	playing = 0;
      else if ((int) current > __midi_ticks) {
	SEQ_WAIT_TIME((__midi_ticks = (int) current));
	SEQ_DUMPBUF();
      }
    }
    if (playing && CMD > 0x7f)
      switch (CMD & 0xf0) {
      case MIDI_KEY_PRESSURE: 
	driver->key_pressure(seqfd, dev, CHN, NOTE, VEL);
	break;
	
      case MIDI_NOTEON:
	driver->start_note(seqfd, dev, CHN, NOTE, VEL);
	break;
	
      case MIDI_NOTEOFF:
	driver->stop_note(seqfd, dev, CHN, NOTE, VEL);
	break;
	
      case MIDI_CTL_CHANGE:
	driver->control(seqfd, dev, CHN, NOTE, VEL);
	break;
	
      case MIDI_CHN_PRESSURE:
	driver->chan_pressure(seqfd, dev, CHN, NOTE);
	break;
	
      case MIDI_PITCH_BEND:
	driver->bender(seqfd, dev, CHN, NOTE, VEL);
	break;
	
      case MIDI_PGM_CHANGE:
	driver->set_patch(seqfd, dev, CHN, NOTE);
	break;
	
      case MIDI_SYSTEM_PREFIX:
	if (len > 1)
	  driver->load_sysex(seqfd, dev, len, data, CMD);
	break;
	
      default:
	break;
      }
    if (midi_reset)
      return 1;
    ioctl(seqfd, SNDCTL_SEQ_SYNC, 0);
  }
  
  seq[track].index += len;
  if (seq[track].index >= midi->track[track].len)
    seq[track].ticks = ~0;
  else
    seq[track].ticks += read_val(midi, track);

  return playing;
}


void sigterm_handler(int sig)
{
  midi_quit = 1;
  signal(sig, sigterm_handler);
}

static void sigusr1_handler(int sig)
{
  midi_reset = 1;
  signal(sig, sigusr1_handler);
}


void do_play_loop(char *program_name, char *data_dir, char *seq_dev,
		  int pipe_fd, MIDI **list, int n_midis)
{
  fd_set pipe_set;
  struct timeval tv, *use_tv;
  int midi_playing = 0, midi_loop = 0;
  MIDI *cur_music = NULL;

  prog_name = program_name;
  sequencer_dev = seq_dev;

  if (install_midi(data_dir))
    return;

  midi_quit = midi_reset = 0;
  signal(SIGTERM, sigterm_handler);
  signal(SIGUSR1, sigusr1_handler);

  while (! midi_quit) {
    if (midi_reset)
      seq_reset();
    FD_ZERO(&pipe_set);
    FD_SET(pipe_fd, &pipe_set);
    if (midi_playing) {
      tv.tv_sec = 0;
      tv.tv_usec = 0;
      use_tv = &tv;
    } else
      use_tv = NULL;

    while (1) {
      if (select(FD_SETSIZE, &pipe_set, NULL, NULL, use_tv) < 0) {
	if (errno != EINTR) {
	  printf("%s: select: %s\n", prog_name, strerror(errno));
	  midi_quit = 1;
	  break;
	}
      } else
	break;
    }
    if (midi_quit)
      continue;
    if (FD_ISSET(pipe_fd, &pipe_set)) {    /* New music to play */
      int new_mus_data[2];

      if (read(pipe_fd, new_mus_data, 2 * sizeof(int)) != 2 * sizeof(int))
	break;     /* The pipe was closed */

      seq_reset();
      midi_playing = 0;
      if (new_mus_data[0] >= 0 && new_mus_data[0] < n_midis
	  && list[new_mus_data[0]] != NULL) {
	cur_music = list[new_mus_data[0]];
	midi_loop = new_mus_data[1];
	start_play(cur_music);
	midi_playing = 1;
      }
    }

    if (midi_playing) {
      if (step_play(cur_music) == 0) {  /* Music terminated */
	SEQ_DUMPBUF();
	if (midi_loop)
	  start_play(cur_music);        /* Re-start */
	else
	  midi_playing = 0;
      }
    }
  }

  seq_reset();
  close(seqfd);
}
