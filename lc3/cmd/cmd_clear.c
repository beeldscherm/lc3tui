#include "cmd_util.h"


// Clear output box
// clear
LC3_CMD_FN(clearOutput) {
    sim->output.ptr[0] = '\0';
    sim->output.sz = 0;
    return 0;
}
