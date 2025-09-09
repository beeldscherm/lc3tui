#include "lc3_cmd.h"
#include "lib/optional.h"
#include <ctype.h>
#include "lib/leakcheck/lc.h"

// Required format for a commmand function, local to this file
#define LC3_CMD_FN(name) static int (name)(LC3_TermInterface *tui, LC3_SimInstance *sim, int argc, const char **argv)

// Pointer to a command function
#define LC3_CMD_FN_PTR(name) int (*name)(LC3_TermInterface *tui, LC3_SimInstance *sim, int argc, const char **argv)


// Get escape character version of ch (e.g. 'n' -> '\n'), or -1 if it does not exist
char getEscaped(int ch) {
    static const char map[] = {
        -1,   -1,   '\"', -1,   -1,   -1,   -1,   '\'', -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   
        '\0', -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   '\\', -1,   -1,   -1,   
        -1,   '\a', '\b', -1,   -1,   -1,   '\f', -1,   -1,   -1,   -1,   -1,   -1,   -1,   '\n', -1,   
        -1,   -1,   '\r', -1,   '\t', -1,   '\v', -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    };

    return (ch >= 32 && ch < 128) ? map[ch - 32] : -1;
}


// Strings for escaped characters
const char *charString(int ch) {
    static const char *map[] = {
        "\\0",   "\\x01", "\\x02", "\\x03", "\\x04", "\\x05", "\\x06", "\\a",   "\\b",   "\\t",   "\\n",   "\\v",   "\\f",   "\\r",   "\\x0E", "\\x0F", 
        "\\x10", "\\x11", "\\x12", "\\x13", "\\x14", "\\x15", "\\x16", "\\x17", "\\x18", "\\x19", "\\x1A", "\\e",   "\\x1C", "\\x1D", "\\x1E", "\\x1F", 
        " ",     "!",     "\"",    "#",     "$",     "%",     "&",     "'",     "(",     ")",     "*",     "+",     ",",     "-",     ".",     "/",     
        "0",     "1",     "2",     "3",     "4",     "5",     "6",     "7",     "8",     "9",     ":",     ";",     "<",     "=",     ">",     "?",     
        "@",     "A",     "B",     "C",     "D",     "E",     "F",     "G",     "H",     "I",     "J",     "K",     "L",     "M",     "N",     "O",     
        "P",     "Q",     "R",     "S",     "T",     "U",     "V",     "W",     "X",     "Y",     "Z",     "[",     "\\\\",  "]",     "^",     "_",     
        "`",     "a",     "b",     "c",     "d",     "e",     "f",     "g",     "h",     "i",     "j",     "k",     "l",     "m",     "n",     "o",     
        "p",     "q",     "r",     "s",     "t",     "u",     "v",     "w",     "x",     "y",     "z",     "{",     "|",     "}",     "~",     "\\xFF",
    };

    return ((ch > 0 && ch < 128) ? map[ch] : "\\?");
}


// Convert digit character to (optional) digit in certain base
OptInt toBase(char digit, uint8_t base) {
    // Why not
    static const int8_t BASE_CONVERT_MAP[256] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1, 
        -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 
        25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1, 
        -1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 
        25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    };

    OptInt ret = {
        .value = BASE_CONVERT_MAP[(uint8_t)digit],
        .set   = BASE_CONVERT_MAP[(uint8_t)digit] < base,
    };

    return ret;
}


// Turns a string into a number if possible
OptInt getNumber(const char *str) {
    uint8_t base = 10;
    size_t idx = 0;
    int8_t factor = 1;
    OptInt ret = {0, false};

    // Deduce base from first character
    if (str[idx] == '#') {
        idx++;
    } else if (str[idx] == 'X' || str[idx] == 'x') {
        idx++;
        base = 16;
    } else if (str[idx] == 'B' || str[idx] == 'b') {
        idx++;
        base = 2;
    } else if (!isdigit(str[idx])) {
        return ret;
    }

    // Deal with negative numbers
    if (str[idx] == '-') {
        factor = -1;
        idx++;
    }

    if (!str[idx]) {
        return ret;
    }

    ret.set = true;

    // Calculate number value
    for (size_t i = idx; ret.set && str[i]; i++) {
        OptInt digit = toBase(str[i], base);
        ret.set = digit.set;
        ret.value *= base;
        ret.value += digit.value;
    }

    ret.value *= factor;
    return ret;
}


