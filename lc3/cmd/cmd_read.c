#include "cmd_util.h"


// Read new file(s) into memory
// r[ea]d <file> ...
LC3_CMD_FN(loadExecutable) {
    for (int i = 0; i < argc; i++) {
        LC3_LoadExecutable(sim, argv[i]);
    }

    if (!LC3_IsAddrDisplayed(tui, sim->reg.PC)) {
        tui->memViewStart = sim->reg.PC;
    }

    if (sim->error != NULL) {
        LC3_ShowMessage(tui, "failed to load file", true);
        LC3_ShowMessage(tui, sim->error, true);
        sim->error = NULL;
    }

    return (sim->error == NULL);
}
