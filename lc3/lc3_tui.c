#include "lc3_tui.h"
#include "lc3_cmd.h"
#include <unistd.h>
#include <sys/time.h>

#define CMD_LEN_MAX (256)
#define RUNSTEP_US  (100000)


static bool initialized = false;


String copyString(String str) {
    String ret = {
        .ptr = malloc(str.cap),
        .sz  = str.sz,
        .cap = str.cap,
    };

    memcpy(ret.ptr, str.ptr, ret.sz + 1);
    return ret;
}


void stringInsert(String *str, char c, int idx) {
    if (idx == str->sz) {
        addchar(str, c);
        return;
    }

    if (++str->sz >= str->cap) {
        str->cap *= 2;
        str->ptr = realloc(str->ptr, str->cap * sizeof(char));
    }

    memmove(str->ptr + idx + 1, str->ptr + idx, str->sz - idx);
    str->ptr[idx] = c;
}


void stringDelete(String *str, int idx) {
    if (idx >= str->sz || idx < 0) {
        return;
    }

    str->sz--;
    memmove(str->ptr + idx, str->ptr + idx + 1, str->sz - idx + 1);
}


LC3_TermInterface LC3_CreateTermInterface(LC3_SimInstance *sim) {
    if (!initialized) {
        initscr();
        initialized = true;
    }

    LC3_TermInterface ret = {
        .sim = sim,
        .cols = 0,
        .rows = 0,
        .memViewStart = sim->pc,
        .memView = NULL,
        .regView = NULL,
        .inView  = NULL,
        .outView = NULL,
        .inViewBox  = NULL,
        .outViewBox = NULL,
        .commands = newStringArray(),
        .delay = 100,
        .numDisplay = LC3_NDISPLAY_HEX,
        .running = false,
        .cmdError = NULL,
    };

    keypad(stdscr, true);
    halfdelay(ret.delay);
    noecho();
    use_default_colors();
    start_color();
    curs_set(0);
    set_escdelay(0);

    init_pair(1, COLOR_WHITE, COLOR_RED);
    init_pair(2, COLOR_WHITE, COLOR_BLUE);
    init_pair(3, COLOR_WHITE, COLOR_GREEN);
    init_pair(4, COLOR_BLACK, COLOR_WHITE);

    return ret;
}


static WINDOW *createWindow(int x, int y, int width, int height) {
    WINDOW *ret = newwin(height, width, y, x);
    return ret;
}


static void destroyWindow(WINDOW *win) {
    if (win == NULL) {
        return;
    }

    delwin(win);
}


void LC3_DestroyTermInterface(LC3_TermInterface tui) {
    destroyWindow(tui.memView);
    destroyWindow(tui.regView);
    destroyWindow(tui.inView);
    destroyWindow(tui.outView);
    destroyWindow(tui.inViewBox);
    destroyWindow(tui.outViewBox);
    freeStringArray(tui.commands);
    endwin();
}


#define BOX_PAD (2)

#define REG_VIEW_W() (24)
#define REG_VIEW_H() (10)

#define OUT_VIEW_W() ((tui->cols) - (3 * BOX_PAD))
#define OUT_VIEW_H() ((tui->rows / 5))

#define IN_VIEW_W() (REG_VIEW_W())
#define IN_VIEW_H() (tui->rows - OUT_VIEW_H() - REG_VIEW_H() - 4 * BOX_PAD)

#define MEM_VIEW_W() (tui->cols - REG_VIEW_W() - (5 * BOX_PAD))
#define MEM_VIEW_H() (tui->rows - OUT_VIEW_H() - 3 * BOX_PAD)


bool LC3_IsAddrDisplayed(LC3_TermInterface *tui, uint16_t addr) {
    return (addr >= tui->memViewStart) && (addr <= tui->memViewStart + MEM_VIEW_H());
}


static void resizeCheck(LC3_TermInterface *tui) {
    int cols = getmaxx(stdscr);
    int rows = getmaxy(stdscr);

    if (((tui->cols == cols) && (tui->rows == rows)) || (cols < 53 || rows < 23)) {
        return;
    }

    tui->cols = cols;
    tui->rows = rows;
    clear();

    // Destroy all the old windows
    destroyWindow(tui->memView);
    destroyWindow(tui->regView);
    destroyWindow(tui->inView);
    destroyWindow(tui->outView);
    destroyWindow(tui->inViewBox);
    destroyWindow(tui->outViewBox);

    // Create new ones
    tui->memView = createWindow(
        2,
        1,
        MEM_VIEW_W() + BOX_PAD,
        MEM_VIEW_H() + BOX_PAD
    );

    tui->regView = createWindow(
        cols - REG_VIEW_W() - (2 * BOX_PAD),
        1,
        REG_VIEW_W() + BOX_PAD,
        REG_VIEW_H() + BOX_PAD
    );

    tui->inView = createWindow(
        cols - REG_VIEW_W() - BOX_PAD - 1,
        REG_VIEW_H() + (BOX_PAD * 2),
        IN_VIEW_W(),
        IN_VIEW_H()
    );

    tui->outView = createWindow(
        3,
        MEM_VIEW_H() + (2 * BOX_PAD), 
        OUT_VIEW_W(),
        OUT_VIEW_H()
    );

    tui->inViewBox = createWindow(
        cols - REG_VIEW_W() - (2 * BOX_PAD),
        REG_VIEW_H() + (BOX_PAD * 2) - 1,
        IN_VIEW_W() + BOX_PAD,
        IN_VIEW_H() + BOX_PAD
    );

    tui->outViewBox = createWindow(
        2,
        MEM_VIEW_H() + (2 * BOX_PAD) - 1, 
        OUT_VIEW_W() + BOX_PAD,
        OUT_VIEW_H() + BOX_PAD
    );
}


