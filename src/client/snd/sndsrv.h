/* sndsrv.h */


#ifndef SNDSRV_H_FILE
#define SNDSRV_H_FILE

#ifndef ABS
#define ABS(a)   (((a) < 0) ? -(a) : (a))
#endif

typedef struct SAMPLE {
  int freq;               /* Sampling frequency */
  int bits;               /* Bits per sample */
  int len;                /* Number of samples */
  unsigned char *data;
} SAMPLE;


SAMPLE *load_wav(char *filename);
void destroy_sample(SAMPLE *sample);
int do_play_loop(char *prog_name, char *dsp_dev,
		 int pipe_fd, SAMPLE **list, int n_samples);


#endif /* SNDSRV_H_FILE */
