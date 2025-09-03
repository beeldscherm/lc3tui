#pragma once
#include "lib/config.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "lib/va_template.h"
#include "lib/vq_template.h"

#ifdef __GNUC__
#define __packed__ __attribute__((packed))
#else
#define __packed__
#endif

// Amount of memory cells in the LC3
#define LC3_MEM_SIZE (65536)

// Variable-length string(array) type
vaTypedef(char, String);
vaTypedef(String, StringArray);

// String functions
vaAllocFunctionDefine(String, newString);
vaAppendFunctionDefine(String, char, addchar);

// String array functions
vaAllocFunctionDefine(StringArray, newStringArray);
vaAppendFunctionDefine(StringArray, String, addString);
vaFreeFunctionDefine(StringArray, freeStringArray);

// Queue for inputs
vqTypedef(char, InputQueue);
vqEnqueueFunctionDefine(InputQueue, char, LC3_QueueInput);


// Representation of a single LC3 memory location
typedef struct __packed__ LC3_MemoryCell {
    int16_t value;              // Contents of the memory location
    uint16_t debugIndex;        // Index for the debug string for this memory location (if hasDebug)
    bool hasDebug   :  1;       // Whether this location has any debug info
    bool breakpoint :  1;       // Whether breakpoint is set
                                // Even got 6 entire bits left in here
} LC3_MemoryCell;


typedef enum LC3_SimFlag {
    LC3_SIM_REDIR_TRAP = 0x01,  // Redirect TRAP codes to C functions
                                // e.g. "getc" instead of "loop until keyboard register is set"
                                // Currently, this option is required for the simulator to work as expected
    LC3_SIM_HALTED     = 0x02,  // Execution is halted, exec functions will do nothing until "unhalted"
} LC3_SimFlag;


// Previous state of the LC3 machine
typedef struct LC3_PrevState {
    uint16_t pc;                // PC value of this state
    uint16_t memoryLocation;    // Which memory location was changed after this state
    int16_t  memoryValue;       // The value of that memory location
    uint8_t  regLocation;       // Which register was changed after this state
    int16_t  regValue;          // The value of that register
    uint8_t  cc;                // Condition codes of this state
} LC3_PrevState;

// List of states
vaTypedef(LC3_PrevState, LC3_StateHistory);


// Simulator state
typedef struct LC3_SimInstance {
    LC3_MemoryCell *memory;     // List of LC3_MEM_SIZE LC3_MemoryCells
    StringArray debug;          // Debug strings
    int16_t regs[8];            // Register values
    uint16_t pc;                // PC
    uint8_t  cc;                // Condition codes
    uint32_t flags;             // Combination of LC3_SimFlags
    size_t counter, c2;         // How many instructions the simulator has executed and a variable for commands
    const char *error;          // Error string (currently nearly unused)
    LC3_StateHistory history;   // Previous states
    InputQueue inputs;          // Input queue
    String output;              // Simulator output
    FILE *outf;                 // File to put output into
} LC3_SimInstance;

/*
 * Allocate and initialize a new sim instance
 * Should be lc_free'd after use with LC3_DestroySimInstance
 */
LC3_SimInstance LC3_CreateSimInstance();

/*
 * Deallocate sim instance
 * Should first have been allocated using LC3_CreateSimInstance
 */
void LC3_DestroySimInstance(LC3_SimInstance sim);

/*
 * Execute a single instruction, unless halted
 */
void LC3_ExecuteInstruction(LC3_SimInstance *sim);

/*
 * Load an LC3 executable into memory
 * Currently, only the LC3A executable format (.lc3) is implemented
 */
void LC3_LoadExecutable(LC3_SimInstance *sim, const char *filename);

/*
 * Run until breakpoint, halted or maximum steps
 * Sets the LC3_SIM_HALTED flag afterwards
 */
void LC3_UntilBreakpoint(LC3_SimInstance *sim, int maxSteps);
