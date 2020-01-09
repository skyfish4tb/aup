#pragma once

#include <vector>

namespace aup
{
    enum ValueType
    {
        TNIL,
        TBOOL,
        TNUM,
        TCFN,
        TPTR,
        TOBJ
    };

    struct Value
    {
        ValueType type;
        union {
            bool Bool;
            double Num;
            void *Ptr;
            uint64_t raw;
        };

        void print();
        bool equal(const Value& that);

        inline bool isNil() { return type == TNIL; }
        inline bool isBool() { return type == TBOOL; }
        inline bool isBool(bool b) { return (type == TBOOL) && (Bool == b); }
        inline bool isNum() { return type == TNUM; }
        inline bool isPtr() { return type == TPTR; }
    };

    struct ValueArray
    {
        std::vector<Value> values;
        int push(Value value, bool allowDup = true);
    };
}
