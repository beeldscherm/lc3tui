#pragma once
#include <stdbool.h>
#include <curses.h>
#include "lc3_sim.h"

// Number formats for the TUI
typedef enum LC3_numDisplay {
    LC3_NDISPLAY_HEX  = 0,          // Hexadecimal, always padded to 4 digits
    LC3_NDISPLAY_UINT,              // Unsigned integer
    LC3_NDISPLAY_INT,               // Signed integer
    LC3_NDISPLAY_MAX                // Required to be the last element for calculations
} LC3_numDisplay;


// Terminal UI for a sim instance
typedef struct LC3_TermInterface {
    LC3_SimInstance *sim;           // Simulator reference
    int rows, cols;                 // Size of the terminal
    int memViewStart;               // Address where memory view starts
    WINDOW *memView;                // Memory view window
    WINDOW *regView;                // Register view window
    WINDOW *inView;                 // Input window
    WINDOW *outView;                // Output window
    WINDOW *inViewBox;              // Input window border
    WINDOW *outViewBox;             // Output window border
    StringArray commands;           // Previously executed commands
    int delay;                      // Character wait delay
    LC3_numDisplay numDisplay;      // Number display format
    bool running;                   // TUI exits once this turns false
} LC3_TermInterface;


/*
 * Allocate terminal UI for simulator
 * After use, this should be destroyed using LC3_DestroyTermInterface()
 */
LC3_TermInterface LC3_CreateTermInterface(LC3_SimInstance *sim);

/*
 * Deallocate terminal UI
 * Does not deallocate the simulator it references
 */
void LC3_DestroyTermInterface(LC3_TermInterface tui);

/*
 * Run terminal UI until exited
 */
int LC3_RunTermInterface(LC3_TermInterface *tui);

/*
 * Check whether provided address is displayed in the memory viewer
 * Used so the PC does not get lost suddenly
 */
bool LC3_IsAddrDisplayed(LC3_TermInterface *tui, uint16_t addr);

/*
 * Show specified message in command row of TUI
 * If isError is true, the message will be printed in red
 */
void LC3_ShowMessage(LC3_TermInterface *tui, const char *msg, bool isError);
