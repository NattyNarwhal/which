CC := cc
CFLAGS := -std=c99 -O2 -g -Wall -pedantic

.PHONY: clean

which: which.c
	$(CC) -o which $(CFLAGS) which.c

clean:
	rm -f which
