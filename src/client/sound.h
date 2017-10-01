/* sound.h
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#ifndef GAME_SOUND_H_FILE
#define GAME_SOUND_H_FILE

#include "snd_data.h"

#define SOUND_DIR     "sound/"

#if defined sun
#define DEV_DSP "/dev/audio"
#define DEV_SEQUENCER "/dev/null"
#elif defined linux
#define DEV_DSP        "/dev/dsp"
#define DEV_SEQUENCER  "/dev/sequencer"
#elif defined GRAPH_DOS
#define DEV_DSP        "???"
#define DEV_SEQUENCER  "???"
#else
#error "Unknown system, maybe disable sound?"
#endif


#define SAMPLE_SERVER  "sndsrv"
#define MIDI_SERVER    "midisrv"

int get_music_id(char *name);

void snd_play_sample(int n);
void snd_start_music(int n, int loop);

void setup_sound(char *midi_dev, char *dsp_dev);
void end_sound(void);

extern int n_musics;      /* # of musics available to play */
extern int n_samples;     /* # of samples available to play */

#endif /* GAME_SOUND_H_FILE */
