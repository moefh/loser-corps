/* fm.c
 *
 * This is based on `playmidi', Copyright (C) 1994-1997 Nathan I. Laredo.
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include "driver.h"

#define MELODIC_FILE   "melodic.o3"
#define DRUMS_FILE     "drums.o3"

#define VOICE_SIZE     60    /* 56 for 2-operator OPL */


static int set_patch(int, int, int, int);

SEQ_DECLAREBUF();

struct {
  int program;
  int bender, bender_range;
  int controller[255];
  int pressure;
} channel[16];

struct {
  int note, channel;
  int timestamp;
  int dead;
} voice[2][36];

static int note_vel[16][128];
static int fmloaded[256], n_voices;


static void adjustfm(char *buf, int key)
{
  unsigned char pan = 2 << 4;

  if (key == FM_PATCH) {
    buf[39] &= 0xc0;
    if (buf[46] & 1)
      buf[38] &= 0xc0;
    buf[46] = (buf[46] & 0xcf) | pan;
  } else {
    int mode;
    if (buf[46] & 1)
      mode = 2;
    else
      mode = 0;
    if (buf[57] & 1)
      mode++;
    buf[50] &= 0xc0;
    if (mode == 3)
      buf[49] &= 0xc0;
    if (mode == 1)
      buf[39] &= 0xc0;
    if (mode == 2 || mode == 3)
      buf[38] &= 0xc0;
    buf[46] = (buf[46] & 0xcf) | pan;
    buf[57] = (buf[57] & 0xcf) | pan;
  }
}

static int load(int seqfd, int dev, char *data_dir)
{
  int sbfd, i, n, data_size;
  char filename[256];
  char buf[VOICE_SIZE];
  struct sbi_instrument instr;

  for (i = 0; i < 256; i++)
    fmloaded[i] = 0;
  snprintf(filename, sizeof(filename), "%s/%s", data_dir, MELODIC_FILE);
  sbfd = open(filename, O_RDONLY, 0);
  if (sbfd < 0)
    return 1;
  instr.device = dev;

  for (i = 0; i < 128; i++) {
    if (read(sbfd, buf, VOICE_SIZE) != VOICE_SIZE) {
      close(sbfd);
      return 1;
    }
    instr.channel = i;

    if (strncmp(buf, "4OP", 3) == 0) {
      instr.key = OPL3_PATCH;
      data_size = 22;
    } else {
      instr.key = FM_PATCH;
      data_size = 11;
    }

    fmloaded[i] = instr.key;

    adjustfm(buf, instr.key);
    for (n = 0; n < 32; n++)
      instr.operators[n] = (n < data_size) ? buf[36 + n] : 0;

    SEQ_WRPATCH(&instr, sizeof(instr));
  }
  close(sbfd);

  snprintf(filename, sizeof(filename), "%s/%s", data_dir, DRUMS_FILE);
  sbfd = open(filename, O_RDONLY, 0);

  for (i = 128; i < 175; i++) {
    if (read(sbfd, buf, VOICE_SIZE) != VOICE_SIZE) {
      close(sbfd);
      return 1;
    }
    instr.channel = i;

    if (strncmp(buf, "4OP", 3) == 0) {
      instr.key = OPL3_PATCH;
      data_size = 22;
    } else {
      instr.key = FM_PATCH;
      data_size = 11;
    }
    fmloaded[i] = instr.key;

    adjustfm(buf, instr.key);
    for (n = 0; n < 32; n++)
      instr.operators[n] = (n < data_size) ? buf[n + 36] : 0;

    SEQ_WRPATCH(&instr, sizeof(instr));
  }
  close(sbfd);
  return 0;
}

#if 0
static int drv_load_prog_sample(int seqfd, int dev, int prog, SAMPLE *spl,
				int key, int loop_start, int loop_end)
{
  return 1;     /* Error: can't load sample in FM */
}

static int drv_unload_prog_samples(int seqfd, int dev)
{
  return 1;
}
#endif

static void reset(int seqfd, int dev, int num_voices)
{
  int i, j;

  n_voices = num_voices;

  ioctl(seqfd, SNDCTL_SEQ_RESET);
  for (i = 0; i < 16; i++) {
    set_patch(seqfd, dev, i, 0);
    for (j = 0; j < 128; j++)
      note_vel[i][j] = 0;
    channel[i].bender = 8192;
    channel[i].bender_range = 2;
    channel[i].controller[CTL_PAN] = 64;
    channel[i].controller[CTL_SUSTAIN] = 0;
  }
  ioctl(seqfd, SNDCTL_FM_4OP_ENABLE, &dev);
  for (i = 0; i < n_voices; i++) {
    SEQ_CONTROL(dev, i, SEQ_VOLMODE, VOL_METHOD_LINEAR);
    if (voice[1][i].note)
      SEQ_STOP_NOTE(dev, i, voice[1][i].note, 64);
    voice[1][i].dead = voice[1][i].timestamp = -1;
  }

  SEQ_DUMPBUF();
}

static void load_sysex(int seqfd, int dev,
		       int len, unsigned char *data, int type)
{
  /* Currently we just ignore any sysex... */
}

static int set_patch(int seqfd, int dev, int chan, int prog)
{
  if (chan == 10 && ! fmloaded[prog])
    return 1;
  channel[chan].program = prog;
  return 0;
}

