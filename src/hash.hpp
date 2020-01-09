#pragma once

#include <vector>
#include "value.hpp"

namespace aup
{
    struct Index
    {
        static const uint64_t UNUSED = -1;
        Index() : key(UNUSED), value(Value())  {};
        Index(Value value) : value(value)  {};

        uint64_t key;
        Value value;
    };

    struct Hash
    {
    public:
        bool get(uint64_t key, Value& value);
        bool set(uint64_t key, const Value& value);
    private:
        size_t count;
        size_t capacity;
        std::vector<Index> indexes;

        void growCapacity();
    };
}
