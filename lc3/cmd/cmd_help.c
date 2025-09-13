#include "cmd_util.h"


// Show help info
// help
LC3_CMD_FN(showHelpInfo) {
    if (tui->headless) {
        return 0;
    }

    int sz = 0;
    const LC3_Command *commands = getCommands(&sz);

    noecho();
    cbreak();
    int c = -1, x = 0, y = 0;

    while (c) {
        clear();
    
        mvprintw(y, x, "Commands:");

        for (int i = 0; i < sz; i++) {
            mvprintw(y + i + 1, x, "    %s\n", commands[i].info);
        }

        mvprintw(y + sz + 1, x, "    WHERE [N] is either a [REG] (register string) or number");
        mvprintw(y + sz + 3, x, "Displayed:");
        mvprintw(y + sz + 4, x, "    Top left               | Memory viewer");
        mvprintw(y + sz + 5, x, "    Top right              | Register status");
        mvprintw(y + sz + 6, x, "    Bottom                 | Program output");
        mvprintw(y + sz + 7, x, "    Middle right           | Queued input");
        mvprintw(y + sz + 8, x, "    Bottom right           | Simulator state, [R]unning or [H]alted");
    
        refresh();
        c = getch();

        switch (c) {
            case 'k':
            case KEY_UP:    y = (++y < 0) ? y : 0; 
                            break;
            case 'j':
            case KEY_DOWN:  y--; 
                            break;
            case KEY_RESIZE: break;
            default: c = 0; break;
        }
    }
    
    clear();
    return 0;
}
