#include <cstdio>

#include "object.hpp"
#include "gc.hpp"

using namespace aup;

void Object::print()
{
    switch (type) {
        case TSTR: {
            String *string = reinterpret_cast<String *>(this);
            printf("%.*s", string->length, string->chars);
            break;
        }
        case TFUN: {

            break;
        }
    }
}