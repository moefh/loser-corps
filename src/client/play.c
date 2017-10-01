/* play.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <signal.h>
#include <string.h>

#include "common.h"
#include "game.h"
#include "options.h"

char **program_argv;
int program_argc;

static void print_help(char *name)
{
  printf("The L.O.S.E.R Corps game client version %d.%d.%d for "
#if (defined GRAPH_SVGALIB)
	 "Linux/SVGALib"
#elif (defined GRAPH_X11)
	 "Unix/X"
#elif (defined GRAPH_DOS)
	 "DOS/Allegro"
#elif (defined GRAPH_WIN)
	 "Windows/DirectX"
#endif /* GRAPH_xxx */

#ifndef NET_PLAY
	 " (no network support)"
#endif /* NET_PLAY */

	 "\n\n",
	 VERSION1(SERVER_VERSION),
	 VERSION2(SERVER_VERSION),
	 VERSION3(SERVER_VERSION));

  printf("%s [options]\n"
	 "\n"
	 " general options:\n"
	 "   -help         show this help\n"
	 "   -debug        print error messages to stdout\n"
	 "   -fskip N      # of prames to skip\n"
	 "   -name NAME    set the player name to `NAME'\n"
	 "   -jack JACK    set jack type to `JACK'\n"
	 "   -map MAPNAME  ask server to use `MAPNAME' (if owner)\n"
	 "   -go           ask the server to start the game (or join)\n"
	 "   -rec FILE     record movie to file `FILE'\n"    
	 "   -replay FILE  replay recorded movie from file `FILE'\n"
	 "   -credits      start showing the credits\n"

#ifdef HAS_SOUND
	 "   -sound DEV    play digital sound in DEV (default: " DEV_DSP ")\n"
	 "   -nosound      don't play digital sound\n"
	 "   -midi DEV     play MIDI sound in DEV (default: "
	                   DEV_SEQUENCER ")\n"
	 "   -nomidi       don't play MIDI sound\n"
#endif /* HAS_SOUND */
	 "", name);

#ifdef NET_PLAY
  printf("   -host H       connect to server at host H (default: %s)\n"
	 "   -port P       connect to server at port P (default: %d)\n"
	 "   -nobuf        don't buffer network writes (slower but safer)\n"
	 "", DEFAULT_HOST, DEFAULT_PORT);
#endif /* NET_PLAY */

  printf("\n"
	 " platform-specific options:\n"

#ifdef GRAPH_DOS
	 "   -w W          select the screen height (default: 320)\n"
	 "   -h H          select the screen height (default: 200)\n"
	 "   -bpp N        select the color depth (default: 8)\n"
#endif /* GRAPH_DOS */

#ifdef GRAPH_X11
	 "   -w W          set the window width (default: 320)\n"
	 "   -h H          set the window height (default: 240)\n"
	 "   -double       double the window size (default)\n"
	 "   -nodouble     don't double the window size\n"
	 "   -noscan       don't skip odd lines when doubled\n"
#ifdef USE_SHM
	 "   -noshm        don't use MIT shared memory extensions\n"
#endif /* USE_SHM */
#endif /* GRAPH_X11 */

#ifdef GRAPH_SVGALIB
	 "   -v N          select video mode, where N is:\n"
	 "                   0 - 320x200x256 (default)\n"
	 "                   1 - 640x480x256\n"
	 "                   2 - 800x600x256\n"
	 "                   3 - 1024x768x256\n"
	 "                   4 - 320x200x64K\n"
	 "                   5 - 640x480x64K\n"
	 "                   6 - 800x600x64K\n"
	 "                   7 - 1024x768x64K\n"
	 "                   8 - 320x200x16M32\n"
	 "                   9 - 640x480x16M32\n"
	 "                   10 - 800x600x16M32\n"
	 "                   11 - 1024x768x16M32\n"
#endif /* GRAPH_SVGALIB */
	 "\n");
}


#define EXPECT_ARG(argv)   do					\
    if ((argv)[1] == NULL) {					\
      printf("Expected argument for option `%s'\n", (argv)[0]);	\
      exit(1);							\
    }								\
  while (0)


#ifdef GRAPH_WIN

