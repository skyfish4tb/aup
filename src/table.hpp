#pragma once

#include <vector>

#include "aup.h"
#include "value.hpp"

namespace aup
{
    struct Entry
    {
        Str *key;
        Value value;

        Entry() : key(nullptr), value(Value()) {}
    };

    struct Table
    {
    public:
        bool get(Str *key, Value& value);
        bool set(Str *key, const Value& value);
        bool remove(Str *key);
        void addAll(Table& from);
        Str *findString(const char *chars, int length, uint32_t hash);

    private:
        size_t count;
        size_t capacity;
        std::vector<Entry> entries;

        void growCapacity();
    };
}
