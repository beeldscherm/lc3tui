#pragma once
#include <stdlib.h> // IWYU pragma: keep

#ifndef VA_BASE_CAP
#define VA_BASE_CAP (8)
#endif

#ifndef VA_NO_CC_ERR
#define VA_NO_CC_ERR (1)
#endif


// Include in struct defintion to make it work with the functions
#define vaRequiredArgs(type) type *ptr; size_t sz, cap


// Templates for resizeable arrays
#define vaTypedef(type, name) typedef struct name {\
    vaRequiredArgs(type);\
} name


#define vaAllocFunctionDefine(vaType, name) vaType name()
#define vaAllocFunction(vaType, type, name, pre, post) vaType name() {\
    pre;\
    vaType va = { .ptr = malloc(VA_BASE_CAP * sizeof(type)), .sz = 0, .cap = VA_BASE_CAP };\
    post;\
    return va;\
}


#define vaAllocCapacityFunctionDefine(vaType, name) vaType name(size_t cap)
#define vaAllocCapacityFunction(vaType, type, name, pre, post) vaType name(size_t cap) {\
    pre;\
    vaType va = { .ptr = malloc(cap * sizeof(type)), .sz = 0, .cap = cap };\
    post;\
    return va;\
}


// Since we have pre and post, we cannot guarantee const for element - user can add it if needed
#define vaAppendFunctionDefine(vaType, type, name) void name(vaType *va, type el)
#define vaAppendFunction(vaType, type, name, pre, post) void name(vaType *va, type el) {\
    pre;\
    if (va->sz >= va->cap) {\
        va->cap *= 2;\
        va->ptr = realloc(va->ptr, va->cap * sizeof(type));\
    }\
    va->ptr[va->sz] = el;\
    va->sz++;\
    post;\
    return;\
}


#define vaClearFunctionDefine(vaType, name) void name(vaType *va)
#define vaClearFunction(vaType, name, pre, post) void name(vaType *va) {\
    pre;\
    va->sz = 0;\
    post;\
}


#define vaFreeFunctionDefine(vaType, name) void name(vaType va)
#define vaFreeFunction(vaType, type, name, foreach, pre, post) void name(vaType va) {\
    pre;\
    for (size_t i = 0; i < va.sz; i++) {\
        type el = va.ptr[i];\
        foreach;\
        memset(&el, 0, 0);\
    }\
    free(va.ptr);\
    post;\
    return;\
}
