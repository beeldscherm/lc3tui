
lc3tui: main.c lc3/lc3_sim.c lc3/lc3_tui.c lc3/lc3_cmd.c
	gcc -std=c99 -o $@ $^ -Wall -pedantic -lcurses
