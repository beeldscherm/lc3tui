#pragma once
#include "../../config.h"

#if DO_LEAK_CHECK
#include <stdlib.h>

void lc_summary(void);
void *_lc_alloc_internal(size_t size, int reset, const char *file, size_t line);
void *_lc_realloc_internal(void *ptr, size_t size, const char *file, size_t line);
void  _lc_free_internal(void *ptr, const char *file, size_t line);


#define lc_malloc(size)        _lc_alloc_internal((size), 0,             __FILE__, __LINE__)
#define lc_calloc(nmemb, size) _lc_alloc_internal(((nmemb) * (size)), 1, __FILE__, __LINE__)
#define lc_realloc(ptr, size)  _lc_realloc_internal((ptr), (size),       __FILE__, __LINE__)
#define lc_free(ptr)           _lc_free_internal((ptr),                  __FILE__, __LINE__)

#else

#define lc_malloc(size)        malloc((size))
#define lc_calloc(nmemb, size) calloc((nmemb), (size))
#define lc_realloc(ptr, size)  realloc((ptr), (size))
#define lc_free(ptr)           free((ptr))

#endif
