#include "lc3_cmd.h"
#include <ctype.h>
#include "lib/leakcheck/lc.h"
#include "cmd/cmd_util.h"


// Pointer to a command function


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


#include "cmd/cmd_read.c"
#include "cmd/cmd_breakpoint.c"
#include "cmd/cmd_set.c"
#include "cmd/cmd_reg.c"
#include "cmd/cmd_run.c"
#include "cmd/cmd_halt.c"
#include "cmd/cmd_undo.c"
#include "cmd/cmd_num.c"
#include "cmd/cmd_step.c"
#include "cmd/cmd_quit.c"
#include "cmd/cmd_input.c"
#include "cmd/cmd_noinput.c"
#include "cmd/cmd_restart.c"
#include "cmd/cmd_go.c"
#include "cmd/cmd_infile.c"
#include "cmd/cmd_outfile.c"
#include "cmd/cmd_clear.c"
#include "cmd/cmd_count.c"
#include "cmd/cmd_help.c"


static const LC3_Command CMD_MAP[] = {
    {"read",        "rd",   loadExecutable,     "r[ea]d FILE             | Read .lc3 file into memory"},
    {"breakpoint",  "bp",   breakpoint,         "b[reak]p[point] N ...   | Sets breakpoint at provided locations (PC assumed)"},
    {"set",         "s",    setMemoryValue,     "s[et] [N1] N2           | Sets address N1 (PC assumed) to N2"},
    {"reg",         "r",    setRegister,        "r[eg] R [N]             | Sets register R to value N, or show R as 4-digit hex if N is not provided"},
    {"run",         NULL,   startSimulation,    "run                     | Run simulator until breakpoint or halted"},
    {"halt",        "h",    stopSimulation,     "h[alt]                  | Halt simulator"},
    {"undo",        "u",    undoSteps,          "u[ndo] [N]              | Undo previous N instructions (1 assumed)"},
    {"num",         "n",    setnumDisplay,      "n[um] [x/i/u/c]         | Set number display type (hex, int, unsigned, char), hex assumed"},
    {"step",        "st",   makeSteps,          "st[ep] [N]              | Execute N instructions (1 assumed), or until breakpoint"},
    {"quit",        "q",    quitTUI,            "q[uit]                  | Quit this program"},
    {"input",       "in",   giveInput,          "in[put] ...             | Queues any characters (possibly escaped) after the delimiter for input"},
    {"noinput",     "nin",  removeInputs,       "n[o]in[put]             | Delete all queued input"},
    {"restart",     NULL,   restartDevice,      "restart                 | Clear simulator"},
    {"go",          "g",    goToCell,           "g[o] [N]                | Scroll memory view N (PC assumed)"},
    {"help",        NULL,   showHelpInfo,       "help                    | Show this message"},
    {"inputfile",   "if",   setInputFile,       "i[nput]f[ile] FILE      | Set file to take input from, this file has higher precedence than the input box"},
    {"outputfile",  "of",   setOutputFile,      "o[utput]f[ile] FILE     | Set file to put output into, control characters are outputted directly"},
    {"clear",       NULL,   clearOutput,        "clear                   | Clear output box"},
    {"count",       "cnt",  counterCommands,    "count [get]/reset/total | Get amount of instructions executed, reset count, or get total count"}
};


const LC3_Command *getCommands(int *sz) {
    (*sz) = sizeof(CMD_MAP) / sizeof(CMD_MAP[0]);
    return CMD_MAP;
}


// Get function from command string
LC3_CMD_FN_PTR(getCommandFunction(const char *instr)) {
    // Linear search
    for (int i = 0; i < (sizeof(CMD_MAP) / sizeof(CMD_MAP[0])); i++) {
        if (strcmp(instr, CMD_MAP[i].full) == 0 || (CMD_MAP[i].abbrev && (strcmp(instr, CMD_MAP[i].abbrev) == 0))) {
            return CMD_MAP[i].func;
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

    if (len <= 0) {
        return;
    }

    char *copy = lc_malloc(len);
    memcpy(copy, cmd, len);

    char *token = strtok(copy, " ;,|");

    if (token == NULL) {
        free(copy);
        return;
    }

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
