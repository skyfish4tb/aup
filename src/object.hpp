#pragma once

#include "aup.h"
#include "value.hpp"
#include "chunk.hpp"
#include "table.hpp"
#include "hash.hpp"

namespace aup
{
    enum ObjType
    {
        TSTR,
        TFUN,
        TMAP
    };

    struct Obj
    {
        ObjType type;
        Obj *next;

        void print();
        inline bool isType(ObjType type) { return this->type == type; };
        inline bool isStr() { return type == TSTR; };
        inline bool isFun() { return type == TFUN; };
        inline bool isMap() { return type == TMAP; };
    };

    struct Str : public Obj
    {
        Str(Table *strings, char *chars, int length, uint32_t hash);
        ~Str();

        char *chars;
        int length;
        uint32_t hash;
    };

    struct Fun : public Obj
    {
        Fun(Source *source);
        ~Fun();

        int arity;
        Chunk chunk;
        Str *name;
    };

    struct Map : public Obj
    {
        Map();
        ~Map();

        Hash hash;
        Table table;
    };
}
