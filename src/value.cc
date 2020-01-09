#include <cstdio>

#include "value.hpp"
#include "object.hpp"

using namespace aup;

void Value::print()
{
    switch (type) {
        default:
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
        case TOBJ:
            Obj->print();
            break;
    }
}

bool Value::equal(const Value& that)
{
    if (type != that.type) return false;

    switch (type) {
        default:
        case TNIL:
            return true;
        case TBOOL:
            return Bool == that.Bool;
        case TNUM:
            return Num == that.Num;
        case TPTR:
            return Ptr == that.Ptr;
    }
}

int ValueArray::push(Value value, bool allowDup)
{
    if (!allowDup) {
        for (int i = 0; i < (int)values.size(); i++) {
            if (value.equal(values[i])) {
                return i;
            }
        }
    }

    values.push_back(value);
    return values.size() - 1;
}
