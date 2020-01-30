/* config.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "common.h"


/* Fill in the `options' struct with the default options */
void fill_default_options(OPTIONS *options)
{
  strcpy(options->data_dir, DEFAULT_DATA_DIR);
  strcpy(options->map_file, DEFAULT_MAP_FILE);
  strcpy(options->block_bmp_file, DEFAULT_BLOCK_BMP_FILE);
  strcpy(options->back_bmp_file, DEFAULT_BACK_BMP_FILE);
  strcpy(options->font, DEFAULT_FONT);
#ifdef USE_SHM
  options->use_shm = DEFAULT_USE_SHM;
#endif /* USE_SHM */
}


/* Try to open the file `filename' in the current directory, then in
 * one of the directories on the `search_path'. An empty path in
 * `search_path' means "$HOME/.".  If everything fails, returns NULL.
 * If succeeds, copy the opened file name to `output' and return the
 * opened FILE. */
static FILE *look_for_file(char *file, char **search_path, char *output)
{
  FILE *f;
  char *home, filename[1024];
  int i;

  if ((f = fopen(file, "r")) != NULL) {
    strcpy(output, file);
    return f;
  }

  for (i = 0; search_path[i] != NULL; i++) {
    if (search_path[i][0] == '\0') {
      if ((home = getenv("HOME")) == NULL)
	continue;
      snprintf(filename, sizeof(filename), "%s/.", home);
    } else
      snprintf(filename, sizeof(filename), "%s/", search_path[i]);

    strcat(filename, file);

#if PRINT_DEBUG
    printf("trying `%s'\n", filename);
#endif

    if ((f = fopen(filename, "r")) != NULL) {
#if PRINT_DEBUG
      printf("found config file `%s'\n", filename);
#endif
      strcpy(output, filename);
      return f;
    }
  }
#if PRINT_DEBUG
  printf("config file not found\n");
#endif
  return NULL;
}

/* Read the configuration from the file `filename' */
int read_options(char *file, OPTIONS *options)
{
  FILE *f;
  int line_no, i;
  char filename[1024];
  char line[1024], *str;
  char option[256], value[256];
  static char *search_path[] = {
    "",
    LOCAL_DATA_DIR,
    NULL
  };


  f = look_for_file(file, search_path, filename);
  if (f == NULL)
    return 1;

  line_no = 0;
  while (fgets(line, 1024, f)) {
    line_no++;
    for (str = line + strlen(line) - 1; str >= line && isspace(*str); str--)
      ;
    if (str < line)
      continue;       /* Empty line */
    *(str + 1) = '\0';

    for (str = line; isspace(*str); str++)
      ;
    if (*str == '#')
      continue;       /* Comment */

    i = 0;
    while (*str != '=' && *str != '\0' && ! isspace(*str))
      option[i++] = *str++;
    option[i] = '\0';

    while (isspace(*str))
      str++;
    if (*str != '=') {
      printf("%s:%d: ignoring option without `='\n", filename, line_no);
      continue;
    }
    str++;
    while (isspace(*str))
      str++;
    i = 0;
    while (*str != '\0' && ! isspace(*str))
      value[i++] = *str++;
    value[i] = '\0';

    if (strcmp(option, "data_dir") == 0)
      strcpy(options->data_dir, value);
    else if (strcmp(option, "map") == 0)
      strcpy(options->map_file, value);
    else if (strcmp(option, "blocks") == 0)
      strcpy(options->block_bmp_file, value);
    else if (strcmp(option, "back") == 0)
      strcpy(options->back_bmp_file, value);
    else if (strcmp(option, "font") == 0)
      strcpy(options->font, value);
    else if (strcmp(option, "shm") == 0)
#ifdef USE_SHM
      options->use_shm = atoi(value);
#else
      ;
#endif
    else
      printf("%s:%d: ignoring unknown option `%s'\n",
	     filename, line_no, option);
  }
  fclose(f);
  return 0;
}
