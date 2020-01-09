#include <cstdlib>
#include "gc.hpp"

using namespace aup;

GC::GC()
{
    allocated = 0;
    nextGC = 0;
}

GC::~GC()
{

}

void *GC::realloc(void *ptr, size_t oldSize, size_t newSize)
{
    allocated += newSize - oldSize;

    if (newSize > oldSize && allocated > nextGC) {
        collect();
    }

    if (newSize == 0) {
        free(ptr);
        return NULL;
    }

    return std::realloc(ptr, newSize);
}

void GC::collect()
{

}
