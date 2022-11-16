#pragma once
#include "game/components/component.h"
#include "console/engineAPI.h"

DefineEngineStringlyVariadicMethod(Component, callMethod, void, 3, 64, "(methodName, argi) Calls script defined method\n"
   "@param methodName The method's name as a string\n"
   "@param argi Any arguments to pass to the method\n"
   "@return No return value"
   "@note %obj.callMethod( %methodName, %arg1, %arg2, ... );\n")
{
   object->callMethodArgList(argc - 1, argv + 2);
}