static void stop_note(int seqfd, int dev, int chan, int note, int vel)
{
  int i;

  note_vel[chan][note] = 0;
  for (i = 0; i < n_voices; i++)
    if (voice[1][i].channel == chan && voice[1][i].note == note) {
      voice[1][i].dead = -1;
      voice[1][i].timestamp /= 2;
      if (! channel[chan].controller[CTL_SUSTAIN] && chan != 10)
	SEQ_STOP_NOTE(dev, i, note, vel);
    }
}

static int new_voice(int dev, int chan)
{
  int last, oldest, i;

  if (fmloaded[channel[chan].program] == OPL3_PATCH)
    last = 6;
  else
    last = n_voices;

  for (i = oldest = 0; i < last; i++)
    if (voice[1][i].timestamp < voice[1][oldest].timestamp)
      oldest = i;
  return oldest;
}

static void start_note(int seqfd, int dev, int chan, int note, int vel)
{
  int v, c;

  if (chan == 10 && vel && set_patch(seqfd, dev, chan, note + 128) < 0)
    return;

  if (vel == 0) {
    stop_note(seqfd, dev, chan, note, vel);
    return;
  }
  v = new_voice(dev, chan);
  SEQ_SET_PATCH(dev, v, channel[chan].program);
  SEQ_BENDER_RANGE(dev, v, (channel[chan].bender_range * 100));
  SEQ_BENDER(dev, v, channel[chan].bender);
  SEQ_CONTROL(dev, v, CTL_PAN, channel[chan].controller[CTL_PAN]);
  SEQ_START_NOTE(dev, v, note, vel);
  voice[1][v].note = note;
  voice[1][v].channel = chan;
  voice[1][v].timestamp = __midi_ticks;
  voice[1][v].dead = 0;
  if ((c = channel[chan].controller[CTL_CHORUS_DEPTH] * 8)) {
    if (channel[chan].bender_range)
      c /= channel[chan].bender_range;
    v = new_voice(dev, chan);
    SEQ_SET_PATCH(dev, v, channel[chan].program);
    SEQ_BENDER_RANGE(dev, v, (channel[chan].bender_range * 100));
    if (channel[chan].bender + c < 0x4000) {
      SEQ_BENDER(dev, v, channel[chan].bender + c);
    } else {
      SEQ_BENDER(dev, v, channel[chan].bender - c);
    }
    c = channel[chan].controller[CTL_PAN];
    if (c < 64)
      c = 0;
    else if (c > 64)
      c = 127;
    SEQ_CONTROL(dev, v, CTL_PAN, c);
    SEQ_START_NOTE(dev, v, note, vel);
    voice[1][v].note = note;
    voice[1][v].channel = chan;
    voice[1][v].timestamp /= 2;
    voice[1][v].dead = 0;
  }
}

static void key_pressure(int seqfd, int dev, int chan, int note, int vel)
{
  int i;

  if (chan == 10 && vel && set_patch(seqfd, dev, chan, note + 128) < 0)
    return;

  for (i = 0; i < n_voices; i++)
    if (voice[1][i].channel == chan && voice[1][i].note == note)
      SEQ_KEY_PRESSURE(dev, i, note, vel);
}

static int rpn1[16] = {
  127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
};
static int rpn2[16] = {
  127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,
};

static void control(int seqfd, int dev, int chan, int p1, int p2)
{
  int i;

  channel[chan].controller[p1] = p2;
  if (p1 == 7 || p1 == 39)
    return;
  switch (p1) {
  case CTL_SUSTAIN:
    if (p2 == 0)
      for (i = 0; i < n_voices; i++)
	if (voice[1][i].channel == chan && voice[1][i].dead) {
	  SEQ_STOP_NOTE(dev, i, voice[1][i].note, 64);
	  voice[1][i].dead = 0;
	}
    break;

  case CTL_REGIST_PARM_NUM_MSB:
    rpn1[chan] = p2;
    break;

  case CTL_REGIST_PARM_NUM_LSB:
    rpn2[chan] = p2;
    break;

  case CTL_DATA_ENTRY:
    if (rpn1[chan] == 0 && rpn2[chan] == 0) {
      channel[chan].bender_range = p2;
      rpn1[chan] = rpn2[chan] = 127;
      for (i = 0; i < n_voices; i++)
	SEQ_BENDER_RANGE(dev, i, p2 * 100);
    }

  default:
    if (p1 < 0x10 || (p1 & 0xf0) == 0x50)
      for (i = 0; i < n_voices; i++)
	if (voice[1][i].channel == chan)
	  SEQ_CONTROL(dev, i, p1, p2);
    break;
  }
}

static void chan_pressure(int seqfd, int dev, int chan, int vel)
{
  int i;

  channel[chan].pressure = vel;
  for (i = 0; i < n_voices; i++)
    if (voice[1][i].channel == chan)
      SEQ_KEY_PRESSURE(dev, i, voice[1][i].note, vel);
}

static void bender(int seqfd, int dev, int chan, int p1, int p2)
{
  int i, val;

  val = ((p2 & 0xff) << 7) | (p1 & 0xff);
  channel[chan].bender = val;

  for (i = 0; i < n_voices; i++)
    if (voice[1][i].channel == chan)
      SEQ_BENDER(dev, i, val);
}


MIDI_DRIVER __midi_fm_driver = {
  "SYNTH_FM",
  "SoundBlaster FM Synth",
  load,
  reset,
  set_patch,
  stop_note,
  start_note,
  key_pressure,
  control,
  chan_pressure,
  bender,
  load_sysex,
#if 0
  drv_load_prog_sample,
  drv_unload_prog_samples,
#endif
};
