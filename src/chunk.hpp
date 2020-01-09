#pragma once

#include <vector>
#include "value.hpp"

#define AUP_OPCODES() \
/*        opcodes      args     stack       description */ \
    _CODE(PRINT)   	/* []       [-1, +0]    pop a value from stack */ \
    _CODE(POP)     	/* []       [-1, +0]    pop a value from stack and print it */ \
    _CODE(CALL)    	/* [n]      [-n, +1]    */ \
    _CODE(RET)     	/* []       [-1, +0]    */ \
    _CODE(NIL)     	/* []       [-0, +1]    push nil to stack */ \
    _CODE(TRUE)    	/* []       [-0, +1]    push true to stack */ \
    _CODE(FALSE)   	/* []       [-0, +1]    push false to stack */ \
    _CODE(CONST)   	/* [k]      [-0, +1]    push a constant from (k) to stack */ \
    _CODE(NEG)     	/* []       [-1, +1]    */ \
    _CODE(NOT)     	/* []       [-1, +1]    */ \
    _CODE(LT)      	/* []       [-1, +1]    */ \
    _CODE(LE)      	/* []       [-1, +1]    */ \
    _CODE(EQ)      	/* []       [-1, +1]    */ \
    _CODE(ADD)     	/* []       [-2, +1]    */ \
    _CODE(SUB)     	/* []       [-2, +1]    */ \
    _CODE(MUL)     	/* []       [-2, +1]    */ \
    _CODE(DIV)     	/* []       [-2, +1]    */ \
    _CODE(DEF)     	/* [k]      [-1, +0]    pop a value from stack and define as (k) in global */ \
    _CODE(GLD)     	/* [k]      [-0, +1]    push a from (k) in global to stack */ \
    _CODE(GST)     	/* [k]      [-0, +0]    set a value from stack as (k) in global */ \
    _CODE(JMP)     	/* [s, s]   [-0, +0]    */ \
    _CODE(JMPF)    	/* [s, s]   [-1, +0]    */ \
    _CODE(LD)      	/* [s]      [-0, +1]    */ \
    _CODE(ST)      	/* [s]      [-0, +0]    */ \
    _CODE(MAP)      /* []       [-0, +1]    */ \
    _CODE(GET)      \
    _CODE(SET)      \
    _CODE(GETI)     \
    _CODE(SETI)

namespace aup
{
    enum Opcode : uint8_t
    {
#define _CODE(x) OP_##x,
        AUP_OPCODES()
        OPCODE_COUNT
#undef _CODE
    };

    struct Source
    {
        char *buffer;
        char *fname;
        size_t size;

        Source(const char *file);
        ~Source();
        bool validate();
    };

    struct Chunk
    {
        std::vector<uint8_t> code;
        std::vector<uint16_t> lines;
        std::vector<uint16_t> columns;
        ValueArray constants;
        Source *source;

        Chunk(Source *source) : source(source) {};
        void emit(uint8_t byte, int line, int column);
    };
}
