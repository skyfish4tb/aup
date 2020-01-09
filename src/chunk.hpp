#pragma once

#include <vector>
#include "value.hpp"

namespace aup
{
    enum Opcode : uint8_t
    {
        OP_RET,
    };

    struct Chunk
    {
        std::vector<uint8_t> code;
        std::vector<uint16_t> lines;
        std::vector<uint16_t> columns;
        ValueArray constants;

        void emit(uint8_t byte, int line, int column);
    };
}
