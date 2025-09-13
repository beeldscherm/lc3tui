#include "cmd_util.h"


// Set value in memory
// If no location is provided, PC is assumed
// s[et] [location] <value> ...
LC3_CMD_FN(setMemoryValue) {
    if (argc < 1) {
        return 1;
    }

    OptInt location = (argc > 1) ? parseVariable(sim, argv[0]) : fromInt(sim->reg.PC);

    if (!location.set) {
        LC3_ShowMessage(tui, "invalid location", true);
        return 1;
    }

    for (int i = (argc > 1); i < argc && inRange(location.value, 0, UINT16_MAX); i++) {
        OptInt value = parseVariable(sim, argv[i]);

        if (value.set && inRange(value.value, INT16_MIN, UINT16_MAX)) {
            sim->memory[location.value].value = (int16_t)value.value;
        } else {
            LC3_ShowMessage(tui, "invalid value", true);
        }

        location.value++;
    }

    return 0;
}
