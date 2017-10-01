
CC = gcc

CFLAGS = -Wall -O2

LDFLAGS =

OBJS = window.o dialog.o win_io.o stdctrl.o stddlg.o bgiclone.o

AR = ar
ARFLAGS = rs

all: libxdialogs.a

clean:
	rm -f *~ *.o libxdialogs.a

libxdialogs.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<
