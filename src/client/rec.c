/* rec.c */

#include <stdio.h>

#include "common.h"

#define REC_MAGIC  0x00434552

static long get_dword_fd(int fd)
{
  char c[4];

  if (read(fd, c, 4) != 4)
    return -1;

  return (c[0] | (c[1] << 8) | (c[2] << 16) | (c[3] << 24));
}

static void put_dword(int s, FILE *f)
{
  fputc(s & 0xff, f);
  fputc((s >> 8) & 0xff, f);
  fputc((s >> 16) & 0xff, f);
  fputc((s >> 24) & 0xff, f);
}


/* Write the header for a recording file */
void write_rec_header(FILE *f)
{
  put_dword(REC_MAGIC, f);
}

/* Read the header for a recording file */
int read_rec_header(int fd)
{
  if (get_dword_fd(fd) != REC_MAGIC)
    return 1;
  return 0;
}
