#include "cmd_util.h"


// Counting commands
// count
LC3_CMD_FN(counterCommands) {
    char counterString[64] = "";

    if (argc == 0 || strcmp(argv[0], "get") == 0) {
        sprintf(counterString, "count: %ld", sim->counter - sim->c2);
        LC3_ShowMessage(tui, counterString, false);
    } else if (strcmp(argv[0], "total") == 0) {
        sprintf(counterString, "total: %ld", sim->counter);
        LC3_ShowMessage(tui, counterString, false);
    } else if (strcmp(argv[0], "reset") == 0) {
        sim->c2 = sim->counter;
    } else {
        LC3_ShowMessage(tui, "invalid argument", true);
    }

    return 0;
}