int clamp(int n, int min, int max) {
    return (n > max) ? max : ((n < min) ? min : n);
}


static String getCommand(LC3_TermInterface *tui) {
    String cmd = newString();

    cbreak();
    curs_set(1);

    int c, input = true, hist = tui->commands.sz, idx = 0;

    while (input) {
        mvaddch(tui->rows - 1, 0, ':');
        clrtoeol();
        addstr(cmd.ptr);
        move(tui->rows - 1, idx + 1);

        switch ((c = getch())) {
            case '\n':          input = false;
                                continue;
            case KEY_BACKSPACE: stringDelete(&cmd, idx - 1);
                                idx = clamp(idx - 1, 0, cmd.sz);
                                break;
            case KEY_UP:        if (tui->commands.sz == 0) {
                                    break;
                                }
                                free(cmd.ptr);
                                hist = (hist > 0) ? (hist - 1) : 0;
                                cmd = copyString(tui->commands.ptr[hist]);
                                idx = cmd.sz;
                                break;
            case KEY_DOWN:      free(cmd.ptr);
                                hist = (hist < tui->commands.sz) ? (hist + 1) : hist;
                                cmd = (hist == tui->commands.sz) ? newString() : copyString(tui->commands.ptr[hist]);
                                idx = cmd.sz;
                                break;
            case KEY_LEFT:      idx = clamp(idx - 1, 0, cmd.sz);
                                break;
            case KEY_RIGHT:     idx = clamp(idx + 1, 0, cmd.sz);
                                break;
            default:            stringInsert(&cmd, c, idx);
                                idx++;
                                break;
        }
    }

    if (cmd.sz != 0) {
        addString(&tui->commands, cmd);
    } else {
        free(cmd.ptr);
        cmd.ptr = NULL;
    }

    curs_set(0);
    return cmd;
}


int loopAround(int i, int mod) {
    if (i < 0) {
        i += mod * ((i / mod) + 1);
    }

    return (i % mod);
}


static void handleInput(LC3_TermInterface *tui) {
    int ch;
    halfdelay(tui->delay);
    String cmd;

    if ((ch = getch()) != ERR) {
        switch (ch) {
            case KEY_DOWN:  tui->memViewStart = loopAround(tui->memViewStart + 1, LC3_MEM_SIZE); 
                            break;
            case KEY_UP:    tui->memViewStart = loopAround(tui->memViewStart - 1, LC3_MEM_SIZE);
                            break;
            case 27:        tui->running = false;
                            break;
            case ':':       cmd = getCommand(tui);
                            LC3_ExecuteCommand(tui, cmd.ptr);
                            if (tui->cmdError != NULL) {
                                attron(COLOR_PAIR(1));
                                move(tui->rows - 1, 0);
                                clrtoeol();
                                addstr(tui->cmdError);
                                attroff(COLOR_PAIR(1));
                            }
                            halfdelay(tui->delay);
                            break;
            case '\n':      tui->sim->flags &= ~LC3_SIM_HALTED;
                            LC3_ExecuteInstruction(tui->sim);
                            tui->sim->flags |= LC3_SIM_HALTED;
                            if (!LC3_IsAddrDisplayed(tui, tui->sim->pc)) {
                                tui->memViewStart = tui->sim->pc;
                            }
                            break;
            case 'u':       LC3_ExecuteCommand(tui, "u");
        }
    }
}


#define CC_CHAR(n, t, f) (((sim->cc & n) != 0) ? t : f)
#define FMT_STR_LEN (20)