/* Parse the options in Windows */
int parse_option(OPTIONS *options, char **args) { return 0; }

/* Fill the default options in Windows */
void fill_default_options(OPTIONS *options) { }

#endif


#ifdef GRAPH_DOS

/* Parse the options in DOS */
int parse_option(OPTIONS *options, char **args)
{
  if (strcmp(args[0], "-w") == 0) {
    EXPECT_ARG(args);
    options->width = atoi(args[1]);
    return 2;
  }
  if (strcmp(args[0], "-h") == 0) {
    EXPECT_ARG(args);
    options->height = atoi(args[1]);
    return 2;
  }
  if (strcmp(args[0], "-bpp") == 0) {
    EXPECT_ARG(args);
    options->bpp = atoi(args[1]);
    if (options->bpp == 8 || options->bpp == 16) {
      options->bpp /= 8;
      return 2;
    }
    printf("Error: color depth of %d is not supported.\n"
           "       Please choose one of { %d, %d }.\n", options->bpp, 8, 16);
    return -1;
  }
  if (strcmp(args[0], "-noflip") == 0) {
    options->page_flip = 0;
    return 1;
  }
  return 0;
}

/* Fill the default options in DOS */
void fill_default_options(OPTIONS *options)
{
  options->conn_type = CONNECT_NETWORK;
  options->gmode = GFX_AUTODETECT;
  options->width = 320;
  options->height = 200;
  options->bpp = 1;
  options->page_flip = 1;
}
#endif /* GRAPH_DOS */

#ifdef GRAPH_X11

/* Parse options in X */
int parse_option(OPTIONS *options, char **args)
{
  if (strcmp(args[0], "-w") == 0) {
    EXPECT_ARG(args);
    options->width = atoi(args[1]);
    return 2;
  }
  if (strcmp(args[0], "-h") == 0) {
    EXPECT_ARG(args);
    options->height = atoi(args[1]);
    return 2;
  }
  if (strcmp(args[0], "-noshm") == 0) {
    options->no_shm = 1;
    return 1;
  }
  if (strcmp(args[0], "-double") == 0) {
    printf("Warning: the option `-double' is assumed "
	   "by default in this version.\n");
    options->double_screen = 1;
    return 1;
  }
  if (strcmp(args[0], "-noscan") == 0) {
    options->no_scanlines = 1;
    return 1;
  }
  if (strcmp(args[0], "-nodouble") == 0) {
    options->double_screen = 0;
    return 1;
  }
  return 0;
}

/* Fill the default options in X */
void fill_default_options(OPTIONS *options)
{
  options->width = 320;
  options->height = 240;
  options->no_shm = 0;
  options->double_screen = 1;
  options->no_scanlines = 0;
}
#endif /* GRAPH_X11 */

#ifdef GRAPH_SVGALIB

/* Parse the options in SVGALIB */
int parse_option(OPTIONS *options, char **args)
{
  if (strcmp(args[0], "-v") == 0) {
    EXPECT_ARG(args);
    switch (atoi(args[1])) {
    case 0: options->gmode = G320x200x256; break;
    case 1: options->gmode = G640x480x256; break;
    case 2: options->gmode = G800x600x256; break;
    case 3: options->gmode = G1024x768x256; break;
    case 4: options->gmode = G320x200x64K; break;
    case 5: options->gmode = G640x480x64K; break;
    case 6: options->gmode = G800x600x64K; break;
    case 7: options->gmode = G1024x768x64K; break;
    case 8: options->gmode = G320x200x16M32; break;
    case 9: options->gmode = G640x480x16M32; break;
    case 10: options->gmode = G800x600x16M32; break;
    case 11: options->gmode = G1024x768x16M32; break;
    default:
      printf("Invalid video mode: `%d'\n", atoi(args[1]));
      return 0;
    }
    return 2;
  }
  return 0;
}

/* Fill the default options in SVGALIB */
void fill_default_options(OPTIONS *options)
{
  options->gmode = G320x200x256;
}

#endif /* GRAPH_SVGALIB */


static void catch_interrupt(int sig)
{
  printf("Please exit the game pressing `ESC' and then `Y'.\n");
  signal(sig, catch_interrupt);
}

