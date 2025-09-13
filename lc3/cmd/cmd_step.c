#include "cmd_util.h"


// Execute N simulation steps, stops at breakpoints
// st[ep] [N]
LC3_CMD_FN(makeSteps) {
    OptInt steps = (argc > 0) ? parseVariable(sim, argv[0]) : fromInt(1);

    if (steps.set) {
        sim->flags &= ~(LC3_SIM_HALTED);
        LC3_UntilBreakpoint(sim, steps.value);
        tui->memViewStart = (LC3_IsAddrDisplayed(tui, sim->reg.PC)) ? tui->memViewStart : sim->reg.PC;
        sim->flags |= LC3_SIM_HALTED;
    }

    return 0;
}
