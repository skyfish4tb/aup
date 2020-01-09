#include <cstdlib>
#include "gc.hpp"

using namespace aup;

GC::GC()
{
    allocated = 0;
    nextGC = 0;
    objects = nullptr;
}

GC::~GC()
{

}

Obj *GC::alloc(ObjType type)
{
    size_t size = 0;

    switch (type) {
        case TSTR:  size = sizeof(Str); break;
        case TFUN:  size = sizeof(Fun); break;
        case TMAP:  size = sizeof(Map); break;
    }

    allocated += size;
    if (allocated > nextGC) collect();

    Obj *object = reinterpret_cast<Obj *>(new char[size]);
    object->type = type;
    object->next = objects;
    return objects = object;
}

void GC::free(Obj *object)
{
    switch (object->type) {
        case TSTR: {
            delete reinterpret_cast<Str *>(object);
            allocated -= sizeof(Str);
            break;
        }
        case TFUN: {
            delete reinterpret_cast<Fun *>(object);
            allocated -= sizeof(Fun);
            break;
        }
        case TMAP: {
            delete reinterpret_cast<Map *>(object);
            allocated -= sizeof(Map);
            break;
        }
    }
}

void GC::collect()
{
    
}