int main(int argc, char *argv[])
{
  int i, n_opts;
  OPTIONS options;

  program_argc = argc;
  program_argv = argv;

  /* Set the default options */
  options.debug = 0;
  options.frames_per_draw = 1;
  options.credits = 0;
  options.name = NULL;
  options.jack[0] = '\0';
  options.ask_map[0] = '\0';
  options.do_go = 0;
  options.record_file = NULL;
  options.playback_file = NULL;

  use_out_buffer = 1;
  options.conn_type = CONNECT_NETWORK;
#ifdef NET_PLAY
  strcpy(options.host, DEFAULT_HOST);
  options.port = DEFAULT_PORT;
#else
  strcpy(options.host, "bogus.hellofriends.com");
  options.port = 42;
#endif

#ifdef HAS_SOUND
  options.midi_dev = DEV_SEQUENCER;
  options.sound_dev = DEV_DSP;
#endif /* HAS_SOUND */

  fill_default_options(&options);

  for (i = 1; i < argc; i++)
    if (strcmp(argv[i], "-help") == 0) {
      print_help(argv[0]);
      exit(0);
    } else if (strcmp(argv[i], "-debug") == 0) {
      options.debug = 1;
    } else if (strcmp(argv[i], "-credits") == 0) {
      options.credits = 1;
    } else if (strcmp(argv[i], "-rec") == 0) {
      EXPECT_ARG(argv + i);
      options.record_file = argv[++i];
    } else if (strcmp(argv[i], "-replay") == 0) {
      EXPECT_ARG(argv + i);
      options.playback_file = argv[++i];
      options.conn_type = CONNECT_PLAYBACK;
    } else if (strcmp(argv[i], "-name") == 0) {
      EXPECT_ARG(argv + i);
      options.name = argv[++i];
    } else if (strcmp(argv[i], "-jack") == 0) {
      EXPECT_ARG(argv + i);
      strcpy(options.jack, argv[++i]);
    } else if (strcmp(argv[i], "-map") == 0) {
      EXPECT_ARG(argv + i);
      strcpy(options.ask_map, argv[++i]);
    } else if (strcmp(argv[i], "-go") == 0) {
      options.do_go = 1;
    } else if (strcmp(argv[i], "-fskip") == 0) {
      EXPECT_ARG(argv + i);
      options.frames_per_draw = atoi(argv[++i]);
#ifdef NET_PLAY
    } else if (strcmp(argv[i], "-nobuf") == 0) {
      use_out_buffer = 0;
    } else if (strcmp(argv[i], "-host") == 0) {
      EXPECT_ARG(argv + 1);
      strcpy(options.host, argv[++i]);
    } else if (strcmp(argv[i], "-port") == 0) {
      EXPECT_ARG(argv + i);
      options.port = atoi(argv[++i]);
#endif /* NET_PLAY */

#ifdef HAS_SOUND
    } else if (strcmp(argv[i], "-nosound") == 0) {
      options.sound_dev = NULL;
    } else if (strcmp(argv[i], "-sound") == 0) {
      EXPECT_ARG(argv + i);
      options.sound_dev = argv[++i];
    } else if (strcmp(argv[i], "-nomidi") == 0) {
      options.midi_dev = NULL;
    } else if (strcmp(argv[i], "-midi") == 0) {
      EXPECT_ARG(argv + i);
      options.midi_dev = argv[++i];
#endif /* HAS_SOUND */

    } else {
      n_opts = parse_option(&options, argv + i);
      if (n_opts < 0)
        exit(1);
      else if (n_opts == 0) {
	printf("Invalid option: `%s'\n", argv[i]);
	exit(1);
      } else
	i += n_opts - 1;
    }

#ifndef GRAPH_WIN
  /* Ignore SIGPIPE and SIGINT.
   * This must be done before installing the sound */
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, catch_interrupt);
#endif

  select_loser_data_dir(0);
  get_color_correction(color_correction);

  if (do_game_init(&options)) {
    close_graph();
    exit(1);
  }
  while (1) {
    if (do_game_startup(&options))
      break;
    do_game(options.credits);
  }

#ifdef HAS_SOUND
  end_sound();
#endif /* HAS_SOUND */
  close_graph();

  return 0;
}
