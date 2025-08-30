#include "lc3_sim.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lc3_cmd.h"
#include "lib/va_template.h"

#define free_nn(x) if (x != NULL) { free(x); }
#define LC3_HIST_MAX (8000)
#define LC3_OUT_MAX  (64000)

// This struct is used for working with weird number lengths
typedef struct Converter {
    signed imm5:        5;
    signed offset6:     6;
    signed pc_offset9:  9;
    signed pc_offset11: 11;
} Converter;

// String functions
vaAllocFunction(String, char, newString, ;, va.ptr[0] = '\0')
vaClearFunction(String, clearString, ;, va->ptr[0] = '\0')

vaAppendFunction(String, char, addchar,
    // Tomfoolery to make the null terminator exist
    va->ptr[va->sz] = el;
    va->sz++;
    el = '\0';
    ,
    va->sz--;
)

// String array functions
vaAllocFunction(StringArray, String, newStringArray, ;, ;)
vaAppendFunction(StringArray, String, addString, ;, ;)
vaFreeFunction(StringArray, String, freeStringArray, free(el.ptr), ;, ;)

// History functions
#define keepLast(arr, el_sz, n, foreach) \
    for (int i = 0; i < (n); i++) {\
        foreach;\
    }\
    memmove(arr.ptr, arr.ptr + (arr.sz - (n)), (n) * el_sz);\
    arr.sz = (n)


void cropHistory(LC3_StateHistory *history) {
    keepLast((*history), sizeof(LC3_PrevState), history->sz >> 1,);
}

vaAllocFunction(LC3_StateHistory, LC3_PrevState, newStateHistory, ;, ;)
vaAppendFunction(LC3_StateHistory, LC3_PrevState, addState, if (va->sz >= LC3_HIST_MAX) { cropHistory(va); };, ;)
vaFreeFunction(LC3_StateHistory, LC3_PrevState, freeStateHistory, ;, ;, ;)

// Input queue functions
vqAllocFunction(InputQueue, char, newInputQueue, ;, ;)
vqEnqueueFunction(InputQueue, char, LC3_QueueInput, ;, ;)
vqDequeueFunction(InputQueue, char, fetchInput, ;, ;)
vqFreeFunction(InputQueue, char, freeInputQueue, ;, ;, ;)


LC3_SimInstance LC3_CreateSimInstance() {
    LC3_SimInstance ret = {
        .memory  = calloc(LC3_MEM_SIZE, sizeof(LC3_MemoryCell)),
        .debug   = newStringArray(),
        .regs    = {0},
        .pc      = 0x3000,
        .cc      = 0,
        .flags   = LC3_SIM_REDIR_TRAP | LC3_SIM_HALTED,
        .counter = 0,
        .c2      = 0,
        .error   = NULL,
        .history = newStateHistory(),
        .inputs  = newInputQueue(),
        .output  = newString(),
        .outf    = NULL,
    };

    return ret;
}


void LC3_DestroySimInstance(LC3_SimInstance sim) {
    if (sim.outf) {
        fclose(sim.outf);
    }

    free(sim.memory);
    freeStringArray(sim.debug);
    freeStateHistory(sim.history);
    freeInputQueue(sim.inputs);
    free(sim.output.ptr);
}


// Get register value
#define R(n) (sim->regs[(n)])

// Get register index from 3 bits
#define REG(n, i) (((n) >> ((sizeof(uint16_t) * 8) - (i) - 3)) & 0x7)

// Store register in state
#define SAVE_R(n) initial.regValue = (R(initial.regLocation = n))

// Set condition codes based on value of n
#define CC(n) tmp = (int16_t)(n); sim->cc = ((tmp < 0) << 2) | ((tmp == 0) << 1) | (tmp > 0);

// Get memory value
#define MEM(n) sim->memory[(n)].value

// Store memory location in state
#define SAVE_MEM(n) initial.memoryValue = (MEM(initial.memoryLocation = n))

// Get memory cell of PC
#define MEM_PC sim->memory[sim->pc]


// Read a character if possible
static int checkedReadChar(LC3_SimInstance *sim) {
    if (sim->inputs.hd == sim->inputs.tl) {
        return 1;
    }

    R(0) = fetchInput(&sim->inputs);
    return 0;
}