// Get register value from register string
OptInt parseRegister(LC3_SimInstance *sim, const char *str) {
    OptInt ret = {0, false};
    int len = strlen(str);

    if (len != 2) {
        return ret;
    }

    if (toupper(str[0]) == 'R' && str[1] < '8' && str[1] >= '0') {
        ret.value = sim->reg.reg[str[1] - '0'];
        ret.set = true;
    } else if (toupper(str[0]) == 'P' && toupper(str[1]) == 'C') {
        ret.value = sim->reg.PC;
        ret.set = true;
    }

    return ret;
}


// Get value from variable string (either number or register)
OptInt parseVariable(LC3_SimInstance *sim, const char *var) {
    // Check if it's a number
    OptInt n = getNumber(var);

    // Check for register
    n = n.set ? n : parseRegister(sim, var);

    return n;
}


// Turn n into a set OptInt
OptInt fromInt(int n) {
    OptInt ret = {n, true};
    return ret;
}


// Check if n is in a certain range
bool inRange(int n, int min, int max) {
    return ((n >= min) && (n <= max));
}


// Read new file(s) into memory
// r[ea]d <file> ...
LC3_CMD_FN(loadExecutable) {
    for (int i = 0; i < argc; i++) {
        LC3_LoadExecutable(sim, argv[i]);
    }

    if (!LC3_IsAddrDisplayed(tui, sim->reg.PC)) {
        tui->memViewStart = sim->reg.PC;
    }

    if (sim->error != NULL) {
        LC3_ShowMessage(tui, "failed to load file", true);
        sim->error = NULL;
    }

    return (sim->error == NULL);
}


// Set breakpoint at provided location(s)
// If no locations are provided, PC is assumed
// b[reak]p[oint] [location] ...
LC3_CMD_FN(breakpoint) {
    if (argc == 0) {
        sim->memory[sim->reg.PC].breakpoint = !sim->memory[sim->reg.PC].breakpoint;
        return 0;
    }

    for (int i = 0; i < argc; i++) {
        OptInt n = parseVariable(sim, argv[i]);

        if (n.set && inRange(n.value, 0, UINT16_MAX)) {
            sim->memory[n.value].breakpoint = !sim->memory[n.value].breakpoint;
        } else {
            LC3_ShowMessage(tui, "invalid location", true);
        }
    }

    return 0;
}


// Set value in memory
// If no location is provided, PC is assumed
// s[et] [location] <value> ...
LC3_CMD_FN(setMemoryValue) {
    if (argc < 1) {
        return 1;
    }

    OptInt location = (argc > 1) ? parseVariable(sim, argv[0]) : fromInt(sim->reg.PC);

    if (!location.set) {
        LC3_ShowMessage(tui, "invalid location", true);
        return 1;
    }

    for (int i = (argc > 1); i < argc && inRange(location.value, 0, UINT16_MAX); i++) {
        OptInt value = parseVariable(sim, argv[i]);

        if (value.set && inRange(value.value, INT16_MIN, UINT16_MAX)) {
            sim->memory[location.value].value = (int16_t)value.value;
        } else {
            LC3_ShowMessage(tui, "invalid value", true);
        }

        location.value++;
    }

    return 0;
}


// Set register value
// r[eg] <register> <value>
LC3_CMD_FN(setRegister) {
    if (argc != 2 || strlen(argv[0]) != 2) {
        return 1;
    }

    OptInt value = parseVariable(sim, argv[1]);
    char c1 = toupper(argv[0][0]);
    char c2 = toupper(argv[0][1]);

    if (value.set && c1 == 'R' && c2 < '8' && c2 >= '0' && inRange(value.value, INT16_MIN, UINT16_MAX)) {
        sim->reg.reg[(c2 - '0')] = value.value;
        return 0;
    } else if (value.set && c1 == 'P' && c2 == 'C' && inRange(value.value, 0, UINT16_MAX)) {
        sim->reg.PC = value.value;
        return 0;
    } else {
        LC3_ShowMessage(tui, "invalid argument", true);
    }

    return 1;
}


