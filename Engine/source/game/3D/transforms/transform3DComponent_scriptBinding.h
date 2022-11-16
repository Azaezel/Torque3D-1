#pragma once
#include "transform3DComponent.h"
#include "console/engineAPI.h"

DefineEngineMethod(Transform3DComponentInstance, setPosition, void, (Point3F pos), (Point3F(0,0,0)),
   "Sets the position.")
{
   object->setPosition(pos);
}
