/* common.c
 *
 * Copyright (C) 1998 Ricardo R. Massaro
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>

#include "common.h"

#define FD_NAME(fd)   (((fd) == CLIENT_FD) ? "CLIENT_FD" : \
		       (((fd) == SERVER_FD) ? "SERVER_FD" : \
			"??"))


#define DO_DEBUG 0       /* 1 to print received and sent messages */


#define MSG_NAME(type)    { MSGT_##type, #type }

#if DO_DEBUG
static struct {
  int type;
  char *name;
} msg_names[] = {
  MSG_NAME(NOP),
  MSG_NAME(STRING),
  MSG_NAME(RETURN),
  MSG_NAME(START),
  MSG_NAME(JOIN),
  MSG_NAME(QUIT),
  MSG_NAME(SERVERINFO),
  MSG_NAME(SETCLIENTTYPE),
  MSG_NAME(SETJACKID),
  MSG_NAME(SETJACKTEAM),
  MSG_NAME(SETJACKNPC),
  MSG_NAME(NPCCREATE),
  MSG_NAME(NPCSTATE),
  MSG_NAME(NPCREMOVE),
  MSG_NAME(JACKSTATE),
  MSG_NAME(NPCINVISIBLE),
  MSG_NAME(NPCINVINCIBLE),
  MSG_NAME(ENEMYHIT),
  MSG_NAME(NJACKS),
  MSG_NAME(TREMOR),
  MSG_NAME(SOUND),
  MSG_NAME(KEYPRESS),
  MSG_NAME(KEYEND),
  { -1, NULL },
};

static char *msg_name(int type)
{
  static char buf[42];
  int i;

  for (i = 0; msg_names[i].name != NULL; i++)
    if (msg_names[i].type == type)
      return msg_names[i].name;
  sprintf(buf, "(type %d)", type);
  return buf;
}
#endif /* DO_DEBUG */


FILE *debug_file;         /* File where to print DEBUG() messages */
FILE *in_record_file;     /* File to record reads */
FILE *out_record_file;    /* File to record writes */
int msg_sent_bytes;       /* # of sent bytes */
int msg_received_bytes;   /* # of received bytes */
int use_out_buffer;       /* 1 to use output buffer */
char loser_data_dir[256];


void select_loser_data_dir(int force_local)
{
#if 0
  if (! force_local) {
    if (access(SYSTEM_DATA_DIR_1, F_OK) == 0) {
      strcpy(loser_data_dir, SYSTEM_DATA_DIR_1);
      return;
    }
    if (access(SYSTEM_DATA_DIR_2, F_OK) == 0) {
      strcpy(loser_data_dir, SYSTEM_DATA_DIR_2);
      return;
    }
  }
#endif
  strcpy(loser_data_dir, LOCAL_DATA_DIR);
}


char *err_msg(void)
{
  return strerror(errno);
}

void DEBUG(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  if (debug_file != NULL)
    vfprintf(debug_file, fmt, ap);
  va_end(ap);
}


/* Returns the size of a message given its type */
static int msg_size(short int type)
{
#define MSG_SIZE(msg)   case MSGT_##msg: return sizeof(MSG_##msg)

  switch (type) {
    MSG_SIZE(NOP);
    MSG_SIZE(STRING);
    MSG_SIZE(RETURN);
    MSG_SIZE(START);
    MSG_SIZE(JOIN);
    MSG_SIZE(QUIT);
    MSG_SIZE(SERVERINFO);
    MSG_SIZE(SETCLIENTTYPE);
    MSG_SIZE(SETJACKID);
    MSG_SIZE(SETJACKTEAM);
    MSG_SIZE(SETJACKNPC);
    MSG_SIZE(NPCCREATE);
    MSG_SIZE(JACKSTATE);
    MSG_SIZE(NPCINVISIBLE);
    MSG_SIZE(NPCINVINCIBLE);
    MSG_SIZE(ENEMYHIT);
    MSG_SIZE(NPCSTATE);
    MSG_SIZE(NPCREMOVE);
    MSG_SIZE(NJACKS);
    MSG_SIZE(TREMOR);
    MSG_SIZE(SOUND);
    MSG_SIZE(KEYPRESS);
    MSG_SIZE(KEYEND);

  default:
    /* This should not happen */
    DEBUG("Invalid or unknown message: %hd\n", type);
    return sizeof(short int);
  }
}


#if ! ((defined GRAPH_DOS) || (defined _WINDOWS))