// Start simulation, stops at breakpoint or stop command
// run
LC3_CMD_FN(startSimulation) {
    sim->flags &= ~LC3_SIM_HALTED;
    return 0;
}


// Stop simulation at current point
// h[alt]
LC3_CMD_FN(stopSimulation) {
    sim->flags |= LC3_SIM_HALTED;
    return 0;
}

// Undo previous N simulation step(s)
// u[ndo] [N]
LC3_CMD_FN(undoSteps) {
    OptInt steps = (argc > 0) ? parseVariable(sim, argv[0]) : fromInt(1);

    for (int i = 0; sim->history.sz > 0 && steps.set && i < steps.value; i++) {
        sim->history.sz--;
        LC3_PrevState state = sim->history.ptr[sim->history.sz];

        sim->reg = state.reg;
        sim->memory[state.memoryLocation].value = state.memoryValue;
    }

    if (!LC3_IsAddrDisplayed(tui, sim->reg.PC)) {
        tui->memViewStart = sim->reg.PC;
    }

    return 0;
}


// Set number display type, default x
// n[um] [x/i/u]
LC3_CMD_FN(setnumDisplay) {
    if (argc == 0) {
        tui->numDisplay = LC3_NDISPLAY_HEX;
        return 0;
    }

    switch (toupper(argv[0][0])) {
        case 'X':   tui->numDisplay = LC3_NDISPLAY_HEX;
                    break;
        case 'I':   tui->numDisplay = LC3_NDISPLAY_INT;
                    break;
        case 'U':   tui->numDisplay = LC3_NDISPLAY_UINT;
                    break;
        default:    LC3_ShowMessage(tui, "unknown number format", true);
                    break;
    }

    return 0;
}


// Execute N simulation steps, stops at breakpoints
// st[ep] [N]
LC3_CMD_FN(makeSteps) {
    OptInt steps = (argc > 0) ? parseVariable(sim, argv[0]) : fromInt(1);

    if (steps.set) {
        LC3_UntilBreakpoint(sim, steps.value);
        tui->memViewStart = (LC3_IsAddrDisplayed(tui, sim->reg.PC)) ? tui->memViewStart : sim->reg.PC;
        sim->flags |= LC3_SIM_HALTED;
    }

    return 0;
}


// Quit TUI
// q[uit]
LC3_CMD_FN(quitTUI) {
    tui->running = false;
    sim->flags |= LC3_SIM_HALTED;
    return 0;
}


// Queue input for LC3 simulator
// in[put] "string"
LC3_CMD_FN(giveInput) {
    if (argc != 1) {
        LC3_ShowMessage(tui, "no input provided", true);
        return 1;
    }

    for (int i = 0; argv[0][i]; i++) {
        if (argv[0][i] == '\\' && ++i && getEscaped(argv[0][i]) > 0) {
            LC3_QueueInput(&sim->inputs, getEscaped(argv[0][i]));
        } else {
            LC3_QueueInput(&sim->inputs, argv[0][i]);
        }
    }

    return 0;
}


// Clear queue'd inputs
// n[o]in[put]
LC3_CMD_FN(removeInputs) {
    sim->inputs.hd = sim->inputs.tl = 0;
    return 0;
}


// Reset LC3 simulator completely
// restart
LC3_CMD_FN(restartDevice) {
    LC3_DestroySimInstance(*sim);
    (*sim) = LC3_CreateSimInstance();
    tui->sim = sim;
    return 0;
}


// Go to provided address, default PC
// g[o] [N]
LC3_CMD_FN(goToCell) {
    OptInt addr = (argc > 0) ? parseVariable(sim, argv[0]) : fromInt(sim->reg.PC);

    if (addr.set && inRange(addr.value, INT16_MIN, UINT16_MAX)) {
        tui->memViewStart = (uint16_t)addr.value;
    }

    return 0;
}


