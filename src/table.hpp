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
    };

    struct Table
    {
        int count;
        int capacity;
        vector<Entry> entries;


    };
}