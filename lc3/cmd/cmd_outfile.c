#include "cmd_util.h"


// Set output file
// o[utput]f[ile] FILE
LC3_CMD_FN(setOutputFile) {
    if (argc < 1 || !argv[0][0]) {
        LC3_ShowMessage(tui, "no file provided", true);
        return 1;
    }

    sim->outf = fopen(argv[0], "wb");

    if (sim->outf == NULL) {
        LC3_ShowMessage(tui, "no file provided", true);
        return 1;
    }
    return 0;
}
