
CFLAGS_EXTRA = $(CFLAGS) -fPIC -I../common

LDFLAGS = -shared

ENEMY_NAMES = cannon ghost walker smart meatball slug snail anvil \
              vplatform hplatform pushblock torch dumbenemy

ITEM_NAMES = teleport energy etank water wind invinc powerup \
             shadow lever starlever passage eyes info dumbitem

ALL_ENTITIES = $(ENEMY_NAMES) $(ITEM_NAMES)

OBJS = $(foreach ENT,$(ALL_ENTITIES),$(ENT).o)
LIBS = $(foreach ENT,$(ALL_ENTITIES),lib$(ENT).so)

all: $(LIBS)

clean:
	rm -f *.o *.so *~

lib%.so: %.o
	$(CC) $(LDFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS_EXTRA) -o $@ -c $<
