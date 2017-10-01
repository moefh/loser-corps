
CFLAGS_EXTRA = $(CFLAGS) -DNET_PLAY -DHAS_SOUND -DDATADIR="\".\"" -DLIBDIR="\".\""

OBJS = common.o gz_open.o npc_data.o parse.o snd_data.o spr_data.o bitmap.o pal8bpp.o

AR = ar
ARFLAGS = rs

all: libcommon.a

clean:
	rm -f *~ *.o libcommon.a

libcommon.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS_EXTRA) -o $@ -c $<


