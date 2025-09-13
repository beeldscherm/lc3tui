#include "cmd_util.h"


// Set input file
// i[nput]f[ile] FILE
LC3_CMD_FN(setInputFile) {
    if (argc < 1 || !argv[0][0]) {
        LC3_ShowMessage(tui, "no file provided", true);
        return 1;
    }

    FILE *fp = fopen(argv[0], "rb");

    if (!fp) {
        LC3_ShowMessage(tui, "unable to open file", true);
        return 1;
    }

    for (int c; fread(&c, 1, 1, fp) == 1; LC3_QueueInput(&sim->inputs, c));

    fclose(fp);
    return 0;
}
