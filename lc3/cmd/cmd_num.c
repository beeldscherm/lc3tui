#include "cmd_util.h"
#include <ctype.h>


// Set number display type, default x
// n[um] [x/i/u]
LC3_CMD_FN(setnumDisplay) {
    if (argc == 0) {
        tui->numDisplay = LC3_NDISPLAY_HEX;
        return 0;
    }

    switch (toupper(argv[0][0])) {
        case 'X':   tui->numDisplay = LC3_NDISPLAY_HEX;
                    break;
        case 'I':   tui->numDisplay = LC3_NDISPLAY_INT;
                    break;
        case 'U':   tui->numDisplay = LC3_NDISPLAY_UINT;
                    break;
        case 'C':   tui->numDisplay = LC3_NDISPLAY_CHAR;
                    break;
        default:    LC3_ShowMessage(tui, "unknown number format", true);
                    break;
    }

    return 0;
}
