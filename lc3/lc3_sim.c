#include "lc3_sim.h"
#include "lc3_cmd.h"
#include "lib/va_template.h"
#include "lib/leakcheck/lc.h"

#define free_nn(x) if (x != NULL) { lc_free(x); }
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
vaFreeFunction(StringArray, String, freeStringArray, lc_free(el.ptr), ;, ;)

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
        .memory  = lc_calloc(LC3_MEM_SIZE, sizeof(LC3_MemoryCell)),
        .debug   = newStringArray(),
        .reg     = {.PC = 0x3000},
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

    lc_free(sim.memory);
    freeStringArray(sim.debug);
    freeStateHistory(sim.history);
    freeInputQueue(sim.inputs);
    lc_free(sim.output.ptr);
}


// Get a value from memory
int16_t memRead(LC3_SimInstance *sim, uint16_t addr) {
    sim->reg.MAR = addr;
    sim->reg.MDR = sim->memory[sim->reg.MAR].value;
    return sim->reg.MDR;
}


void memWrite(LC3_SimInstance *sim, uint16_t addr, int16_t val) {
    sim->reg.MAR = addr;
    sim->reg.MDR = val;
    sim->memory[sim->reg.MAR].value = sim->reg.MDR;
}

// Get register value
#define R(n) (sim->reg.reg[(n)])

// Get register index from 3 bits
#define REG(n, i) (((n) >> ((sizeof(uint16_t) * 8) - (i) - 3)) & 0x7)

// Set condition codes based on value of n
#define SET_CC(n) tmp = (int16_t)(n); sim->reg.PSR = (sim->reg.PSR & 0xFFF8) | (((tmp < 0) << 2) | ((tmp == 0) << 1) | (tmp > 0));

// Get memory value
#define MEM(n) sim->memory[(n)].value

// Store memory location in state
#define SAVE_MEM(n) initial.memoryValue = (MEM(initial.memoryLocation = n))

#define _PC (sim->reg.PC)
#define _CC (sim->reg.PSR & 0x7)

// Get memory cell of PC
#define MEM_PC sim->memory[_PC]

#define _ACV() sim->reg.ACV = (sim->reg.PSR & (1 << 15)) && (sim->reg.MAR < 0x3000 || sim->reg.MAR >= 0xFE00)

#define gotoIfElse(condition, T, F) if (condition) { goto T; } else { goto F; }
#define INSTR (sim->reg.IR)


#define interrupt(table, vector)                        \
    sim->reg.Table = table;                             \
    sim->reg.Vector = vector;                           \
    sim->reg.MDR = sim->reg.PSR;                        \
    sim->reg.PSR &= 0x7FFF;                             \
    gotoIfElse(sim->reg.MDR & 0x8000, state45, state37)


