#include "cmd_util.h"


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
