/* read.c */

#include <stdio.h>
#include <malloc.h>

#include "sndsrv.h"


/* Read a word (2 butes) from a file using Intel byte ordering */
static int get_w(FILE *f)
{
  int i;

  i = getc(f);
  return ((getc(f) & 0xff) << 8) | (i & 0xff);
}

/* Read a double word (4 butes) from a file using Intel byte ordering */
static int get_l(FILE *f)
{
  int i, val = 0;

  for (i = 0; i < 4; i++)
    val |= ((getc(f) & 0xff) << (i * 8));
  return val;
}

/* Load a sample from a wave (`.wav') file */
SAMPLE *load_wav(char *filename)
{
  FILE *f;
  char buffer[16];
  int i, n_bytes, len, freq = 22050, bits = 8;
  SAMPLE *spl = NULL;

  if ((f = fopen(filename, "rb")) == NULL)
    return NULL;

  /* Check the header */
  fread(buffer, 1, 12, f);
  if (memcmp(buffer, "RIFF", 4) != 0 || memcmp(buffer + 8, "WAVE", 4) != 0) {
    fclose(f);
    return NULL;
  }

  while (! feof(f)) {
    if (fread(buffer, 1, 4, f) != 4)
      break;

    n_bytes = get_l(f);

    if (memcmp(buffer, "fmt ", 4) == 0) {
      i = get_w(f);
      n_bytes -= 2;
      if (i != 1)                /* Not PCM data */
	goto get_out;

      i = get_w(f);
      n_bytes -= 2;
      if (i != 1)                /* Not MONO */
	goto get_out;

      freq = get_l(f);           /* Read frequency */
      n_bytes -= 4;

      get_l(f);                  /* Skip 6 bytes */
      get_w(f);
      n_bytes -= 6;

      bits = get_w(f);
      n_bytes -= 2;
      if (bits != 8 && bits != 16)
	goto get_out;
    } else if (memcmp(buffer, "data", 4) == 0) {
      len = n_bytes;
      if (bits == 16)
	len /= 2;

      spl = (SAMPLE *) malloc(sizeof(SAMPLE));
      if (spl != NULL) {
	spl->bits = bits;
	spl->freq = freq;
	spl->len = len;
	spl->data = (unsigned char *) malloc(n_bytes);
	if (spl->data == NULL) {
	  free(spl);
	  spl = NULL;
	} else {
	  int error = 0;

	  if (bits == 8) {
	    if (fread(spl->data, 1, len, f) != len)
	      error = 1;
	  } else {
	    int c;

	    for (i = 0; i < len; i++) {
	      if ((c = get_w(f)) == EOF) {
		error = 1;
		break;
	      }
	      ((signed short *) spl->data)[i] = c;
	    }
	  }
	  if (error) {
	    free(spl->data);
	    free(spl);
	    spl = NULL;
	  }
	}
      }
    }
    while (n_bytes > 0) {
      if (getc(f) == EOF)
	break;
      n_bytes--;
    }
  }

get_out:
  fclose(f);
  return spl;
}

void destroy_sample(SAMPLE *s)
{
  if (s->data != NULL)
    free(s->data);
  free(s);
}

