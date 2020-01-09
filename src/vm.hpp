#pragma once

#include "aup.h"
#include "value.hpp"
#include "object.hpp"
#include "table.hpp"
#include "gc.hpp"

#include <cstdint>

#define AUP_MAX_FRAMES  64
#define AUP_MAX_STACK   (UINT8_COUNT * AUP_MAX_FRAMES)

namespace aup
{
    enum Status : int {
        OK,

    };

    struct Frame {
        uint8_t *ip;
        Fun *function;
        Value *slots;
    };

    struct VM
    {
    public:
        VM();
        ~VM();

        Status doFile(const char *fname);
        Status execute();

        void push(const Value& value);
        Value pop();
       
        Map *newMap();
        Fun *newFunction();
        Str *takeString(char *chars, int length);
        Str *copyString(const char *chars, int length);

    private:
        Value *top;
        Value stack[AUP_MAX_STACK];
        Frame frames[AUP_MAX_FRAMES];
        int frameCount;

        GC *gc;
        Table *strings;
        Table *globals;

        void resetStack();
    };
}
