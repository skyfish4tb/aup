#include <cstdarg>
#include <cstdio>

#include "vm.hpp"

using namespace aup;

VM::VM()
{
    gc = new GC();
    strings = new Table();
    globals = new Table();
}

VM::~VM()
{
    delete globals;
    delete strings;
    delete gc;
}

void VM::resetStack()
{
    top = stack;
    frameCount = 0;
}

void VM::push(const Value& value)
{
    *(top++) = value;
}

Value VM::pop()
{
    return *(--top);
}




Status VM::execute()
{

    return OK;
}

