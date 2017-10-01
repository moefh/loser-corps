
CFLAGS_EXTRA = $(CFLAGS) -DDATADIR="\".\"" -I../xdialogs -I../common

LDFLAGS =

OBJS = mapedit.o misc.o dialogs.o procs.o map.o draw.o config.o

LIBS = ../xdialogs/libxdialogs.a ../common/libcommon.a -lXext -lXpm -lX11


all: loser-map

clean:
	rm -f *.o *~ loser-map

loser-map: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS_EXTRA) -o $@ -c $<
