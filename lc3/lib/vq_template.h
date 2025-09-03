#pragma once
#include <stdlib.h> // IWYU pragma: keep
#include <string.h> // IWYU pragma: keep
#include "lc.h"

#ifndef VQ_BASE_CAP
#define VQ_BASE_CAP (8)
#endif


// Include in struct defintion to make it work with the functions
#define vqRequiredArgs(type) type *ptr; size_t hd, tl, cap


// Templates for resizeable arrays
#define vqTypedef(type, name) typedef struct name {\
    vqRequiredArgs(type);\
} name


#define vqAllocFunctionDefine(vqType, name) vqType name()
#define vqAllocFunction(vqType, type, name, pre, post) vqType name() {\
    pre;\
    vqType vq = { .ptr = lc_malloc(VQ_BASE_CAP * sizeof(type)), .hd = 0, .tl = 0, .cap = VQ_BASE_CAP };\
    post;\
    return vq;\
}


#define vqAllocCapacityFunctionDefine(vqType, name) vqType name(size_t cap)
#define vqAllocCapacityFunction(vqType, type, name, pre, post) vqType name(size_t cap) {\
    pre;\
    vqType vq = { .ptr = lc_malloc(cap * sizeof(type)), .hd = 0, .tl = 0, .cap = cap };\
    post;\
    return vq;\
}


// Since we have pre and post, we cannot guarantee const for element - user can add it if needed
#define vqEnqueueFunctionDefine(vqType, type, name) void name(vqType *vq, type el)
#define vqEnqueueFunction(vqType, type, name, pre, post) void name(vqType *vq, type el) {\
    pre;\
    vq->ptr[vq->tl] = el;\
    vq->tl = (vq->tl + 1) % vq->cap;\
    if (vq->hd >= vq->tl) {\
        vq->ptr = lc_realloc(vq->ptr, 2 * vq->cap * sizeof(type));\
        memcpy(vq->ptr + vq->cap, vq->ptr, vq->tl * sizeof(type));\
        vq->tl += vq->cap;\
        vq->cap *= 2;\
    }\
    post;\
    return;\
}


#define vqDequeueFunctionDefine(vqType, name) type name(vqType *vq)
#define vqDequeueFunction(vqType, type, name, pre, post) type name(vqType *vq) {\
    pre;\
    type el = vq->ptr[vq->hd];\
    vq->hd = (vq->hd + 1) % vq->cap;\
    post;\
    return el;\
}


#define VQ_SZ(vq)    ((vq.tl >= vq.hd) ? (vq.tl - vq.hd) : (vq.tl + vq.cap - vq.hd))
#define VQ_EL(vq, i) (vq.ptr[(vq.hd + i) % vq.cap])

#define vqSizeFunctionDefine(vqType, name) size_t name(vqType vq)
#define vqSizeFunction(vqType, name, pre, post) size_t name(vqType vq) {\
    pre;\
    size_t ret = VQ_SZ(vq);\
    post;\
    return ret;\
}


#define vqClearFunctionDefine(vqType, name) void name(vqType *vq)
#define vqClearFunction(vqType, name, pre, post) void name(vqType *vq) {\
    pre;\
    vq->hd = 0;\
    vq->tl = 0;\
    post;\
}


#define vqFreeFunctionDefine(vqType, name) void name(vqType vq)
#define vqFreeFunction(vqType, type, name, foreach, pre, post) void name(vqType vq) {\
    pre;\
    for (size_t i = vq.hd; i != vq.tl; i = ((i + 1) % vq.cap)) {\
        type el = vq.ptr[i];\
        foreach;\
        memset(&el, 0, 0);\
    }\
    lc_free(vq.ptr);\
    post;\
    return;\
}