// Set input file
// i[nput]f[ile] FILE
LC3_CMD_FN(setInputFile) {
    if (argc < 1 || !argv[0][0]) {
        LC3_ShowMessage(tui, "no file provided", true);
        return 1;
    }

    FILE *fp = fopen(argv[0], "rb");

    if (!fp) {
        LC3_ShowMessage(tui, "unable to open file", true);
        return 1;
    }

    for (int c; fread(&c, 1, 1, fp) == 1; LC3_QueueInput(&sim->inputs, c));

    fclose(fp);
    return 0;
}


// Set output file
// o[utput]f[ile] FILE
LC3_CMD_FN(setOutputFile) {
    if (argc < 1 || !argv[0][0]) {
        LC3_ShowMessage(tui, "no file provided", true);
        return 1;
    }

    sim->outf = fopen(argv[0], "wb");

    if (sim->outf == NULL) {
        LC3_ShowMessage(tui, "no file provided", true);
        return 1;
    }
    return 0;
}


// Clear output box
// clear
LC3_CMD_FN(clearOutput) {
    sim->output.ptr[0] = '\0';
    sim->output.sz = 0;
    return 0;
}


// Clear output box
// clear
LC3_CMD_FN(counterCommands) {
    char counterString[64] = "";

    if (argc == 0 || strcmp(argv[0], "get") == 0) {
        sprintf(counterString, "count: %ld", sim->counter - sim->c2);
        LC3_ShowMessage(tui, counterString, false);
    } else if (strcmp(argv[0], "total") == 0) {
        sprintf(counterString, "total: %ld", sim->counter);
        LC3_ShowMessage(tui, counterString, false);
    } else if (strcmp(argv[0], "reset") == 0) {
        sim->c2 = sim->counter;
    } else {
        LC3_ShowMessage(tui, "invalid argument", true);
    }

    return 0;
}


// Show help info
// help
LC3_CMD_FN(showHelpInfo) {
    noecho();
    cbreak();
    int c = -1, x = 0, y = 0;

    while (c) {
        clear();
    
        mvprintw(y, x, "Commands:");
    
        mvprintw(y + 1,  x, "    l[oa]d FILE            | Loads .lc3 file into memory");
        mvprintw(y + 2,  x, "    b[reak]p[point] N ...  | Sets breakpoint at provided locations (PC assumed)");
        mvprintw(y + 3,  x, "    s[et] [N1] N2          | Sets address N1 (PC assumed) to N2");
        mvprintw(y + 4,  x, "    r[eg] R N              | Sets register R to value N");
        mvprintw(y + 5,  x, "    run                    | Run simulator until breakpoint or halted");
        mvprintw(y + 6,  x, "    h[alt]                 | Halt simulator");
        mvprintw(y + 7,  x, "    st[ep] [N]             | Execute N instructions (1 assumed), or until breakpoint");
        mvprintw(y + 8,  x, "    u[ndo] [N]             | Undo previous N instructions (1 assumed)");
        mvprintw(y + 9,  x, "    n[um] [x/i/u]          | Set number display type (hex, int, unsigned), hex assumed");
        mvprintw(y + 10, x, "    q[uit]                 | Quit this program");
        mvprintw(y + 11, x, "    in[put] ...            | Queues any characters (possibly escaped) after the delimiter for input");
        mvprintw(y + 12, x, "    n[o]in[put]            | Delete all queued input");
        mvprintw(y + 13, x, "    restart                | Clear simulator");
        mvprintw(y + 14, x, "    g[o] [N]               | Scroll memory view N (PC assumed)");
        mvprintw(y + 15, x, "    help                   | Show this message");
        mvprintw(y + 16, x, "    i[nput]f[ile] FILE     | Set file to take input from, this file has higher precedence than the input box");
        mvprintw(y + 17, x, "    o[utput]f[ile] FILE    | Set file to put output into, control characters are outputted directly");
        mvprintw(y + 18, x, "    clear                  | Clear output box");
        mvprintw(y + 20, x, "    c[ou]nt [get]          | Get instrutions executed since last counter reset");
        mvprintw(y + 21, x, "    c[ou]nt reset          | Reset counter to 0");
        mvprintw(y + 22, x, "    c[ou]nt total          | Get total amount of instructions executed");
        mvprintw(y + 23, x, "    WHERE [N] is either a [REG] (register string) or number");
    
        mvprintw(y + 25, x, "Displayed:");
        mvprintw(y + 26, x, "    Top left               | Memory viewer");
        mvprintw(y + 27, x, "    Top right              | Register status");
        mvprintw(y + 28, x, "    Bottom                 | Program output");
        mvprintw(y + 29, x, "    Middle right           | Queued input");
        mvprintw(y + 30, x, "    Bottom right           | Simulator state, [R]unning or [H]alted");
    
        refresh();
        c = getch();

        switch (c) {
            case 'k':
            case KEY_UP:    y = (++y < 0) ? y : 0; 
                            break;
            case 'j':
            case KEY_DOWN:  y--; 
                            break;
            case KEY_RESIZE: break;
            default: c = 0; break;
        }
    }
    
    clear();
    return 0;
}


