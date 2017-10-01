
.PHONY: all build local-install clean

CC = gcc
CFLAGS = -O2 -Wall

all: local-install

clean:
	make -C src clean
	rm -f xloser loser-s loser-map libs/npcs/*.so *~

build:
	make -C src "CC=$(CC)" "CFLAGS=$(CFLAGS)"

local-install: build
	cp -f src/client/xloser .
	cp -f src/server/loser-s .
	cp -f src/mapedit/loser-map .
	cp -f src/npcs/*.so libs/npcs/
