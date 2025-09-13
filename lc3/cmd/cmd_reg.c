#include "cmd_util.h"
#include <ctype.h>


// Set register value
// r[eg] <register> [value]
LC3_CMD_FN(setRegister) {
    char tmp[64];
    #define SR_IfElse(reg, type, min, max)                  \
        if (value.set && inRange(value.value, min, max)) {  \
            reg = (type)value.value;                        \
        } else {                                            \
            sprintf(tmp, "x%04X", reg);                     \
            LC3_ShowMessage(tui, tmp, false);               \
        }

    if (argc < 1 || argc > 2) {
        LC3_ShowMessage(tui, "invalid argc", true);
        return 1;
    }

    OptInt value = {0, 0};
    value = argc > 1 ? parseVariable(sim, argv[1]) : value;

    // Registers are at most 3 characters
    int len = strlen(argv[0]);

    if (len < 1 || len > 3) {
        LC3_ShowMessage(tui, "invalid register name", true);
    }

    char c1 = len > 0 ? toupper(argv[0][0]) : '\0';
    char c2 = len > 1 ? toupper(argv[0][1]) : '\0';
    char c3 = len > 2 ? toupper(argv[0][2]) : '\0';

    if (c1 == 'R' && c2 < '8' && c2 >= '0') {
        SR_IfElse(sim->reg.reg[(c2 - '0')], int16_t, INT16_MIN, UINT16_MAX);
    } else if (c1 == 'P' && c2 == 'C') {
        SR_IfElse(sim->reg.PC, uint16_t, 0, UINT16_MAX);
    } else if (c1 == 'P' && c2 == 'S' && c3 == 'R') {
        SR_IfElse(sim->reg.PSR, uint16_t, 0, UINT16_MAX);
    } else if (c1 == 'M' && c2 == 'A' && c3 == 'R') {
        SR_IfElse(sim->reg.MAR, uint16_t, 0, UINT16_MAX);
    } else if (c1 == 'M' && c2 == 'D' && c3 == 'R') {
        SR_IfElse(sim->reg.MDR, int16_t, INT16_MIN, UINT16_MAX);
    } else {
        LC3_ShowMessage(tui, "invalid register name", true);
        return 1;
    }

    if (!LC3_IsAddrDisplayed(tui, sim->reg.PC)) {
        tui->memViewStart = sim->reg.PC;
    }

    return 0;
}
