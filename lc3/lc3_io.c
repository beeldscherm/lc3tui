#include "lc3_io.h"
#include "lc3_util.h"

#define READ_SAFE(ptr, sz, n, fp, onFail) if (fread(ptr, sz, n, fp) != n) { fclose(fp); onFail;}
#define CHECK(x) if(!(x)) { return 1; }


// Read string from file
static String readString(FILE *fp) {
    String ret = newString();
    char c = 0;
    READ_SAFE(&c, 1, 1, fp, ret.cap = 0; return ret);

    while (c != '\0') {
        for (const char *str = charString(c); str[0]; addchar(&ret, str[0]), str++);
        READ_SAFE(&c, 1, 1, fp, ret.cap = 0; return ret);
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


static void loadExecutableLC3T(LC3_SimInstance *sim, FILE *fp) {
    struct LC3T_MemEntry {
        uint16_t value;
        uint8_t  isOrig;
        uint32_t len;
        String debug;
    } entry;

    uint32_t addr = 0, bytes_read = 7, filesize = 0;
    bool origFound = false;

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);

    // Skip magic number and version string
    fseek(fp, 7, SEEK_SET);

    while (bytes_read < filesize) {
        // Read instructions
        fread(&entry.value,  sizeof(uint16_t), 1, fp);
        fread(&entry.isOrig, sizeof(uint8_t),  1, fp);
        fread(&entry.len,    sizeof(uint32_t), 1, fp);

        if (entry.len > 0) {
            entry.debug.ptr = lc_malloc(entry.len + 1);
            entry.debug.sz  = entry.len;
            entry.debug.cap = entry.len + 1;

            fread(entry.debug.ptr, 1, entry.len, fp);
            entry.debug.ptr[entry.len] = '\0';
        }

        if (entry.isOrig) {
            sim->reg.PC = entry.value;
            addr = entry.value - 1;
            origFound = true;

            if (entry.len > 0) {
                lc_free(entry.debug.ptr);
            }
        } else if (origFound) {
            sim->memory[addr].value = entry.value;

            if (entry.len > 0) {
                sim->memory[addr].hasDebug   = true;
                sim->memory[addr].debugIndex = sim->debug.sz;
                addString(&sim->debug, entry.debug);
            }
        } else {
            if (entry.len > 0) {
                lc_free(entry.debug.ptr);
            }

            sim->error = "unable to determine orig";
            break;
        }

        bytes_read += entry.len + 7;
        addr++;
    }
}


enum LC3_FileType {
    LC3_UnknownFile,
    LC3_MYLC3A_File, // https://github.com/beeldscherm/lc3-assembler .lc3 format
    LC3_LC3TV1_File, // https://github.com/chiragsakhuja/lc3tools .obj format
};


enum LC3_FileType getFileType(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    enum LC3_FileType type = LC3_UnknownFile;

    if (fp == NULL) {
        return type;
    }

    uint8_t magic[8] = {0};
    READ_SAFE(magic, 1, sizeof(magic), fp, return type);
    
    if (memcmp(magic, "LC3\x03", 4) == 0) {
        type = LC3_MYLC3A_File;
    } else if (memcmp(magic, "\x1c\x30\x15\xc0\x01\x01\x01", 7) == 0) {
        type = LC3_LC3TV1_File;
    }

    return type;
}


// Load executable
void LC3_LoadExecutable(LC3_SimInstance *sim, const char *filename) {
    enum LC3_FileType type = getFileType(filename);
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL) {
        sim->error = "failed to open file";
        return;
    }

    switch (type) {
        case LC3_UnknownFile:
            sim->error = "unknown file format";
            break;
        case LC3_MYLC3A_File:
            loadExecutableLC3A(sim, fp);
            break;
        case LC3_LC3TV1_File:
            loadExecutableLC3T(sim, fp);
            break;
        default:
            break;
    }

    fclose(fp);
    return;
}


// Saving/loading simulator state
static bool isEmpty(const LC3_MemoryCell cell) {
    return (cell.value == 0) && (cell.debugIndex == 0) && (cell.hasDebug == 0) && (cell.breakpoint == 0);
}


static void writeMemory(LC3_SimInstance *sim, FILE *fp) {
    LC3_MemoryCell *empty = lc_calloc(1, sizeof(LC3_MemoryCell));
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

    lc_free(empty);
    return;
}


static int readMemory(LC3_SimInstance *sim, FILE *fp) {
    LC3_MemoryCell current = {0};
    int chunkCounter = 0;

    for (int i = 0; i < LC3_MEM_SIZE;) {
        READ_SAFE(&current, sizeof(LC3_MemoryCell), 1, fp, return 1);

        if (isEmpty(current)) {
            READ_SAFE(&chunkCounter, sizeof(int), 1, fp, return 1);
            CHECK(i + chunkCounter < LC3_MEM_SIZE);
            memset(sim->memory + i, 0, chunkCounter * sizeof(LC3_MemoryCell));
            i += chunkCounter;
        } else {
            sim->memory[i] = current;
            i++;
        }
    }

    return 0;
}


int LC3_SaveSimulatorState(LC3_SimInstance *sim, const char *filename) {
    FILE *fp = fopen(filename, "wb");

    if (fp == NULL) {
        sim->error = "Failed to open file!";
        return 1;
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
    return 0;
}


int LC3_LoadSimulatorState(LC3_SimInstance *sim, const char *filename) {
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL) {
        sim->error = "Failed to open file!";
        return 1;
    }

    // Read registers
    uint8_t buffer[48] = {0};
    READ_SAFE(buffer, 1, sizeof(buffer), fp, return 1);

    sim->reg = *((LC3_Registers *)buffer);
    CHECK(readMemory(sim, fp) != 0);

    int dsz = 0;
    READ_SAFE(&dsz, sizeof(int), 1, fp, return 1);

    freeStringArray(sim->debug);
    sim->debug = newStringArray();

    for (int i = 0; i < dsz; i++) {
        String current = readString(fp);
        addString(&sim->debug, current);
        CHECK(current.cap > 0);
    }

    READ_SAFE(&sim->flags, sizeof(uint32_t), 1, fp, return 1);
    READ_SAFE(&sim->counter, sizeof(size_t), 1, fp, return 1);
    READ_SAFE(&sim->c2, sizeof(size_t), 1, fp, return 1);

    fclose(fp);
    return 0;
}
