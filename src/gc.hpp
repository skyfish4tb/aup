#pragma once

#include <cstddef>

namespace aup
{
    struct GC
    {
        size_t allocated;
        size_t nextGC;

        GC();
        ~GC();
        void *realloc(void *ptr, size_t oldSize, size_t newSize);
        void collect();
    };
}