// Mapping between command string and command functions
static const struct {const char * name; LC3_CMD_FN_PTR(func); } COMMAND_MAP[] = {
    {"ld",          loadExecutable},
    {"load",        loadExecutable},
    {"bp",          breakpoint},
    {"breakpoint",  breakpoint},
    {"s",           setMemoryValue},
    {"set",         setMemoryValue},
    {"r",           setRegister},
    {"reg",         setRegister},
    {"run",         startSimulation},
    {"h",           stopSimulation},
    {"halt",        stopSimulation},
    {"u",           undoSteps},
    {"undo",        undoSteps},
    {"n",           setnumDisplay},
    {"num",         setnumDisplay},
    {"st",          makeSteps},
    {"step",        makeSteps},
    {"q" ,          quitTUI},
    {"quit",        quitTUI},
    {"in" ,         giveInput},
    {"input",       giveInput},
    {"nin" ,        removeInputs},
    {"noinput",     removeInputs},
    {"restart",     restartDevice},
    {"g",           goToCell},
    {"go",          goToCell},
    {"help",        showHelpInfo},
    {"if",          setInputFile},
    {"inputfile",   setInputFile},
    {"of",          setOutputFile},
    {"outputfile",  setOutputFile},
    {"clear",       clearOutput},
    {"cnt",         counterCommands},
    {"count",       counterCommands},
};


// Get function from command string
LC3_CMD_FN_PTR(getCommandFunction(const char *instr)) {
    // Linear search
    for (int i = 0; i < (sizeof(COMMAND_MAP) / sizeof(COMMAND_MAP[0])); i++) {
        if (strcmp(instr, COMMAND_MAP[i].name) == 0) {
            return COMMAND_MAP[i].func;
        }
    }

    return NULL;
}


// List of cosnt char * for argv
vaTypedef(const char *, ArgumentList);
static vaAllocFunction(ArgumentList, const char *, newArgumentList, ;, ;)
static vaAppendFunction(ArgumentList, const char *, addArgument, ;, ;)


// Execute the command in cmd on the provided tui
void LC3_ExecuteCommand(LC3_TermInterface *tui, const char *cmd) {
    if (cmd == NULL) {
        return;
    }

    int len = strlen(cmd) + 1;

    if (len < 0) {
        return;
    }

    char *copy = lc_malloc(len);
    memcpy(copy, cmd, len);

    char *token = strtok(copy, " ;,|");
    LC3_CMD_FN_PTR(func) = getCommandFunction(token);

    // Special logic for input
    if (func == giveInput) {
        const char *arg = strtok(NULL, "");
        func(tui, tui->sim, (arg != NULL), &arg);
    } else if (func != NULL) {
        // Get arguments
        ArgumentList argv = newArgumentList();

        for (; (token = strtok(NULL, " ;,|")); addArgument(&argv, token));

        func(tui, tui->sim, argv.sz, argv.ptr);
        lc_free(argv.ptr);
    } else {
        LC3_ShowMessage(tui, "unknown command", true);
    }

    lc_free(copy);
}
