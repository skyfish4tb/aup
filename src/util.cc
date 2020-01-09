#include <cstdio>

#include "aup.h"

using namespace aup;

uint32_t aup::hashBytes(const void *bytes, size_t size)
{
    uint32_t hash = 2166136261u;
    const uint8_t *bs = reinterpret_cast<const uint8_t *>(bytes);

    for (size_t i = 0; i < size; i++) {
        hash ^= bs[i];
        hash *= 16777619;
    }

    return hash;
}
