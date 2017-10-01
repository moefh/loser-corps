
CFLAGS_EXTRA = $(CFLAGS) -I../common -DNET_PLAY -DHAS_SOUND -DLIBDIR="\".\""

LDFLAGS = -ldl -rdynamic -s

OBJS = main.o network.o server.o connect.o send.o receive.o calc.o jack.o weapon.o \
       enemy.o item.o event.o sound.o map.o movement.o addon.o lowlib.o

LIBS = ../common/libcommon.a -ldl

all: loser-s

clean:
	rm -f *.o *~ loser-s

loser-s: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS_EXTRA) -o $@ -c $<
