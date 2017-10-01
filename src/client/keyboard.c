/* keyboard.c */

#include <stdio.h>

#include "game.h"

#if ! ((defined GRAPH_DOS) || (defined GRAPH_WIN))
#define DO_USLEEP(n)  usleep(n)
#else
#define DO_USLEEP(n)
#endif /* GRAPH_DOS */

static int keyboard_caps_lock = 0;  /* caps lock state */

typedef struct keyitem {
  char normal, shifted;      /* Normal and shifted characters */
  int do_caps;               /* 1 if capslock inverts shift */
} keyitem;

/* Keyboard map (scancode->ascii) */
keyitem keymap[] = {
  { 0, 0, 0 },
  { 27, 27, 0 },
  { '1', '!', 0 },
  { '2', '@', 0 },
  { '3', '#', 0 },
  { '4', '$', 0 },
  { '5', '%', 0 },
  { '6', '^', 0 },
  { '7', '&', 0 },
  { '8', '*', 0 },
  { '9', '(', 0 },
  { '0', ')', 0 },
  { '-', '_', 0 },
  { '=', '+', 0 },
  { 8, 8, 0 },
  { 9, 9, 0 },
  { 'q', 'Q', 1 },
  { 'w', 'W', 1 },
  { 'e', 'E', 1 },
  { 'r', 'R', 1 },
  { 't', 'T', 1 },
  { 'y', 'Y', 1 },
  { 'u', 'U', 1 },
  { 'i', 'I', 1 },
  { 'o', 'O', 1 },
  { 'p', 'P', 1 },
  { '[', '{', 0 },
  { ']', '}', 0 },
  { 13, 13, 0 },
  { 0, 0, 0 },
  { 'a', 'A', 1 },
  { 's', 'S', 1 },
  { 'd', 'D', 1 },
  { 'f', 'F', 1 },
  { 'g', 'G', 1 },
  { 'h', 'H', 1 },
  { 'j', 'J', 1 },
  { 'k', 'K', 1 },
  { 'l', 'L', 1 },
  { ';', ':', 0 },
  { '\'', '\"', 0 },
  { '`', '~', 0 },
  { 0, 0, 0 },
  { '\\', '|', 0 },
  { 'z', 'Z', 1 },
  { 'x', 'X', 1 },
  { 'c', 'C', 1 },
  { 'v', 'V', 1 },
  { 'b', 'B', 1 },
  { 'n', 'N', 1 },
  { 'm', 'M', 1 },
  { ',', '<', 0 },
  { '.', '>', 0 },
  { '/', '?', 0 },
  { 0, 0, 0 },
  { '*', '*', 0 },
  { 0, 0, 0 },
  { ' ', ' ', 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { '-', '-', 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { '+', '+', 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { '/', '/', 0 },   /* 98 */
  { 0, 0, 0 },
  { 0, 0, 0 },

  { 0, 0, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { -1, -1, 0 },
  { 8, 8, 0 },     /* 111 */
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
  { 0, 0, 0 },
};


/* Translate a scancode to a character knowing the state
 * of the shift keys (1 if left or right is pressed)
 * and the capslock state (1 if on). If the scancode produces
 * a character, it will be stored in *c (provided that c is
 * not NULL) and this function will return 1; otherwise, 0
 * will be returned.
 */
static int translate_key(int code, int shift_state, int capslock_state, int *c)
{
  if (code < 0 || code >= sizeof(keymap) / sizeof(keymap[0]))
    return 0;

  if (keymap[code].normal == 0)
    return 0;

  if (c != NULL) {
    if (shift_state)
      if (capslock_state && keymap[code].do_caps)
	*c = keymap[code].normal;
      else
	*c = keymap[code].shifted;
    else
      if (keyboard_caps_lock && keymap[code].do_caps)
	*c = keymap[code].shifted;
      else
	*c = keymap[code].normal;
  }
  return 1;
}



int game_kb_has_key(void)
{
  int i;

  DO_USLEEP(10000);

  for (i = 0; i < 127; i++)
    if (game_key_trans[i] == 1
	&& translate_key(i,
			 game_key[SCANCODE_LEFTSHIFT]
			 || game_key[SCANCODE_RIGHTSHIFT],
			 keyboard_caps_lock, NULL))
      return 1;
  return 0;
}

int game_kb_get_key(void)
{
  int i, ch;

  for (i = 0; i < 127; i++)
    if (game_key_trans[i] == 1) {
      game_key_trans[i] = 0;
      if (translate_key(i,
			game_key[SCANCODE_LEFTSHIFT]
			|| game_key[SCANCODE_RIGHTSHIFT],
			keyboard_caps_lock, &ch))
	return (ch & 0xff) | ((i << 8) & 0xff00);
    }
  return 0;
}
