#pragma once

#include <vector>

#include "aup.h"
#include "value.hpp"
#include "object.hpp"

namespace aup
{
    struct Entry
    {
        String *key;
        Value value;

        Entry() : key(nullptr), value(Value()) {}
    };

    struct Table
    {
    public:
        bool get(String *key, Value& value);
        bool set(String *key, const Value& value);
        bool remove(String *key);
        void addAll(Table& from);
        String *findString(const char *chars, int length, uint32_t hash);

    private:
        size_t count;
        size_t capacity;
        vector<Entry> entries;

        void growCapacity();
    };
}