#include "cmd_util.h"


// Queue input for LC3 simulator
// in[put] "string"
LC3_CMD_FN(giveInput) {
    if (argc != 1) {
        LC3_ShowMessage(tui, "no input provided", true);
        return 1;
    }

    for (int i = 0; argv[0][i]; i++) {
        if (argv[0][i] == '\\' && ++i && getEscaped(argv[0][i]) > 0) {
            LC3_QueueInput(&sim->inputs, getEscaped(argv[0][i]));
        } else {
            LC3_QueueInput(&sim->inputs, argv[0][i]);
        }
    }

    return 0;
}
