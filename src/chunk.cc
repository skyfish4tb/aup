#include "chunk.hpp"

using namespace aup;

void Chunk::emit(uint8_t byte, int line, int column)
{
    code.push_back(byte);
    lines.push_back(line);
    columns.push_back(column);
}
