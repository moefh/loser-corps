/* sound.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#ifdef HAS_SOUND

#include <stdio.h>
#include <string.h>

#include "sound.h"
#include "common.h"
#include "game.h"


int n_samples, n_musics;
static SOUND_INFO game_sound_info;


/* Return the music ID given a music name */
int get_music_id(char *name)
{
  int i;

  for (i = 0; i < sound_info->n_musics; i++)
    if (strcmp(name, sound_info->music[i].name) == 0)
      return sound_info->music[i].id;
  return -1;
}



#ifdef GRAPH_DOS

#include <allegro.h>

static void read_sound_files(void)
{
  int i, j;
  char filename[256];

  for (j = i = 0; i < N_SAMPLES; i++) {
    snprintf(filename, sizeof(filename), "%s%s%s", DATA_DIR, SOUND_DIR, spl_file[i]);
    snd_sample[j] = load_sample(filename);
    if (snd_sample[j] != NULL)
      j++;
  }
  n_samples = j;

  for (j = i = 0; i < N_MUSICS; i++) {
    snprintf(filename, sizeof(filename), "%s%s%s", DATA_DIR, SOUND_DIR, midi_file[i]);
    snd_midi[j] = load_midi(filename);
    if (snd_midi[j] != NULL)
      j++;
  }
  n_musics = j;
}

/* Initialize the sound system */
void setup_sound(char *midi_dev, char *dsp_dev)
{
  read_sound_files();
}

/* Close the sound system */
void end_sound(void) { }

/* Play a sound sample */
void snd_play_sample(int n)
{
  if (n >= 0 && n < n_samples)
    play_sample(snd_sample[n], 255, 128, 1000, 0);
}

/* Start a music. If `loop' is `, the music will restart when it ends. */
void snd_start_music(int n, int loop)
{
  if (n >= 0 && n < n_musics)
    play_midi(snd_midi[n], loop);
}


#else /* GRAPH_DOS */

#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>


static int sample_server_fd, midi_server_fd;
static pid_t sample_server_pid, midi_server_pid;

#ifndef WAIT_ANY
#define WAIT_ANY (-1)
#endif

static void sigchld_handler(int signum)
{
  pid_t pid;
  int status;

  while (1) {
    pid = waitpid(WAIT_ANY, &status, WNOHANG);
#if 0
    if (pid < 0) {
      printf("waitpid(): %s\n", strerror(errno));
      break;
    }
#endif
    if (pid <= 0)
      break;

    if (pid == sample_server_pid) {       /* Sample server terminated */
      close(sample_server_fd);
      sample_server_fd = -1;
      sample_server_pid = -1;
    } else if (pid == midi_server_pid) {  /* MIDI server terminated */
      close(midi_server_fd);
      midi_server_fd = -1;
      midi_server_pid = -1;
    }    
  }

  signal(SIGCHLD, sigchld_handler);
}


/* Fork and execute a sound server */
static int install_sound_server(pid_t *pid_return,
				char *server, char *dev,
				SOUND_FILE *files, int n_files)
{
  int fd[2];
  pid_t pid;

  {
    /* Try to open the device here. If it does not succeed,
     * we won't even fork to execute the sound server */
    int test_fd;

    test_fd = open(dev, O_WRONLY);
    if (test_fd < 0) {
      /* printf("%s: %s\n", dev, err_msg()); */
      return -1;
    }
    close(test_fd);
  }

  pipe(fd);
  if ((pid = fork()) < 0) {
    printf("install_sound_server(): can't fork()\n");
    close(fd[0]);
    close(fd[1]);
    return -1;
  }

  if (pid == 0) {     /* Child */
    char **args;
    int i;

    close(fd[1]);
    dup2(fd[0], 0);    /* fd[0] will be server's STDIN */

    args = (char **) malloc(sizeof(char *) * (n_files + 4));
    args[2] = (char *) malloc(strlen(DATA_DIR) + strlen(SOUND_DIR) + 1);
    if (args == NULL || args[2] == NULL) {
      printf("Sound: out of memory to execute `%s'\n", server);
      _exit(1);
    }
    args[0] = server;
    args[1] = dev;
    sprintf(args[2], "%s%s", DATA_DIR, SOUND_DIR);
    for (i = 0; i < n_files; i++)
      args[i + 3] = files[i].file;
    args[i + 3] = NULL;

    execvp(server, args);
#if 0
    printf("Sound: can't exec `%s'\n", server);
#endif
    _exit(1);
  }

  *pid_return = pid;
  close(fd[0]);
  return fd[1];
}

/* Play a sound sample */
void snd_play_sample(int n)
{
  fd_set set;
  struct timeval tv;

  if (sample_server_fd < 0 || sample_server_pid == 0)
    return;

  FD_ZERO(&set);
  FD_SET(sample_server_fd, &set);
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  if (select(FD_SETSIZE, NULL, &set, NULL, &tv) < 0)
    kill(sample_server_pid, SIGTERM);
  else
    write(sample_server_fd, &n, sizeof(int));
}

/* Start a music.  If `loop' is 1, the music will be restarted when it
 * ends */
void snd_start_music(int n, int loop)
{
  static int music_started = 0;
  fd_set set;
  struct timeval tv;

  if (midi_server_fd < 0 || midi_server_pid == 0)
    return;

  FD_ZERO(&set);
  FD_SET(midi_server_fd, &set);
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  if (music_started) {
    kill(midi_server_pid, SIGUSR1);     /* interrupt wait state */
    music_started = 0;
  }

  if (select(FD_SETSIZE, NULL, &set, NULL, &tv) < 0)
    kill(midi_server_pid, SIGTERM);
  else {
    int data[2];

    data[0] = n;
    data[1] = loop;
    write(midi_server_fd, data, sizeof(data));
  }
  music_started = 1;
}

/* Initialize the sound system */
void setup_sound(char *midi_dev, char *dsp_dev)
{
  char info_filename[1024];

  sound_info = &game_sound_info;

  snprintf(info_filename, sizeof(info_filename), "%ssound.dat", DATA_DIR);
  if (parse_sound_info(sound_info, info_filename)) {
    sample_server_pid = midi_server_pid = 0;
    return;
  }

  if (midi_dev == NULL && dsp_dev == NULL) {
    sample_server_pid = midi_server_pid = 0;
    return;
  }

  signal(SIGCHLD, sigchld_handler);

  if (dsp_dev != NULL && sound_info->n_samples != 0) {
    n_samples = sound_info->n_samples;
    sample_server_fd = install_sound_server(&sample_server_pid,
					    SAMPLE_SERVER, dsp_dev,
					    sound_info->sample,
					    sound_info->n_samples);
    if (sample_server_fd < 0)
      n_samples = 0;
  }

  if (midi_dev != NULL && sound_info->n_musics != 0) {
    n_musics = sound_info->n_samples;
    midi_server_fd = install_sound_server(&midi_server_pid,
					  MIDI_SERVER, midi_dev,
					  sound_info->music,
					  sound_info->n_musics);
    if (midi_server_fd < 0)
      n_musics = 0;
  }
}

/* Close the sound system */
void end_sound(void)
{
  if (sample_server_pid > 0)
    kill(sample_server_pid, SIGTERM);
  if (midi_server_pid > 0)
    kill(midi_server_pid, SIGTERM);
}

#endif /* GRAPH_DOS */

#endif /* HAS_SOUND */
