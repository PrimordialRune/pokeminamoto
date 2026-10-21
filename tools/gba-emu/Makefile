# Headless mGBA host for agents. Needs libmgba-dev (mGBA >= 0.10).
CC ?= cc
CFLAGS ?= -O2 -Wall

gbaemu: gbaemu.c
	$(CC) $(CFLAGS) -o $@ $< -lmgba

clean:
	rm -f gbaemu

.PHONY: clean
