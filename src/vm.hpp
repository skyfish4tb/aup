#pragma once

#include "value.hpp"
#include "gc.hpp"

#include <cstdint>

#define AUP_MAX_FRAMES  64
#define AUP_MAX_STACK   (UINT8_COUNT * AUP_MAX_FRAMES)

namespace aup
{
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
        void push(Value value);
        Value pop();

    private:
        Value *top;
        Value stack[AUP_MAX_STACK];
        Frame frames[AUP_MAX_FRAMES];
        int frameCount;
        GC *gc;
    };
}