// Put character into output(s)
static void checkedPutChar(LC3_SimInstance *sim, char c) {
    if (sim->outf != NULL) {
        fputc(c, sim->outf);
    }

    for (const char *cstr = charString(c); cstr && cstr[0]; cstr++) {
        addchar(&sim->output, cstr[0]);
    }

    if (sim->output.sz > LC3_OUT_MAX) {
        keepLast(sim->output, sizeof(char), (LC3_OUT_MAX / 2),);
    }
}


// Execute TRAP code
static int TRAP(LC3_SimInstance *sim, uint16_t instr) {
    instr &= 0x00FF;

    switch (((sim->flags & LC3_SIM_REDIR_TRAP) != 0) * instr) {
        case 0x00: break;
        case 0x23: // printf("Input a character> ");
        case 0x20: return checkedReadChar(sim);
        case 0x21: checkedPutChar(sim, R(0));
                   return 0;
        case 0x22: for (uint16_t r0 = R(0); MEM(r0); checkedPutChar(sim, MEM(r0) & 0xFF), r0++);;
                   return 0;
        case 0x24: for (uint16_t r0 = R(0); MEM(r0); checkedPutChar(sim, MEM(r0) >> 8), putchar(MEM(r0) & 0xFF), r0++);;
                   return 0;
        case 0x25: return 1;
        default:   return 0;
    }
    
    R(7) = sim->pc + 1;
    sim->pc = MEM(instr);
    return 0;
}


// Execute instruction at the current PC
void LC3_ExecuteInstruction(LC3_SimInstance *sim) {
    if (sim->flags & LC3_SIM_HALTED) {
        return;
    }

    uint16_t instr = MEM_PC.value;
    int16_t tmp;
    uint8_t opcode = instr >> 12;
    Converter cast = {0};

    enum OpCodes {
        OP_BR   = 0x0,
        OP_ADD  = 0x1,
        OP_LD   = 0x2,
        OP_ST   = 0x3,
        OP_JSR  = 0x4, // JSR, JSRR
        OP_AND  = 0x5,
        OP_LDR  = 0x6,
        OP_STR  = 0x7,
        OP_RTI  = 0x8,
        OP_NOT  = 0x9,
        OP_LDI  = 0xA,
        OP_STI  = 0xB,
        OP_JMP  = 0xC, // JMP, RET
        OP_LEA  = 0xE,
        OP_TRAP = 0xF,
    };

    LC3_PrevState initial = {
        .pc             = sim->pc,
        .memoryLocation = 0,
        .memoryValue    = MEM(0),
        .regLocation    = 0,
        .regValue       = R(0),
        .cc             = sim->cc,
    };

    // Macros make it look a little better, but not much
    switch (opcode) {
        case OP_ADD:  SAVE_R(REG(instr, 4));
                      CC(R(REG(instr, 4)) = R(REG(instr, 7)) + ((instr & 0x0020) ? (cast.imm5 = instr) : R(instr & 0x0007)));
                      break;
        case OP_AND:  SAVE_R(REG(instr, 4));
                      CC(R(REG(instr, 4)) = R(REG(instr, 7)) & ((instr & 0x0020) ? (cast.imm5 = instr) : R(instr & 0x0007)));
                      break;
        case OP_BR:   sim->pc += (((instr >> 9) & sim->cc) != 0) * (cast.pc_offset9 = instr);
                      break;
        case OP_JMP:  sim->pc = R(REG(instr, 7)) - 1;
                      break;
        case OP_JSR:  SAVE_R(7);
                      R(7) = sim->pc + 1;
                      sim->pc = (instr & 0x0800) ? (sim->pc + (cast.pc_offset11 = instr)) : (R(REG(instr, 7)) - 1);
                      break;
        case OP_LD:   SAVE_R(REG(instr, 4));
                      CC(R(REG(instr, 4)) = MEM(sim->pc + (cast.pc_offset9 = instr) + 1));
                      break;
        case OP_LDI:  SAVE_R(REG(instr, 4));
                      CC(R(REG(instr, 4)) = MEM((uint16_t)MEM(sim->pc + (cast.pc_offset9 = instr) + 1)));
                      break;
        case OP_LDR:  SAVE_R(REG(instr, 4));
                      CC(R(REG(instr, 4)) = MEM((uint16_t)R(REG(instr, 7)) + (cast.offset6 = instr)));
                      break;
        case OP_LEA:  SAVE_R(REG(instr, 4));
                      CC(R(REG(instr, 4)) = sim->pc + (cast.pc_offset9 = instr) + 1);
                      break;
        case OP_NOT:  SAVE_R(REG(instr, 4));
                      CC(R(REG(instr, 4)) = ~R(REG(instr, 7)));
                      break;
        case OP_RTI:  // TODO
                      break;
        case OP_ST:   SAVE_MEM(sim->pc + (cast.pc_offset9 = instr) + 1);
                      MEM(sim->pc + (cast.pc_offset9 = instr) + 1) = R(REG(instr, 4));
                      break;
        case OP_STI:  SAVE_MEM((uint16_t)MEM(sim->pc + (cast.pc_offset9 = instr) + 1));
                      MEM((uint16_t)MEM(sim->pc + (cast.pc_offset9 = instr) + 1)) = R(REG(instr, 4));
                      break;
        case OP_STR:  SAVE_MEM((uint16_t) R(REG(instr, 7)) + (cast.offset6 = instr));
                      MEM((uint16_t) R(REG(instr, 7)) + (cast.offset6 = instr)) = R(REG(instr, 4));
                      break;
        case OP_TRAP: SAVE_R(7);
                      if (TRAP(sim, instr)) {
                          sim->flags |= LC3_SIM_HALTED;
                          return;
                      }
                      break;
        default: break;
    }

    sim->counter++;
    sim->pc++;
    addState(&sim->history, initial);
}


