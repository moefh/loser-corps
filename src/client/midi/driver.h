/* driver.h */

#ifndef DRIVER_H
#define DRIVER_H

extern unsigned long int __midi_ticks;

typedef struct MIDI_DRIVER {
  char name[32], desc[32];
  int (*load)(int fd, int dev, char *data_dir);
  void (*reset)(int fd, int dev, int n_voices);
  int (*set_patch)(int fd, int dev, int chan, int prog);
  void (*stop_note)(int fd, int dev, int chan, int note, int vel);
  void (*start_note)(int fd, int dev, int chan, int note, int vel);
  void (*key_pressure)(int fd, int dev, int chan, int note, int vel);
  void (*control)(int fd, int dev, int chan, int p1, int p2);
  void (*chan_pressure)(int fd, int dev, int chan, int vel);
  void (*bender)(int fd, int dev, int chan, int p1, int p2);
  void (*load_sysex)(int fd, int dev, int len, unsigned char *data, int type);
#if 0
  int (*load_prog_sample)(int fd, int dev, int prog, SAMPLE *s,
			  int key, int loop_start, int loop_end);
  int (*unload_prog_samples)(int fd, int dev);
#endif
} MIDI_DRIVER;


#endif /* DRIVER_H */
