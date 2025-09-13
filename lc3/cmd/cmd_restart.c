#include "cmd_util.h"


// Reset LC3 simulator completely
// restart
LC3_CMD_FN(restartDevice) {
    LC3_DestroySimInstance(*sim);
    (*sim) = LC3_CreateSimInstance();
    tui->sim = sim;
    return 0;
}
