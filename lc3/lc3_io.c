#include "lc3_io.h"



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

