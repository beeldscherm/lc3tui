#pragma once
#include "lc3_sim.h"


/*
 * Load an LC3 executable into simulator memory
 */
void LC3_LoadExecutable(LC3_SimInstance *sim, const char *filename);

/*
 * Save simulator state
 */
void LC3_SaveSimulatorState(LC3_SimInstance *sim, const char *filename);

/*
 * Load simulator state
 */
void LC3_LoadSimulatorState(LC3_SimInstance *sim, const char *filename);
