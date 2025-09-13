#include "cmd_util.h"


// Start simulation, stops at breakpoint or stop command
// run
LC3_CMD_FN(startSimulation) {
    sim->flags &= ~LC3_SIM_HALTED;

    if (tui->headless) {
        LC3_UntilBreakpoint(sim, -1);
    }

    return 0;
}
