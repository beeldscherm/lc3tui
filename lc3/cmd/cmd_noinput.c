#include "cmd_util.h"


// Clear queue'd inputs
// n[o]in[put]
LC3_CMD_FN(removeInputs) {
    sim->inputs.hd = sim->inputs.tl = 0;
    return 0;
}
