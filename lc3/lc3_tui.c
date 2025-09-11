#include "lc3_tui.h"
#include "lc3_cmd.h"
#include "lib/cmdarg/cmdarg.h"
#include <sys/time.h>
#include "lib/leakcheck/lc.h"

#define CMD_LEN_MAX (256)
#define RUNSTEP_US  (100000)


String copyString(String str) {
    String ret = {
        .ptr = lc_malloc(str.cap),
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
        str->ptr = lc_realloc(str->ptr, str->cap * sizeof(char));
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


LC3_TermInterface LC3_CreateTermInterface(LC3_SimInstance *sim, int argc, char **argv) {
    LC3_TermInterface ret = {
        .sim = sim,
        .cols = 0,
        .rows = 0,
        .memViewStart = sim->reg.PC,
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
        .headless = false,
    };

    // Handle flag(s)
    enum Flag {
        HEADLESS = 0x01,
    };

    ca_config *config = ca_alloc_config();
    ca_bind_flag(config, "--headless", HEADLESS);

    ca_info *info = ca_parse(config, argc, argv);
    uint64_t flags = ca_flags(info);

    ca_free_config(config);
    ca_free_info(info);

    if ((ret.headless = (flags & HEADLESS) > 0)) {
        return ret;
    }

    // ncurses setup
    initscr();
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
    freeStringArray(tui.commands);

    if (tui.headless) {
        return;
    }

    // ncurses cleanup
    destroyWindow(tui.memView);
    destroyWindow(tui.regView);
    destroyWindow(tui.inView);
    destroyWindow(tui.outView);
    destroyWindow(tui.inViewBox);
    destroyWindow(tui.outViewBox);
    endwin();
}


#define BOX_PAD (2)

#define REG_VIEW_W() (24)
#define REG_VIEW_H() (13)

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
                                lc_free(cmd.ptr);
                                hist = (hist > 0) ? (hist - 1) : 0;
                                cmd = copyString(tui->commands.ptr[hist]);
                                idx = cmd.sz;
                                break;
            case KEY_DOWN:      lc_free(cmd.ptr);
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
        lc_free(cmd.ptr);
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
                            halfdelay(tui->delay);
                            break;
            case '\n':      tui->sim->flags &= ~LC3_SIM_HALTED;
                            LC3_ExecuteInstruction(tui->sim);
                            tui->sim->flags |= LC3_SIM_HALTED;
                            if (!LC3_IsAddrDisplayed(tui, tui->sim->reg.PC)) {
                                tui->memViewStart = tui->sim->reg.PC;
                            }
                            break;
            case 'u':       LC3_ExecuteCommand(tui, "u");
        }
    }
}


void LC3_ShowMessage(LC3_TermInterface *tui, const char *msg, bool isError) {
    if (tui->headless) {
        printf("%s\n", msg);
        return;
    }

    if (isError) {
        attron(COLOR_PAIR(1));
    }

    move(tui->rows - 1, 0);
    clrtoeol();
    addstr(msg);
    attroff(COLOR_PAIR(1));
}


#define CC_CHAR(n, t, f) ((((sim->reg.PSR & 7) & n) != 0) ? t : f)
#define FMT_STR_LEN (20)


static void printValue(LC3_TermInterface *tui, WINDOW *win, uint16_t value) {
    switch (tui->numDisplay) {
        case LC3_NDISPLAY_HEX:  wprintw(win, "0x%04X", value);
                                break;
        case LC3_NDISPLAY_UINT: wprintw(win, "%6hu", value);
                                break;
        case LC3_NDISPLAY_INT:  wprintw(win, "%6hi", (int16_t)value);
                                break;
        case LC3_NDISPLAY_CHAR: wprintw(win, "%3s%3s", charString(value >> 8), charString(value & 0xFF));
                                break;
        default: break;
    }
}


static void displaySimulator(LC3_TermInterface *tui) {
    LC3_SimInstance *sim = tui->sim;
    resizeCheck(tui);

    // Draw memory view
    box(tui->memView, 0, 0);

    for (int i = tui->memViewStart, y = 1; y <= MEM_VIEW_H() ; i = loopAround(i + 1, LC3_MEM_SIZE), y++) {
        int max = MEM_VIEW_W() - FMT_STR_LEN;

        if (sim->memory[i].breakpoint) {
            wattron(tui->memView, COLOR_PAIR(1));
        }

        if (i == sim->reg.PC) {
            wattron(tui->memView, COLOR_PAIR(2));
        }

        mvwprintw(tui->memView, y, 2, "%cx%04X | ", (i == sim->reg.PC) ? '>' : ' ', (uint16_t)i);
        printValue(tui, tui->memView, sim->memory[i].value);
        wprintw(tui->memView, " | ");

        for (int i = 0; i < max; waddch(tui->memView, ' '), i++);

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
    box(tui->regView, 0, 0);

    for (int i = 0; i < 8; i++) {
        mvwprintw(tui->regView, i + 1, 1, " R%1i | ", i);
        printValue(tui, tui->regView, sim->reg.reg[i]);
    }

    mvwprintw(tui->regView,  9, 1, "PSR | 0x%04X", sim->reg.PSR);
    mvwprintw(tui->regView, 10, 1, " CC | %c %c %c ", CC_CHAR(0x4, 'N', '.'), CC_CHAR(0x2, 'Z', '.'), CC_CHAR(0x1, 'P', '.'));

    mvwprintw(tui->regView, 11, 1, " PC | 0x%04X", sim->reg.PC);
    mvwprintw(tui->regView, 12, 1, "MAR | 0x%04X", sim->reg.MAR);
    mvwprintw(tui->regView, 13, 1, "MDR | 0x%04X", (uint16_t)sim->reg.MDR);

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


// Main loop for TUI mode
static void runTermInterfaceDefault(LC3_TermInterface *tui) {
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

            if (!LC3_IsAddrDisplayed(tui, tui->sim->reg.PC)) {
                tui->memViewStart = tui->sim->reg.PC;
            }
        } else {
            tui->delay = 250;
        }
    }

    clear();
    refresh();
}


// Main loop for headless mode
static void runTermInterfaceHeadless(LC3_TermInterface *tui) {
    tui->running = true;

    char cmd[CMD_LEN_MAX + 1] = {0};
    int ch = 0, i;

    while (tui->running) {
        // Read command input from stdin
        for (i = 0; (ch = getchar()) != EOF && ch != '\n' && i < CMD_LEN_MAX; i++) {
            cmd[i] = (char)ch;
        }

        if (ch == EOF) {
            break;
        }

        cmd[i] = '\0';
        LC3_ExecuteCommand(tui, cmd);
    }
}


int LC3_RunTermInterface(LC3_TermInterface *tui) {
    if (tui->headless) {
        runTermInterfaceHeadless(tui);
    } else {
        runTermInterfaceDefault(tui);
    }

    return 0;
}
