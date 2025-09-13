#include "cmd_util.h"


// Go to provided address, default PC
// g[o] [N]
LC3_CMD_FN(goToCell) {
    OptInt addr = (argc > 0) ? parseVariable(sim, argv[0]) : fromInt(sim->reg.PC);

    if (addr.set && inRange(addr.value, INT16_MIN, UINT16_MAX)) {
        tui->memViewStart = (uint16_t)addr.value;
    }

    return 0;
}
