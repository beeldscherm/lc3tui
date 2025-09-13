#pragma once
#include "../lc3_io.h"  // IWYU pragma: keep
#include "../lc3_tui.h" // IWYU pragma: keep
#include "../lib/optional.h"

// Required format for a commmand function
#define LC3_CMD_FN(name) int (name)(LC3_TermInterface *tui, LC3_SimInstance *sim, int argc, const char **argv)

// Pointer to a command function
#define LC3_CMD_FN_PTR(name) int (*name)(LC3_TermInterface *tui, LC3_SimInstance *sim, int argc, const char **argv)

typedef struct LC3_Command {
    const char *full;
    const char *abbrev;
    LC3_CMD_FN_PTR(func);
    const char *info;
} LC3_Command;

const LC3_Command *getCommands(int *sz);
char getEscaped(int ch);
bool inRange(int n, int min, int max);
OptInt toBase(char digit, uint8_t base);
OptInt getNumber(const char *str);
OptInt parseRegister(LC3_SimInstance *sim, const char *str);
OptInt parseVariable(LC3_SimInstance *sim, const char *var);
OptInt fromInt(int n);
