#pragma once

#include <vector>
#include "value.hpp"

namespace aup
{
    using namespace std;

    enum Opcode : uint8_t
    {
        OP_RET,
    };

    struct Chunk
    {
        vector<uint8_t> code;
        vector<uint16_t> lines;
        vector<uint16_t> columns;
        ValueArray constants;

        void emit(uint8_t byte, int line, int column);
    };
}
