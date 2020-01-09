#include <cstdio>
#include "value.hpp"

using namespace aup;

void Value::print()
{
    switch (type) {
        case TNIL:
            printf("nil");
            break;
        case TBOOL:
            printf(Bool ? "true" : "false");
            break;
        case TNUM:
            printf("%.14g", Num);
            break;
        case TPTR:
            printf("ptr: %p", Ptr);
            break;
    }
}

bool Value::equal(const Value& that)
{
    if (type != that.type) return false;

    switch (type) {
        case TNIL:
            return true;
        case TBOOL:
            return Bool == that.Bool;
        case TNUM:
            return Num == that.Num;
        case TPTR:
            return Ptr == that.Ptr;
    }

    return false;
}

int ValueArray::push(Value value, bool allowDup)
{
    if (!allowDup) {
        for (int i = 0; i < values.size(); i++) {
            if (value.equal(values[i])) {
                return i;
            }
        }
    }

    values.push_back(value);
    return values.size() - 1;
}
