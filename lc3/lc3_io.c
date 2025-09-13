#include "lc3_io.h"
#include "lc3_util.h"


// Read string from file
static String readString(FILE *fp) {
    String ret = newString();

    for (char c; fread(&c, 1, 1, fp) && c != '\0';) {
        for (const char *str = charString(c); str[0]; addchar(&ret, str[0]), str++);
    }

    return ret;
}


static void printString(FILE *fp, String str) {
    fwrite(str.ptr, sizeof(char), str.sz + 1, fp);
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

            sim->reg.PC = orig;

            for (uint16_t i = orig; i < (orig + count); i++) {
                fread(sim->memory + i, 2, 1, fp);

                if ((flags & LC3_FILE_DBG)) {
                    String debug = readString(fp);

                    if (sim->memory[i].hasDebug) {
                        lc_free(sim->debug.ptr[sim->memory[i].debugIndex].ptr);
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


static bool isEmpty(const LC3_MemoryCell cell) {
    return (cell.value == 0) && (cell.debugIndex == 0) && (cell.hasDebug == 0) && (cell.breakpoint == 0);
}


static void writeMemory(LC3_SimInstance *sim, FILE *fp) {
    LC3_MemoryCell *empty = calloc(1, sizeof(LC3_MemoryCell));
    int chunkCounter = 0;

    for (int i = 0; i < LC3_MEM_SIZE; i++) {
        if (isEmpty(sim->memory[i])) {
            chunkCounter++;
        } else {
            if (chunkCounter > 0) {
                fwrite(empty, sizeof(LC3_MemoryCell), 1, fp);
                fwrite(&chunkCounter, sizeof(int), 1, fp);
                chunkCounter = 0;
            }

            fwrite(&sim->memory[i], sizeof(LC3_MemoryCell), 1, fp);
        }
    }

    if (chunkCounter > 0) {
        fwrite(empty, sizeof(LC3_MemoryCell), 1, fp);
        fwrite(&chunkCounter, sizeof(int), 1, fp);
    }

    free(empty);
    return;
}


static void readMemory(LC3_SimInstance *sim, FILE *fp) {
    LC3_MemoryCell current = {0};
    int chunkCounter = 0;

    for (int i = 0; i < LC3_MEM_SIZE;) {
        fread(&current, sizeof(LC3_MemoryCell), 1, fp);

        if (isEmpty(current)) {
            fread(&chunkCounter, sizeof(int), 1, fp);
            memset(sim->memory + i, 0, chunkCounter * sizeof(LC3_MemoryCell));
            i += chunkCounter;
        } else {
            sim->memory[i] = current;
            i++;
        }
    }

    return;
}


void LC3_SaveSimulatorState(LC3_SimInstance *sim, const char *filename) {
    FILE *fp = fopen(filename, "wb");

    if (fp == NULL) {
        sim->error = "Failed to open file!";
        return;
    }

    // Save some space in case any variables are added
    uint8_t buffer[48] = {0};
    memcpy(buffer, &sim->reg, sizeof(LC3_Registers));

    fwrite(buffer, 1, sizeof(buffer), fp);
    writeMemory(sim, fp);
    fwrite(&sim->debug.sz, sizeof(int), 1, fp);

    for (int i = 0; i < sim->debug.sz; i++) {
        printString(fp, sim->debug.ptr[i]);
    }

    // Other variables
    fwrite(&sim->flags, sizeof(uint32_t), 1, fp);
    fwrite(&sim->counter, sizeof(size_t), 1, fp);
    fwrite(&sim->c2, sizeof(size_t), 1, fp);

    fclose(fp);
}


void LC3_LoadSimulatorState(LC3_SimInstance *sim, const char *filename) {
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL) {
        sim->error = "Failed to open file!";
        return;
    }

    // Read registers
    uint8_t buffer[48] = {0};
    fread(buffer, 1, sizeof(buffer), fp);

    sim->reg = *((LC3_Registers *)buffer);
    readMemory(sim, fp);

    int dsz = 0;
    fread(&dsz, sizeof(int), 1, fp);

    freeStringArray(sim->debug);
    sim->debug = newStringArray();

    for (int i = 0; i < dsz; i++) {
        addString(&sim->debug, readString(fp));
    }

    fread(&sim->flags, sizeof(uint32_t), 1, fp);
    fread(&sim->counter, sizeof(size_t), 1, fp);
    fread(&sim->c2, sizeof(size_t), 1, fp);

    fclose(fp);
}
