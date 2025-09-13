#include "cmd_util.h"


// Quit TUI
// q[uit]
LC3_CMD_FN(quitTUI) {
    tui->running = false;
    sim->flags |= LC3_SIM_HALTED;
    return 0;
}
