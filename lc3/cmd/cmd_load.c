#include "cmd_util.h"


LC3_CMD_FN(loadSimulator) {
    if (argc != 1) {
        LC3_ShowMessage(tui, "no filename provided", true);
        return 1;
    }

    if (LC3_LoadSimulatorState(sim, argv[0]) != 0) {
        LC3_ShowMessage(tui, "failed to load from file", true);
        return 1;
    }

    return 0;
}
