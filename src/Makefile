
.PHONY: all common xdialogs clean

all: xloser loser-s loser-map npcs

clean:
	make -C client clean
	make -C server clean
	make -C common clean
	make -C xdialogs clean
	make -C mapedit clean
	make -C npcs clean

xloser: common
	make -C client "CC=$(CC)" "CFLAGS=$(CFLAGS)"

loser-map: xdialogs
	make -C mapedit "CC=$(CC)" "CFLAGS=$(CFLAGS)"

loser-s:
	make -C server "CC=$(CC)" "CFLAGS=$(CFLAGS)"

npcs: common
	make -C npcs "CC=$(CC)" "CFLAGS=$(CFLAGS)"

common:
	make -C common "CC=$(CC)" "CFLAGS=$(CFLAGS)"

xdialogs:
	make -C xdialogs "CC=$(CC)" "CFLAGS=$(CFLAGS)"