#define short_from_net(s)   ntohs((s))
#define short_to_net(s)     htons((s))

#else /* ! ((defined GRAPH_DOS) || (defined _WINDOWS)) */

short int short_from_net(short int s)
{
  return ((s & 0xff) << 8) | ((s >> 8) & 0xff);
}

#define short_to_net(s)   short_from_net((s))

#endif /* ! ((defined GRAPH_DOS) || (defined _WINDOWS)) */


/* Put in `buf' the network converted version of `msg' */
void conv_output(short int *buf, short int size, NETMSG *msg)
{
  buf[0] = short_to_net(size);                       /* Size */
  buf[1] = short_to_net(((short int *) msg)[0]);     /* Type */

  if (msg->type == MSGT_STRING) {
    buf[2] = short_to_net(((short int *) msg)[1]);     /* Subtype */
    buf[3] = short_to_net(((short int *) msg)[2]);     /* Data */
    memcpy(buf + 4, ((short int *) msg) + 3, MAX_STRING_LEN);
  } else {
    int i;

    for (i = 1; i < size / sizeof(short int); i++)
      buf[i + 1] = short_to_net(((short int *) msg)[i]);
  }
}

/* Return the host converted of the size of a message */
short int conv_size_in(short int size)
{
  return short_to_net(size);
}

/* Put in `*msg' the message read from `buf' */
void conv_input(NETMSG *msg, short int size, short int *buf)
{
  msg->type = short_from_net(buf[0]);
  if (msg->type == MSGT_STRING) {
    ((short int *) msg)[1] = short_from_net(buf[1]);
    ((short int *) msg)[2] = short_from_net(buf[2]);
    memcpy(((char *) msg) + 3 * sizeof(short int), buf + 3, MAX_STRING_LEN);
  } else {
    int i;

    for (i = 1; i < size / sizeof(short int); i++)
      ((short int *) msg)[i] = short_from_net(buf[i]);
  }
}


/* Network buf queue. Writes move the `end', reads move the `start'. */

typedef struct NET_BUFFER {
  unsigned char *buf;
  int size, start, end;
} NET_BUFFER;

/* Network buffers:
 *   [0] = client -> server
 *   [1] = server -> client
 */
static NET_BUFFER net_buffer[2];


static int nb_read(NET_BUFFER *nb, char *data, int len)
{
  int i;

  if (nb->buf == NULL || nb->size == 0)
    return 0;
  for (i = 0; i < len; i++) {
    if (nb->start == nb->end) {
      printf("Trying to read past end of buffer\n");
      break;
    }
    *data++ = nb->buf[nb->start++];
    nb->start %= nb->size;
  }
  return i;
}

static int nb_write(NET_BUFFER *nb, char *data, int len)
{
  int i;

  if (nb->buf == NULL || nb->size == 0) {
    nb->size = 4096;
    nb->buf = malloc(nb->size);
    if (nb->buf == NULL) {
      errno = ENOMEM;        /* Simulate `out of memory' error */
      return -1;
    }
    nb->start = nb->end = 0;
  }
  for (i = 0; i < len; i++) {
    nb->buf[nb->end++] = *data++;
    nb->end %= nb->size;
    if (nb->end == nb->start) {
      errno = ENOSPC;        /* Simulate `no space left on device' error */
      printf("Out of space: ask the author to increase "
	     "the network emulation buffer!\n");
      return -1;
    }
  }
  return i;
}

static int net_read(int fd, char *data, int len)
{
  NET_BUFFER *nb;

  if (fd == CLIENT_FD)
    nb = &net_buffer[1];
  else if (fd == SERVER_FD)
    nb = &net_buffer[0];
  else {
    int n;

    do
      n = read(fd, data, len);
    while (n < 0 && errno == EINTR);
    return n;
  }

  return nb_read(nb, data, len);
}

static int net_write(int fd, char *data, int len)
{
  NET_BUFFER *nb;

  if (fd == CLIENT_FD)
    nb = &net_buffer[0];
  else if (fd == SERVER_FD)
    nb = &net_buffer[1];
  else {
#ifdef NET_PLAY
    int n;

    do
      n = write(fd, data, len);
    while (n < 0 && errno == EINTR);
    return n;
#else
    errno = EBADF;      /* Simulate a `bad file descriptor */
    return -1;
#endif
  } 

  return nb_write(nb, data, len);
}


/* General buffering functions */

#define NET_BUF_SIZE   1024