// Read a character if possible
static int checkedReadChar(LC3_SimInstance *sim) {
    if (sim->inputs.hd == sim->inputs.tl) {
        return -1;
    }

    R(0) = fetchInput(&sim->inputs);
    return 1;
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


// Simulate TRAP
int fakeTRAP(LC3_SimInstance *sim, uint8_t code) {
    switch (((sim->flags & LC3_SIM_REDIR_TRAP) != 0) * code) {
        case 0x00:  return 0;
        case 0x23:
        case 0x20:  return checkedReadChar(sim);
        case 0x21:  checkedPutChar(sim, R(0));
                    return 1;
        case 0x22:  for (uint16_t r0 = R(0); MEM(r0); checkedPutChar(sim, MEM(r0) & 0xFF), r0++);;
                    return 1;
        case 0x24:  for (uint16_t r0 = R(0); MEM(r0); checkedPutChar(sim, MEM(r0) >> 8), putchar(MEM(r0) & 0xFF), r0++);;
                    return 1;
        case 0x25:  return -1;
        default:    return 0;
    }
}


// Execute instruction at the current PC
void LC3_ExecuteInstruction(LC3_SimInstance *sim) {
    // Pre
    if (sim->flags & LC3_SIM_HALTED) {
        return;
    }

    enum OpCode {
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
        OP_NOOP = 0xD,
        OP_LEA  = 0xE,
        OP_TRAP = 0xF,
    };

    LC3_PrevState initial = {
        .reg            = sim->reg,
        .memoryLocation = 0,
        .memoryValue    = MEM(0),
    };

    int16_t tmp = 0;
    Converter cast = {0};
    goto state18;

    // LD instruction
    state2:
        sim->reg.MAR = _PC + (cast.pc_offset9 = INSTR);
        gotoIfElse((_ACV()), state60, state25);
    
    // ST instruction
    state3:
        sim->reg.MAR = _PC + (cast.pc_offset9 = INSTR);
        gotoIfElse((_ACV()), state60, state23);
    
    // LDR instruction
    state6:
        sim->reg.MAR = (uint16_t)R(REG(INSTR, 7)) + (cast.offset6 = INSTR);
        gotoIfElse((_ACV()), state60, state25);
    
    // STR instruction
    state7:
        sim->reg.MAR = (uint16_t)R(REG(INSTR, 7)) + (cast.offset6 = INSTR);
        gotoIfElse((_ACV()), state60, state23);

    // Start of RTI instruction
    state8:
        sim->reg.MAR = sim->reg.reg[6];
        gotoIfElse(sim->reg.PSR & 0x8000, state44, state36);
    
    // LDI instruction
    state10:
        sim->reg.MAR = _PC + (cast.pc_offset9 = INSTR);
        gotoIfElse((_ACV()), state60, state24);
    
    // STI instruction
    state11:
        sim->reg.MAR = _PC + (cast.pc_offset9 = INSTR);
        gotoIfElse((_ACV()), state60, state29);
    
    // Invalid opcode
    state13:
        interrupt(0x01, 0x01);
    
    // Start of TRAP
    state15:
        if ((tmp = fakeTRAP(sim, sim->reg.IR & 0x00FF))) {
            gotoIfElse(tmp > 0, done, failure);
        }

        sim->reg.PC++;
        interrupt(0x00, (sim->reg.IR & 0x00FF));
    
    state16:
        sim->memory[sim->reg.MAR].value = sim->reg.MDR;
        goto done;

    // Start of instruction cycle
    state18:
        sim->reg.MAR = sim->reg.PC;
        sim->reg.PC++;
        _ACV();
        gotoIfElse(sim->reg.INT, state49, state33);
    
    // End of store instructions
    state23:
        sim->reg.MDR = R(REG(INSTR, 4));
        gotoIfElse((_ACV()), state60, state16);
    
    // Continuation of LDI
    state24:
        sim->reg.MAR = memRead(sim, sim->reg.MAR);
        gotoIfElse((_ACV()), state60, state25);
    
    // End of load instructions
    state25:
        SET_CC(R(REG(INSTR, 4)) = memRead(sim, sim->reg.MAR));
        goto done;

    // Continuation of regular instruction cycle
    state28: {
        sim->reg.MDR = sim->memory[sim->reg.MAR].value;             // 28
        sim->reg.IR = sim->reg.MDR;                                 // 30
        sim->reg.BEN = (((sim->reg.IR >> 9) & _CC) > 0);            // 32
        
        switch (sim->reg.IR >> 12) {
            case OP_BR:     _PC += (sim->reg.BEN) * (cast.pc_offset9 = INSTR);
                            break;
            case OP_ADD:    SET_CC(R(REG(INSTR, 4)) = R(REG(INSTR, 7)) + ((INSTR & 0x0020) ? (cast.imm5 = INSTR) : R(INSTR & 0x0007)));
                            break;
            case OP_LD:     goto state2;
            case OP_ST:     goto state3;
            case OP_JSR:    R(7) = _PC;
                            _PC = (INSTR & 0x0800) ? (_PC + (cast.pc_offset11 = INSTR)) : (R(REG(INSTR, 7)) - 1);
                            break;
            case OP_AND:    SET_CC(R(REG(INSTR, 4)) = R(REG(INSTR, 7)) & ((INSTR & 0x0020) ? (cast.imm5 = INSTR) : R(INSTR & 0x0007)));
                            break;
            case OP_LDR:    goto state6;
            case OP_STR:    goto state7;
            case OP_RTI:    goto state8;
            case OP_NOT:    SET_CC(R(REG(INSTR, 4)) = ~R(REG(INSTR, 7)));
                            break;
            case OP_LDI:    goto state10;
            case OP_STI:    goto state11;
            case OP_JMP:    _PC = R(REG(INSTR, 7));
                            break;
            case OP_NOOP:   goto state13;
            case OP_LEA:    R(REG(INSTR, 4)) = _PC + (cast.pc_offset9 = INSTR);
                            break;
            case OP_TRAP:   goto state15;
        }

        goto done;
    }

    // Continuation of STI
    state29:
        sim->reg.MAR = memRead(sim, sim->reg.MAR);
        gotoIfElse((_ACV()), state60, state23);

    // Access violation check
    state33:
        gotoIfElse(sim->reg.ACV, state60, state28)
    
    // RTI continued
    state36:
        sim->reg.PC = memRead(sim, sim->reg.MAR);
        sim->reg.reg[6]++;
        sim->reg.PSR = memRead(sim, sim->reg.reg[6]);
        sim->reg.reg[6]++;
        gotoIfElse(sim->reg.PSR & 0x8000, state59, done);
    
    // Continuation of interrupt
    state37:
        // Push PSR (37, 41)
        sim->reg.reg[6]--;
        memWrite(sim, sim->reg.reg[6], sim->reg.MDR);
        // Push PC - 1 (43, 52)
        sim->reg.reg[6]--;
        memWrite(sim, sim->reg.reg[6], sim->reg.PC - 1);
        // Go to interrupt (54, 53, 55)
        sim->reg.PC = memRead(sim, ((sim->reg.Table << 8) | sim->reg.Vector));
        goto done;
    
    // Privilege exception (?)
    state44:
        interrupt(0x01, 0x00);
    
    // Save USP, load SSP
    state45:
        sim->reg.Saved_USP = sim->reg.reg[6];
        sim->reg.reg[6] = sim->reg.Saved_SSP;

    // Start of interrupt
    state49:
        // TODO PRIORITY
        interrupt(0x01, sim->reg.INTV);
    
    // Save SSP, load USP
    state59:
        sim->reg.Saved_SSP = sim->reg.reg[6];
        sim->reg.reg[6] = sim->reg.Saved_USP;
        goto done;

    // Access violation
    state60:
        interrupt(0x01, 0x02);

    done:
        sim->counter++;
        addState(&sim->history, initial);
        goto end;

    failure:
        sim->flags |= LC3_SIM_HALTED;
        sim->reg = initial.reg;
        goto end;

    end:
        return;
}


// Run until breakpoint, halt or maxsteps
void LC3_UntilBreakpoint(LC3_SimInstance *sim, int maxSteps) {
    if (sim->flags & LC3_SIM_HALTED || maxSteps == 0) {
        return;
    }

    int i = 0;
    do {
        LC3_ExecuteInstruction(sim);
    } while (!sim->memory[_PC].breakpoint && (++i) < maxSteps);

    sim->flags |= (MEM_PC.breakpoint * LC3_SIM_HALTED);
}
