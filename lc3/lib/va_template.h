#pragma once

// Handle possible custom allocation functions
#if !defined(VA_MALLOC) || !defined(VA_REALLOC) || !defined(VA_FREE)
    #include <stdlib.h> // IWYU pragma: keep

    // Malloc function
    #ifndef VA_MALLOC
    #define VA_MALLOC lc_malloc
    #endif

    // Realloc function
    #ifndef VA_REALLOC
    #define VA_REALLOC lc_realloc
    #endif

    // Free function
    #ifndef VA_FREE
    #define VA_FREE lc_free
    #endif
#endif

// Dismiss "unused variable" warnings
#ifdef VA_USELESS_WARNINGS
#define VA_MAKE_USED(x)
#else
#include <string.h> // IWYU pragma: keep
#define VA_MAKE_USED(x) memset(&x, 0, 0)
#endif

// By default, VA_BASE_CAP elements will be allocated
#ifndef VA_BASE_CAP
#define VA_BASE_CAP (8)
#endif


/*
 * Definition for dynamic array required variables
 * Used inside struct definition to make it work with the other VA functions
 *
 * Macro arguments:
 *      type   | The dynamic array element type
 */
#define vaRequiredArgs(type) type *ptr; size_t sz, cap


/*
 * Type definition for new dynamic array type
 *
 * Macro arguments:
 *      type   | The dynamic array element type
 *      name   | The name of dynamic array struct
 */
#define vaTypedef(type, name) typedef struct name {                                             \
    vaRequiredArgs(type);                                                                       \
} name


/*
 * Definition and implementation of the allocation function
 * This function creates an instance of a predefined dynamic array type
 * By default, VA_BASE_CAP elements are allocated
 *
 * Macro arguments:
 *      vaType | The dynamic array type
 *      type   | The dynamic array element type
 *      name   | The name of the function
 *      pre    | Code that should be executed at the start of this function
 *      post   | Code that should be executed at the end of this function
 *
 * Returns: a newly allocated dynamic array of the specified type
 */
#define vaAllocFunctionDefine(vaType, name) vaType name()
#define vaAllocFunction(vaType, type, name, pre, post) vaType name() {                          \
    pre;                                                                                        \
    vaType va = { .ptr = VA_MALLOC(VA_BASE_CAP * sizeof(type)), .sz = 0, .cap = VA_BASE_CAP };  \
    post;                                                                                       \
    return va;                                                                                  \
}


/*
 * Definition and implementation of the allocation function with a specified capacity
 * This function creates an instance of a predefined dynamic array type
 * The default capacity of the array is specified in the arguments of the generated function
 *
 * Macro arguments:
 *      vaType | The dynamic array type
 *      type   | The dynamic array element type
 *      name   | The name of the function
 *      pre    | Code that should be executed at the start of this function
 *      post   | Code that should be executed at the end of this function
 *
 * Function arguments:
 *      cap    | Capacity to be allocated in new dynamic array
 *
 * Returns: a newly allocated dynamic array of the specified type
 */
#define vaAllocCapacityFunctionDefine(vaType, name) vaType name(size_t cap)
#define vaAllocCapacityFunction(vaType, type, name, pre, post) vaType name(size_t cap) {        \
    pre;                                                                                        \
    vaType va = { .ptr = VA_MALLOC(cap * sizeof(type)), .sz = 0, .cap = cap };                  \
    post;                                                                                       \
    return va;                                                                                  \
}


/*
 * Definition and implementation of the append function
 * This function appends an element to the given dynamic array
 * If the array is full, new memory is allocated before appending
 *
 * Arguments:
 *      vaType | The dynamic array type
 *      type   | The dynamic array element type
 *      name   | The name of the function
 *      pre    | Code that should be executed at the start of this function
 *      post   | Code that should be executed at the end of this function
 *
 * Function arguments:
 *      va     | Dynamic array
 *      el     | Element to add
 */
#define vaAppendFunctionDefine(vaType, type, name) void name(vaType *va, type el)
#define vaAppendFunction(vaType, type, name, pre, post) void name(vaType *va, type el) {        \
    pre;                                                                                        \
    if (va->sz >= va->cap) {                                                                    \
        va->cap *= 2;                                                                           \
        va->ptr = VA_REALLOC(va->ptr, va->cap * sizeof(type));                                  \
    }                                                                                           \
    va->ptr[va->sz] = el;                                                                       \
    va->sz++;                                                                                   \
    post;                                                                                       \
    return;                                                                                     \
}


/*
 * Definition and implementation of the clear function
 * This function deletes all elements from the current array
 * By default, no de/re/allocation is performed
 *
 * Arguments:
 *      vaType | The dynamic array type
 *      type   | The dynamic array element type
 *      name   | The name of the function
 *      foreach| Code to execute on each element (i to access index, el to access element)
 *      pre    | Code that should be executed at the start of this function
 *      post   | Code that should be executed at the end of this function
 *
 * Function arguments:
 *      va     | Dynamic array
 */
#define vaClearFunctionDefine(vaType, name) void name(vaType *va)
#define vaClearFunction(vaType, type, name, foreach, pre, post) void name(vaType *va) {               \
    pre;                                                                                        \
    for (size_t i = 0; i < va->sz; i++) {                                                       \
        type el = va->ptr[i];                                                                   \
        foreach;                                                                                \
        VA_MAKE_USED(el);                                                                       \
    }                                                                                           \
    va->sz = 0;                                                                                 \
    post;                                                                                       \
}


/*
 * Definition and implementation of the free function
 * This function frees the dynamic array
 *
 * Arguments:
 *      vaType | The dynamic array type
 *      name   | The name of the function
 *      foreach| Code to execute on each element (i to access index, el to access element)
 *      pre    | Code that should be executed at the start of this function
 *      post   | Code that should be executed at the end of this function
 *
 * Function arguments:
 *      va     | Dynamic array
 */
#define vaFreeFunctionDefine(vaType, name) void name(vaType va)
#define vaFreeFunction(vaType, type, name, foreach, pre, post) void name(vaType va) {           \
    pre;                                                                                        \
    for (size_t i = 0; i < va.sz; i++) {                                                        \
        type el = va.ptr[i];                                                                    \
        foreach;                                                                                \
        VA_MAKE_USED(el);                                                                       \
    }                                                                                           \
    VA_FREE(va.ptr);                                                                            \
    post;                                                                                       \
    return;                                                                                     \
}
