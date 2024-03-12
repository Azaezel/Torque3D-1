//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------
#include "console/engineAPI.h"
#include "math/mTransform.h"
#include "math/util/TransformVec2D.h"

IMPLEMENT_CONOBJECT(TransformVec2D);
ConsoleDocClass(TransformVec2D, "Relational Vector. 2D space");

DefineEngineMethod(TransformVec2D, push, void, (S32 inRootID, String inTransform), , "")
{
   Box2DF box;
   object->push(inRootID, box.fromString(inTransform));
}

DefineEngineMethod(TransformVec2D, dumpVals, void, (), , "")
{
   for (U32 i = 0; i < object->refLocal()->size(); i++)
   {
      Con::printf("local[%i]: %s", i, object->local(i)->toString().c_str());
      Con::printf("global[%i]: %s", i, object->global(i)->toString().c_str());
      Con::printf("\n");
   }
}

DefineEngineMethod(TransformVec2D, getTransform, String, (S32 id, bool global), (0, false), "")
{
   Box2DF* space = (global) ? object->global(id) : object->local(id);
   return space->toString();
}

DefineEngineMethod(TransformVec2D, getPosition, Point2F, (S32 id, bool global), (0, false), "")
{
   Box2DF* space = (global) ? object->global(id) : object->local(id);
   return space->point;
}

DefineEngineMethod(TransformVec2D, getRotation, F32, (S32 id, bool global), (0, false), "")
{
   Box2DF* space = (global) ? object->global(id) : object->local(id);
   return space->rot;
}

DefineEngineMethod(TransformVec2D, getScale, Point2F, (S32 id, bool global), (0, false), "")
{
   Box2DF* space = (global) ? object->global(id) : object->local(id);
   return space->extent;
}

DefineEngineMethod(TransformVec2D, setConstraint2DString, void, (S32 id, const char* constraintString), (0, ""), "")
{
   Constraint2D constraint;
   object->setConstraint(id, constraint.fromString(constraintString));
}


DefineEngineMethod(TransformVec2D, setConstraint2D, void, (S32 id, Point2F inPosMin, Point2F inPosMax, F32 inRotMin, F32 inRotMax, Point2F inScaleMin, Point2F inScaleMax), , "")
{
   Constraint2D constraint;
   object->setConstraint(id, Constraint2D(inPosMin, inPosMax, inRotMin, inRotMax, inScaleMin, inScaleMax));
}

DefineEngineMethod(TransformVec2D, getConstraint2D, const char*, (S32 id), , "")
{
   return object->getConstraint(id)->toString().c_str();
}
