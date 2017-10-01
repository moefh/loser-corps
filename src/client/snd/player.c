/* player.c
 *
 * This is heavily based on the `SoundIt'
 * library, Copyright (C) 1994 Brad Pitzel.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef sun
#define SOLARIS_AUDIO
#endif
#ifdef linux
#define LINUX_AUDIO
#endif

#ifdef LINUX_AUDIO
#include <linux/soundcard.h>
#endif
#ifdef SOLARIS_AUDIO
#include <sys/audioio.h>
#endif

#include <sys/ioctl.h>

#include "sndsrv.h"

#define DSP_FRAGMENT   0x00020009   /* 2 buffers of 2^9 = 512 bytes each */
#define NUM_CHANNELS   4            /* Increase to play more waves at the
			             * same time at cost of some CPU burn */


typedef struct CHANNEL {
  unsigned char *start, *pos;
  int len;                      /* Length of data, in samples */
  int left;                     /* # of samples left to play */
} CHANNEL;

typedef struct MIXER {
  int size;                  /* Buffer size, in samples */
  unsigned char *clipped;    /* Can be read as `short int' */
  int *unclipped;
} MIXER;

static CHANNEL channel[NUM_CHANNELS];
static int bits_per_sample;	/* size of samples from sound files */
static MIXER mixer;
static int device_sample_size;	/* may be different from bits_per_sample */


static void reset_channel(CHANNEL *ch)
{
  ch->start = ch->pos = NULL;
  ch->len = ch->left = 0;
}

static int allocate_channel(CHANNEL *list, unsigned char *data)
{
  int i, len, i_len;

  for (i = 0; i < NUM_CHANNELS; i++)
    if (list[i].start == NULL || list[i].left <= 0)
      return i;  /* Free */

  /* No channel available: look for one playing the asked data */
  i_len = -1;
  len = 0;
  for (i = 0; i < NUM_CHANNELS; i++)
    if (list[i].start == data && list[i].pos - list[i].start > len) {
      i_len = i;
      len = list[i].pos - list[i].start;
    }
  if (i_len >= 0)
    return i_len;

  /* None found: replace any channel */
  i_len = -1;
  len = 0;
  for (i = 0; i < NUM_CHANNELS; i++)
    if (list[i].pos - list[i].start > len) {
      i_len = i;
      len = list[i].pos - list[i].start;
    }
  return i_len;
}

static void assign_channel(SAMPLE *s, CHANNEL *ch)
{
  ch->start = ch->pos = s->data;
  ch->len = ch->left = s->len;
}

static void mix_channel(MIXER *mix, CHANNEL *ch)
{
  int *dest, len, i;

  len = ((ch->left < mix->size) ? ch->left : mix->size);

  dest = mix->unclipped;
  if (bits_per_sample == 8)
    for (i = 0; i < len; i++)
      (*dest++) += ((int) (*ch->pos++)) - 128;
  else
    for (i = 0; i < len/2; i++) {
      (*dest++) += (int) (*(short int *)ch->pos);
      ch->pos += 2;
    }
  ch->left -= len;
}

static void mix_all(MIXER *mix, CHANNEL *ch_list)
{
  int i, *unc, val;

  unc = mix->unclipped;
  if (device_sample_size == 8)          /* Force silence */
    for (i = 0; i < mix->size; i++)
      *unc++ = 128;
  else
    for (i = 0; i < mix->size; i++)
      *unc++ = 0;

  /* Mix in all channels */
  for (i = 0; i < NUM_CHANNELS; i++)
    mix_channel(mix, ch_list + i);

  /* Clip the data */
  unc = mix->unclipped;
  if (device_sample_size == 8) {
    unsigned char *clip = mix->clipped;

    for (i = 0; i < mix->size; i++) {
      val = *unc++;
      *clip++ = ((val < 0) ? 0 : ((val > 255) ? 255 : val));
    }
  } else {
    short int *clip = (short int *) mix->clipped;

    if(bits_per_sample == 8) {
      /* convert from unsigned 128-biased 8-bit samples to signed 16-bit */
      for(i = 0; i < mix->size; i++) {
	val = (*unc++ - 128) << 8;
        *clip++ = ((val < -32768) ? -32768 : ((val > 32767) ? 32767 : val));
      }
    } else {
      for (i = 0; i < mix->size; i++) {
        val = *unc++;
        *clip++ = ((val < -32768) ? -32768 : ((val > 32767) ? 32767 : val));
      }
    }
  }
}


