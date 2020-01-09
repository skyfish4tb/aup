#pragma once

#include "value.hpp"

namespace aup
{
    struct {
        uint8_t *ip;
        Fun *function;
        Value *slots;
    } frame_t;

    struct VM
    {
        Value *top;
        Value stack[];
    };
}
