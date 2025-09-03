
CFLAGS=-std=c99 -Wall -pedantic
LC3CFILES=lc3/lc3_cmd.c lc3/lc3_sim.c lc3/lc3_tui.c

lc3tui: main.c lc3/config.h $(LC3CFILES) lc3/lib/cmdarg/cmdarg.o lc3/lib/leakcheck/lc.o
	$(CC) $(CFLAGS) -o $@ $^ -lcurses

lc3/lib/cmdarg/cmdarg.o: lc3/config.h lc3/lib/cmdarg/cmdarg.c
	$(CC) $(CFLAGS) -c -o $@ lc3/lib/cmdarg/cmdarg.c

lc3/lib/leakcheck/lc.o: lc3/config.h lc3/lib/leakcheck/lc.c
	$(CC) $(CFLAGS) -c -o $@ lc3/lib/leakcheck/lc.c

clean:
	rm lc3tui
