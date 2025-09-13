#include "cmd_util.h"
#include "../lib/optional.h"


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