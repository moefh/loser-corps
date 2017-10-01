/* event.h */

#ifndef EVENT_H
#define EVENT_H

typedef struct EVENT EVENT;

#define EVENT_DATA_SIZE    5   /* # of data slots to an event */

struct EVENT {
  int id;                  /* Event ID */
  int active;              /* 1 if active */
  int frames_left;         /* Frames until the event happens */
  int frames;              /* # of frames of repetition (-1 means one-shot) */
  void (*func)(SERVER *server, EVENT *);
  union {
    void *p;
    int i;
  } data[EVENT_DATA_SIZE]; /* Data to pass to the function */
};

EVENT *srv_allocate_event(SERVER *server);
void srv_free_event(SERVER *server, EVENT *e);
void srv_process_events(SERVER *server);

#endif
