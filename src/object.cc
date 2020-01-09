#include "object.hpp"
#include "gc.hpp"
#include "vm.hpp"

using namespace aup;

void Obj::print()
{
    switch (type) {
        case TSTR: {
            Str *string = reinterpret_cast<Str *>(this);
            printf("%.*s", string->length, string->chars);
            break;
        }
        case TFUN: {

            break;
        }
        default:
            printf("obj: %p", reinterpret_cast<void *>(this));
            break;
    }
}

Map::Map()
{
}

Map::~Map()
{
}

Fun::Fun()
{
    arity = 0;
    name = nullptr;
}

Fun::~Fun()
{
}

Str::Str(Table *strings, char *chars, int length, uint32_t hash)
{
    this->length = length;
    this->chars = chars;
    this->hash = hash;

    strings->set(this, Value());
}

Str::~Str()
{
    delete[] chars;
}

static Str *allocStr(GC *gc, Table *strings, char *chars, int length, uint32_t hash)
{
    Str *string = reinterpret_cast<Str *>(gc->alloc(TSTR));
    return new (string) Str(strings, chars, length, hash);
}

Map *VM::newMap()
{
    Map *map = reinterpret_cast<Map *>(gc->alloc(TMAP));
    new (map) Map();

    return map;
}

Fun *VM::newFunction()
{
    Fun *function = reinterpret_cast<Fun *>(gc->alloc(TFUN));
    new (function) Fun();

    return function;
}

Str *VM::takeString(char *chars, int length)
{
    uint32_t hash = hashBytes(chars, length);
    Str *interned = strings->findString(chars, length, hash);
    if (interned != NULL) {
        delete[] chars;
        return interned;
    }

    Str *string = reinterpret_cast<Str *>(gc->alloc(TSTR));
    return new (string) Str(strings, chars, length, hash);
}

Str *VM::copyString(const char *chars, int length)
{
    uint32_t hash = hashBytes(chars, length);
    Str *interned = strings->findString(chars, length, hash);
    if (interned != NULL) return interned;

    char *heapChars = new char[(length + 1) * sizeof(char)];
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';

    Str *string = reinterpret_cast<Str *>(gc->alloc(TSTR));
    return new (string) Str(strings, heapChars, length, hash);
}