static void displaySimulator(LC3_TermInterface *tui) {
    LC3_SimInstance *sim = tui->sim;

    resizeCheck(tui);

    const char *formatStrings[LC3_NDISPLAY_MAX];
    formatStrings[LC3_NDISPLAY_HEX]  = "%cx%04X | 0x%04X | ";
    formatStrings[LC3_NDISPLAY_UINT] = "%cx%04X | %6hu | ";
    formatStrings[LC3_NDISPLAY_INT]  = "%cx%04X | %6hi | ";

    // Draw memory view
    box(tui->memView, 0, 0);

    for (int i = tui->memViewStart, y = 1; y <= MEM_VIEW_H() ; i = loopAround(i + 1, LC3_MEM_SIZE), y++) {
        int max = MEM_VIEW_W() - FMT_STR_LEN;

        if (sim->memory[i].breakpoint) {
            wattron(tui->memView, COLOR_PAIR(1));
        }

        if (i == sim->pc) {
            wattron(tui->memView, COLOR_PAIR(2));
        }

        mvwprintw(
            tui->memView, y, 2, formatStrings[tui->numDisplay], (i == sim->pc) ? '>' : ' ' , 
            (uint16_t)i, (tui->numDisplay == LC3_NDISPLAY_INT) ? sim->memory[i].value : (uint16_t)sim->memory[i].value
        );

        for (int i = 0; i < max; waddch(tui->memView, ' '), i++);
        ;

        if (sim->memory[i].hasDebug) {
            wmove(tui->memView, y, FMT_STR_LEN);
            String debug = sim->debug.ptr[sim->memory[i].debugIndex];

            if (debug.sz <= max) {
                wprintw(tui->memView, "%s", debug.ptr);
            } else {
                char tmp = debug.ptr[max - 2];
                debug.ptr[max - 2] = '\0';
                wprintw(tui->memView, "%s..", debug.ptr);
                debug.ptr[max - 2] = tmp;
            }
        }

        wattroff(tui->memView, COLOR_PAIR(1));
        wattroff(tui->memView, COLOR_PAIR(2));
    }

    // Draw register view
    const char *regStrings[LC3_NDISPLAY_MAX];
    regStrings[LC3_NDISPLAY_HEX]  = " R%1i | 0x%04X";
    regStrings[LC3_NDISPLAY_UINT] = " R%1i | %6hu";
    regStrings[LC3_NDISPLAY_INT]  = " R%1i | %6hi";

    box(tui->regView, 0, 0);

    for (int i = 0; i < 8; i++) {
        mvwprintw(
            tui->regView, i + 1, 1, regStrings[tui->numDisplay], i,
            (tui->numDisplay == LC3_NDISPLAY_INT) ? sim->regs[i] : (uint16_t)sim->regs[i]
        );
    }

    mvwprintw(
        tui->regView,  9, 1, " CC | %c%i%c%i%c%i",
        CC_CHAR(0x4, 'N', '.'), CC_CHAR(0x4, 1, 0),     CC_CHAR(0x2, 'Z', '.'),
        CC_CHAR(0x2, 1, 0),     CC_CHAR(0x1, 'P', '.'), CC_CHAR(0x1, 1, 0)
    );
    mvwprintw(tui->regView, 10, 1, " PC | 0x%04X", (uint16_t)sim->pc);

    // Draw input view
    werase(tui->inView);
    box(tui->inViewBox, 0, 0);
    wattron(tui->inView, COLOR_PAIR(4));
    wmove(tui->inView, 0, 0);

    for (int i = 0; i < VQ_SZ(sim->inputs); i++) {
        waddstr(tui->inView, charString(VQ_EL(sim->inputs, i)));
    }

    wattroff(tui->inView, COLOR_PAIR(4));

    // Draw output view
    box(tui->outViewBox, 0, 0);
    int i = sim->output.sz - (OUT_VIEW_W() * OUT_VIEW_H());
    wmove(tui->outView, 0, 0);

    for (i = (i < 0) ? 0 : i; i < sim->output.sz; i++) {
        waddch(tui->outView, sim->output.ptr[i]);
    }

    // State indicator
    if (sim->flags & LC3_SIM_HALTED ) {
        attron(COLOR_PAIR(1));
        mvaddch(tui->rows - 1, tui->cols - 1, 'H');
        attroff(COLOR_PAIR(1));
    } else {
        attron(COLOR_PAIR(3));
        mvaddch(tui->rows - 1, tui->cols - 1, 'R');
        attroff(COLOR_PAIR(3));
    }

    refresh();
    wrefresh(tui->memView);
    wrefresh(tui->regView);
    wrefresh(tui->inViewBox);
    wrefresh(tui->inView);
    wrefresh(tui->outViewBox);
    wrefresh(tui->outView);
}


int LC3_RunTermInterface(LC3_TermInterface *tui) {
    if (!initialized) {
        return 1;
    }

    tui->running = true;

    while (tui->running) {
        displaySimulator(tui);
        handleInput(tui);

        // Simulation
        if (!(tui->sim->flags & LC3_SIM_HALTED)) {
            tui->delay = 1;

            struct timeval start, current;
            gettimeofday(&start, NULL);

            for (int elapsed = 0; !(tui->sim->flags & LC3_SIM_HALTED) && elapsed < RUNSTEP_US;) {
                LC3_UntilBreakpoint(tui->sim, 64);
                gettimeofday(&current, NULL);
                elapsed = (((current.tv_sec - start.tv_sec) * 1000000) + current.tv_usec - start.tv_usec);
            }

            if (!LC3_IsAddrDisplayed(tui, tui->sim->pc)) {
                tui->memViewStart = tui->sim->pc;
            }
        } else {
            tui->delay = 250;
        }
    }

    clear();
    refresh();
    return 0;
}
