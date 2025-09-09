#pragma once
#include "config.h"
#include "lc3_tui.h"

/*
 * Executes one of the TUI commands
 * Definitions and implementations of these commands are in lc3_cmd.c
 * Might modify tui and tui->sim
 */
void LC3_ExecuteCommand(LC3_TermInterface *tui, const char *cmd);
