#pragma once
#include "config.h"
#include "lc3_sim.h"


/*
 * Load an LC3 executable into simulator memory
 */
void LC3_LoadExecutable(LC3_SimInstance *sim, const char *filename);
