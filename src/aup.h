#ifndef _AUP_H
#define _AUP_H
#pragma once

#include <cstddef>
#include <cstdint>

#ifndef UINT8_COUNT
#define UINT8_COUNT (UINT8_MAX + 1)
#endif

namespace aup
{
    struct VM;
    struct Value;

    uint32_t hashBytes(const void *bytes, size_t size);
}

#endif
