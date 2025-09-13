#include "cmd_util.h"


// Stop simulation at current point
// h[alt]
LC3_CMD_FN(stopSimulation) {
    sim->flags |= LC3_SIM_HALTED;
    return 0;
}
