
CFLAGS_EXTRA = $(CFLAGS) -DNET_PLAY -DHAS_SOUND -DGRAPH_X11 -I../common -DDATADIR="\".\"" -DLIBDIR="\".\""
LDFLAGS =

OBJS = play.o game.o draw.o client.o startup.o map.o \
       jack.o npc.o sound.o credits.o rec.o commands.o rmsg.o \
       keyboard.o network.o read.o grtext.o key_data.o
X_OBJS = X/screen.o

LIBS = ../common/libcommon.a -lX11 -lXpm -lXext

all: xloser

clean:
	rm -f *.o *~ xloser
	make -C X clean

xloser: $(OBJS) $(X_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(X_OBJS) $(LIBS)

X/screen.o:
	make -C X "CC=$(CC)" "CFLAGS=$(CFLAGS)"

%.o: %.c
	$(CC) $(CFLAGS_EXTRA) -o $@ -c $<

