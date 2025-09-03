#pragma once
#include "config.h"
#include "lc3_tui.h"

/*
 * Returns a string representing the inputted character
 * For normal characters, this is just the character
 * For escape charactrs, its string representation is returned (e.g. "\\n")
 * Otherwise, the string "\\?" is returned
 */
const char *charString(int ch);

/*
 * Executes one of the TUI commands
 * Definitions and implementations of these commands are in lc3_cmd.c
 * Might modify tui and tui->sim
 */
void LC3_ExecuteCommand(LC3_TermInterface *tui, const char *cmd);
