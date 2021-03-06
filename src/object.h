#ifndef _AUP_OBJECT_H
#define _AUP_OBJECT_H
#pragma once

#include "value.h"
#include "code.h"
#include "table.h"

#define AUP_OBJBASE aupObj obj

struct _aupObj {
    aupObj *next;
    aupOType type : 8;
    unsigned isMarked : 1;
};

struct _aupStr {
    AUP_OBJBASE;
    char *chars;
    int length;
    uint32_t hash;
};

struct _aupFun {
    AUP_OBJBASE;
    aupStr *name;
    aupUpv **upvalues;
    aupChunk chunk;   
    int arity;
    int upvalueCount;
};

struct _aupUpv {
    AUP_OBJBASE;
    aupVal *location;
    aupVal closed;
    struct _aupUpv *next;
};

struct _aupMap {
    AUP_OBJBASE;
    aupTab table;
    aupHash hash;
};

#define AUP_OBJTYPE(v)  (AUP_AS_OBJ(v)->type)
#define AUP_IS_STR(v)   (aup_isObject(v, AUP_TSTR))
#define AUP_IS_FUN(v)   (aup_isObject(v, AUP_TFUN))
#define AUP_IS_MAP(v)   (aup_isObject(v, AUP_TMAP))

#define AUP_AS_STR(v)   ((aupStr *)AUP_AS_OBJ(v))
#define AUP_AS_CSTR(v)  (AS_STR(v)->chars)
#define AUP_AS_FUN(v)   ((aupFun *)AUP_AS_OBJ(v))
#define AUP_AS_MAP(v)   ((aupMap *)AUP_AS_OBJ(v))

static inline bool aup_isObject(aupVal value, aupOType type) {
    return AUP_IS_OBJ(value) && AUP_OBJTYPE(value) == type;
}

void aup_printObject(aupObj *object);
const char *aup_typeofObject(aupObj *object);
void aup_freeObject(aupGC *gc, aupObj *object);

aupStr *aup_takeString(aupVM *vm, char *chars, int length);
aupStr *aup_copyString(aupVM *vm, const char *chars, int length);

aupFun *aup_newFunction(aupVM *vm, aupSrc *source);
void aup_makeClosure(aupFun *function);

aupUpv *aup_newUpvalue(aupVM *vm, aupVal *slot);

aupMap *aup_newMap(aupVM *vm);
void aup_setMap(aupVM *vm, aupMap *map, const char *name, aupVal value);

#endif
