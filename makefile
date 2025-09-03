
lc3tui: main.c lc3/lc3.h lc3/lc3_sim.c lc3/lc3_tui.c lc3/lc3_cmd.c lc3/lc3_sim.h lc3/lc3_tui.h lc3/lc3_cmd.h lc3/lib/lc.c lc3/lib/config.h
	gcc -std=c99 -o $@ $^ -Wall -pedantic -lcurses -g
