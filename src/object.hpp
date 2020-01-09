#pragma once

#include "aup.h"
#include "value.hpp"
#include "chunk.hpp"

namespace aup
{
    enum ObjectType
    {
        TSTR,
        TFUN,
        TMAP
    };

    struct Object
    {
        ObjectType type;
        struct Obj *next;

        void print();

        inline bool isType(ObjectType type) { return this->type == type; };
        inline bool isStr() { return type == TSTR; };
        inline bool isFun() { return type == TFUN; };
    };

    struct String : public Object
    {
        char *chars;
        int length;
        uint32_t hash;
    };

    struct Function : public Object
    {
        int arity;
        Chunk chunk;
        String *name;
    };
}
