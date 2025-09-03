#include "lc3/config.h"
#include "lc3/lc3_tui.h"


int main(int argc, char **argv) {
    LC3_SimInstance sim = LC3_CreateSimInstance();
    LC3_TermInterface tui = LC3_CreateTermInterface(&sim);

    LC3_RunTermInterface(&tui);

    LC3_DestroyTermInterface(tui);
    LC3_DestroySimInstance(sim);

    #if DO_LEAK_CHECK
    lc_summary();
    #endif

    return 0;
}