// Run until breakpoint, halt or maxsteps
void LC3_UntilBreakpoint(LC3_SimInstance *sim, int maxSteps) {
    if (sim->flags & LC3_SIM_HALTED || maxSteps == 0) {
        return;
    }

    int i = 0;
    do {
        LC3_ExecuteInstruction(sim);
    } while (!sim->memory[sim->pc].breakpoint && (++i) < maxSteps);

    sim->flags |= (MEM_PC.breakpoint * LC3_SIM_HALTED);
}


// Read string from file
static String readString(FILE *fp) {
    String ret = newString();

    for (char c; fread(&c, 1, 1, fp) && c != '\0';) {
        // These mess up the memory viewer
        if (c == '\t') {
            addchar(&ret, '\\');
            addchar(&ret, 't');
        } else {
            addchar(&ret, c);
        }
    }
    return ret;
}


// Load LC3A executable (.lc3)
static void loadExecutableLC3A(LC3_SimInstance *sim, FILE *fp) {
    enum {
        LC3_FILE_OBJ = 0x0001,
        LC3_FILE_EXC = 0x0002,
        LC3_FILE_DBG = 0x0004,
    };

    char magic[4];
    uint16_t flags;
    uint8_t indicator;

    fread(magic, 1, 4, fp);
    fread(&flags, 2, 1, fp);

    if (memcmp(magic, "LC3\x03", 4) || (flags & LC3_FILE_OBJ)) {
        sim->error = "Can only execute executable files, not objects!";
        return;
    }

    while (fread(&indicator, 1, 1, fp)) {
        if (indicator == 'S') {
            // Why is there a symbol table in my executable?
            uint32_t count;
            uint32_t gobbler;

            fread(&count, 4, 1, fp);

            // Skip skip skip skip
            for (uint32_t i = 0; i < count; i++) {
                fread(&gobbler, 2, 1, fp);
                for (char c; fread(&c, 1, 1, fp) && c != '\0';);
            }
        }

        else if (indicator == 'A') {
            uint16_t orig;
            uint16_t count;

            fread(&orig, 2, 1, fp);
            fread(&count,  2, 1, fp);

            if (((int)orig + count) > UINT16_MAX) {
                sim->error = "instructions out of memory range!";
                return;
            }

            sim->pc = orig;

            for (uint16_t i = orig; i < (orig + count); i++) {
                fread(sim->memory + i, 2, 1, fp);

                if ((flags & LC3_FILE_DBG)) {
                    String debug = readString(fp);

                    if (sim->memory[i].hasDebug) {
                        free(sim->debug.ptr[sim->memory[i].debugIndex].ptr);
                        sim->debug.ptr[sim->memory[i].debugIndex] = debug;
                    } else {
                        sim->memory[i].hasDebug = true;
                        sim->memory[i].debugIndex = sim->debug.sz;
                        addString(&sim->debug, debug);
                    }
                }
            }
        }
    }
}


// Load executable
void LC3_LoadExecutable(LC3_SimInstance *sim, const char *filename) {
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL) {
        sim->error = "Failed to open file!";
        return;
    }

    loadExecutableLC3A(sim, fp);
    fclose(fp);
}
