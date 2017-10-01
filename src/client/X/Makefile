
#CFLAGS_EXTRA = $(CFLAGS) -DUSE_ASM -DHAS_SOUND -DUSE_SHM -DGRAPH_X11 -I../../common
CFLAGS_EXTRA = $(CFLAGS) -DHAS_SOUND -DUSE_SHM -DGRAPH_X11 -I../../common 

OBJS = screen.o

all: $(OBJS)

clean:
	rm -f *.o *~

%.o: %.c
	$(CC) $(CFLAGS_EXTRA) -o $@ -c $<