static int net_buf_n[1024];
static char *net_buf[1024];

int net_flush(int fd)
{
  if (net_buf_n[fd] > 0 && net_buf[fd] != NULL) {
    int ret;

    ret = net_write(fd, net_buf[fd], net_buf_n[fd]);
    net_buf_n[fd] = 0;
    return ret;
  }
  return 0;
}

static int net_write_buf(int fd, char *data, int n)
{
  if (net_buf[fd] == NULL) {
    net_buf[fd] = (char *) malloc(NET_BUF_SIZE);
    net_buf_n[fd] = 0;
    if (net_buf[fd] == NULL)
      return net_write(fd, data, n);   /* Out of memory, write directly */
    memset(net_buf[fd], 1, NET_BUF_SIZE);
  }

  if (net_buf_n[fd] + n > NET_BUF_SIZE)
    net_flush(fd);

  /* Doesn't fit on buffer: write directly */
  if (net_buf_n[fd] + n > NET_BUF_SIZE)
    return net_write(fd, data, n);

  memcpy(net_buf[fd] + net_buf_n[fd], data, n);
  net_buf_n[fd] += n;
  return n;
}



/********************************************************************/
/** High level network functions ************************************/
/********************************************************************/

int has_message(int fd)
{
  NET_BUFFER *nb;

  if (fd == CLIENT_FD)
    nb = &net_buffer[1];
  else if (fd == SERVER_FD)
    nb = &net_buffer[0];
  else {
#ifdef NET_PLAY
    fd_set set;
    struct timeval tv;

    if (fd < 0)
      return -1;

    FD_ZERO(&set);
    FD_SET(fd, &set);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    while (select(fd + 1, &set, NULL, NULL, &tv) < 0)
      if (errno != EINTR) {
	DEBUG("select(): %s\n", err_msg());
	return -1;
      }

    if (FD_ISSET(fd, &set))
      return 1;

    return 0;
#else
    errno = EBADF;      /* Simulate a `bad file descriptor */
    return -1;
#endif
  }

  return (nb->end != nb->start);
}


/* Read a message. Return -1 if an error ocurred, 0 if the message was read. */
int read_message(int fd, NETMSG *msg)
{
  short int size, buf[sizeof(NETMSG) / sizeof(short int)];
  int i;

  if (fd < 0)
    return -1;       /* Invalid file descriptor */
  i = net_read(fd, (char *) &size, sizeof(short int));
  if (i <= 0) {
    if (i < 0)
      DEBUG("read(): %s\n", err_msg());
    return -1;       /* Read error or connection closed */
  }
  msg_received_bytes += sizeof(short int);
#ifndef NET_PLAY
  if (fd != SERVER_FD)
#endif
  if (in_record_file != NULL)
    fwrite(&size, 1, sizeof(short int), in_record_file);
  size = conv_size_in(size);
  if (size <= 0 || size > sizeof(NETMSG)) {
    DEBUG("Invalid message size: %d\n", size);
    return -1;       /* Invalid message size */
  }

  net_read(fd, (char *) buf, size);
#ifndef NET_PLAY
  if (fd != SERVER_FD)
#endif
  if (in_record_file != NULL)
    fwrite(buf, 1, size, in_record_file);
  msg_received_bytes += sizeof(size);
  conv_input(msg, size, buf);

#if DO_DEBUG
  printf("read_message(): %s read `%s' (%d bytes)\n",
	 FD_NAME(fd), msg_name(msg->type), size);
#endif

  return 0;
}

/* Send a message, return 1 on error */
int send_message(int fd, NETMSG *msg, int use_buffer)
{
  char buf[sizeof(NETMSG) + sizeof(short int)];
  int size;

  if (fd < 0)
    return 1;

  size = msg_size(msg->type);
  conv_output((short int *) buf, size, msg);
  if (out_record_file != NULL)
    fwrite(buf, 1, size + sizeof(short int), out_record_file);

  if (use_buffer && use_out_buffer)
    net_write_buf(fd, buf, size + sizeof(short int));
  else {
    net_flush(fd);
    if (net_write(fd, buf,
		  size + sizeof(short int)) != size + sizeof(short int))
      return -1;
  }
  msg_sent_bytes += size + sizeof(short int);

#if DO_DEBUG
  printf("send_message(): %s sending `%s' (%d bytes)\n",
	 FD_NAME(fd), msg_name(msg->type), size);
#endif

  return 0;
}
