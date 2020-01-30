/* rmsg.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


static char *def_msg[] = {
  "Die! Die! Die!",
  "AAAARGH!",
  "Help!",
  "Follow me.",
  "Let's go!",
  "Truce!",
  "Do not underestimate the enemy!",
  "Surprise!",
  "Now I will teach you!",
  "Ha, ha, ha!!!",
  "Come to the dark side.",
  "Better luck next time... :-)",
};


int read_messages(char *name, char **msg)
{
  int i, j, num;
  FILE *f = NULL;
  char filename[256], str[1024];
  char *home;

  home = getenv("HOME");
  if (home != NULL) {
    snprintf(filename, sizeof(filename), "%s/.%s", home, name);
    f = fopen(filename, "r");
  }

  if (f == NULL) {
    snprintf(filename, sizeof(filename), "data/%s", name);
    f = fopen(filename, "r");
  }

  if (f == NULL)
    for (i = 0; i < 12; i++)
      msg[i] = strdup(def_msg[i]);
  else {
    num = 0;
    while (fgets(str, 1024, f) != NULL && num < 12) {
      for (i = 0; isspace(str[i]); i++)
	;
      if (str[i] == '#' || str[i] == '\0')
	continue;

      for (j = strlen(str) - 1; j > i && isspace(str[j]); j--)
	str[j] = '\0';
      
      msg[num++] = strdup(str + i);
    }
    fclose(f);

    for (i = num; i < 12; i++)
      msg[i] = strdup(def_msg[i]);
  }

  for (i = 0; i < 12; i++)
    if (msg[i] == NULL)
      return 1;

  return 0;
}
