// Copyright 2012 Stefan Thomas

#include <node.h>
#include "hayai.hpp"

using namespace v8;
using namespace node;

extern "C" void init(Handle<Object> target)
{
    Hayai::Benchmarker::RunAllTests();
}
