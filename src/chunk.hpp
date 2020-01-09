#pragma once

#include <vector>

namespace aup
{
    using namespace std;

    enum Opcode : uint8_t
    {
        OP_RET,
    };

    struct Chunk
    {
    public:
        vector<uint8_t> code;
        vector<uint16_t> lines;
        vector<uint16_t> columns;

        void emit(uint8_t byte, int line, int column);
    };
}