static int init_dsp(char *prog_name, char *dsp_dev,
		    int bits, int freq, int frag)
{
  int dsp_fd;
#ifdef LINUX_AUDIO
  int wanted_format, speed, stereo;
#endif
#ifdef SOLARIS_AUDIO
  audio_info_t info;
  /* allowed frequencies of the dbri and crystal devices */
  int freqs[] = { 8000, 9600, 11025, 16000, 18900,
		  22050, 32000, 37800, 44100, 48000, -1 };
  int i, d, min_d, best_freq;
#endif

  if ((dsp_fd = open(dsp_dev, O_WRONLY)) < 0) {
    printf("%s: %s: %s\n", prog_name, dsp_dev, strerror(errno));
    return -1;
  }

#ifdef LINUX_AUDIO
  /* Setup the sound device */
  ioctl(dsp_fd, SNDCTL_DSP_SETFRAGMENT, &frag);
  mixer.size = 0;
  if (ioctl(dsp_fd, SNDCTL_DSP_GETBLKSIZE, &mixer.size) < 0) {
    printf("%s: SNDCTL_DSP_GETBLKSIZE: %s\n", prog_name, strerror(errno));
    close(dsp_fd);
    return -1;
  }

  wanted_format = bits;
  if (ioctl(dsp_fd, SNDCTL_DSP_SETFMT, &bits) < 0 || bits != wanted_format) {
    printf("%s: SNDCTL_DSP_SETFMT: %s", prog_name, strerror(errno));
    return 1;
  }
  speed = freq;
  if (ioctl(dsp_fd, SNDCTL_DSP_SPEED, &speed) < 0 || ABS(speed-freq) > 2000) {
    printf("%s: SNDCTL_DSP_SPEED: %s", prog_name, strerror(errno));
    return 1;
  }
  stereo = 0;
  if (ioctl(dsp_fd, SNDCTL_DSP_STEREO, &stereo) < 0) {
    printf("%s: SNDCTL_DSP_STEREO: %s\n", prog_name, strerror(errno));
    close(dsp_fd);
    return -1;
  }
  device_sample_size = bits;
#endif
#ifdef SOLARIS_AUDIO
  /* find closest sample rate */
  min_d = 1000000;
  best_freq = freqs[0];
  for(i = 0; freqs[i] > 0; i++) {
      d = abs(freqs[i] - freq);
      if(d < min_d) {
	  min_d = d;
	  best_freq = freqs[i];
      }
  }
  AUDIO_INITINFO(&info);
  info.play.encoding = AUDIO_ENCODING_LINEAR;
  info.play.precision = 16;	/* device only supports 16 bit linear sound */
  info.play.sample_rate = best_freq;
  info.play.channels = 1;
  if(ioctl(dsp_fd, AUDIO_SETINFO, &info) < 0) {
    printf("%s: AUDIO_SETINFO: %s\n", prog_name, strerror(errno));
    close(dsp_fd);
    return -1;
  }
  device_sample_size = 16;
  mixer.size = 1024;
#endif

  /* Allocate memory for the mixer buffer */
  mixer.unclipped = (int *) malloc(sizeof(int) * mixer.size);
  mixer.clipped = (unsigned char *) malloc(sizeof(short) * mixer.size);
  if (mixer.unclipped == NULL || mixer.clipped == NULL) {
    printf("%s: out of memory for sound buffers\n", prog_name);
    close(dsp_fd);
    if (mixer.unclipped != NULL)
      free(mixer.unclipped);
    if (mixer.clipped != NULL)
      free(mixer.clipped);
    return -1;
  }

  return dsp_fd;
}


static int quit;

void sigterm_handler(int sig)
{
  quit = 1;
  signal(SIGTERM, sigterm_handler);
}

int do_play_loop(char *prog_name, char *dsp_dev, int pipe_fd,
		 SAMPLE **list, int n_samples)
{
  fd_set pipe_set;
  struct timeval tv;
  int sound_num, chan;
  int dsp_fd;

  quit = 0;
  signal(SIGTERM, sigterm_handler);

  if ((dsp_fd = init_dsp(prog_name, dsp_dev, list[0]->bits,
			 list[0]->freq, DSP_FRAGMENT)) < 0)
    return 1;
  bits_per_sample = list[0]->bits;

  for (chan = 0; chan < NUM_CHANNELS; chan++)
    reset_channel(channel);

  while (! quit) {
    FD_ZERO(&pipe_set);
    FD_SET(pipe_fd, &pipe_set);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    if (select(FD_SETSIZE, &pipe_set, NULL, NULL, &tv) < 0) {
      printf("select(): %s\n", strerror(errno));
      break;
    }
    if (FD_ISSET(pipe_fd, &pipe_set)) {    /* New sound to play */
      if (read(pipe_fd, &sound_num, sizeof(int)) != sizeof(int))
	break;     /* The pipe was closed */
      if (sound_num >= 0 && sound_num < n_samples && list[sound_num] != NULL) {
	chan = allocate_channel(channel, list[sound_num]->data);
#if 0
	if (chan < 0)
	  printf("can't play sample %d; all channels are busy\n", sound_num);
	else
	  printf("playing sample %d in channel %d\n", sound_num, chan);
#endif
	if (chan >= 0)
	  assign_channel(list[sound_num], channel + chan);
      }
    }

    mix_all(&mixer, channel);
    write(dsp_fd, mixer.clipped, mixer.size * (device_sample_size >> 3));
#ifdef SOLARIS_AUDIO
    ioctl(dsp_fd, AUDIO_DRAIN, 0);
#endif
  }

  close(dsp_fd);
  return 0;
}
