#include "lc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


typedef struct lc_block *lc_block_ptr;


typedef struct lc_block {
    size_t size;
    const char *file;
    size_t line;
    lc_block_ptr prev, next;
} lc_block;


static lc_block_ptr head = NULL;
static size_t allocs = 0, frees = 0;
static size_t current = 0, max = 0, total = 0;


void lc_summary(void) {
    printf("Leak summary: (%ld allocs, %ld frees, %ld bytes max, %ld bytes total)\n", allocs, frees, max, total);

    if (!head) {
        printf("\tNo leaks detected.\n");
        return;
    }

    while (head) {
        printf("\t%ld bytes at (%p), allocated at %s:%ld\n", head->size, (void *)(head + 1), head->file, head->line);
        head = head->prev;
    }
}


void *_lc_alloc_internal(size_t size, int reset, const char *file, size_t line) {
    lc_block_ptr bl = malloc(size + sizeof(lc_block));

    if (reset) {
        memset(bl + 1, 0, size);
    }

    bl->size = size;
    bl->file = file;
    bl->line = line;
    bl->prev = head;
    bl->next = NULL;

    if (head) {
        head->next = bl;
    }

    head = bl;

    allocs++;
    total += size;
    current += size;
    max = (current > max) ? current : max;

    return (bl + 1);
}



void _lc_free_internal(void *ptr, const char *file, size_t line) {
    lc_block_ptr bl = ((lc_block_ptr)ptr) - 1;

    if (bl->next) {
        bl->next->prev = bl->prev;
    } else {
        // No next, so bl is head
        head = bl->prev;
    }

    if (bl->prev) {
        bl->prev->next = bl->next;
    }

    frees++;
    current -= bl->size;

    free(bl);
}



void *_lc_realloc_internal(void *ptr, size_t size, const char *file, size_t line) {
    lc_block_ptr bl = ((lc_block_ptr)ptr) - 1;

    if (bl->size >= size) {
        return ptr;
    }

    void *ret = _lc_alloc_internal(size, 0, file, line);
    memcpy(ret, ptr, bl->size);
    _lc_free_internal(ptr, file, line);

    return ret;
}
