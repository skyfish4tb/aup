#pragma once

#include <cstddef>
#include "object.hpp"

namespace aup
{
    struct GC
    {
    public:
        GC();
        ~GC();
        Obj *alloc(ObjType type);
        void collect();
        void free(Obj *object);
    private:
        size_t allocated;
        size_t nextGC;
        Obj *objects;
    };
}
