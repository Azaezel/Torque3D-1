#include "math/util/relationVec.h"
#include "console/simBase.h"
#include "console/engineAPI.h"
#include "math/mAngAxis.h"
#include "math/mTransform.h"

IMPLEMENT_CONOBJECT(MatrixVec);
ConsoleDocClass(MatrixVec,"Matrix Relational Vector");

void testMatrixVec()
{
   RelationVec<MatrixF> tMat;
   tMat.push(-1, MatrixF());
   MatrixF* localMatPtr = tMat.local(0);
   localMatPtr->add(MatrixF::Identity);
   MatrixF* globalMatPtr = tMat.global(0);
   //expected: global is an identity matrix
   localMatPtr->dumpMatrix("local");
   globalMatPtr->dumpMatrix("global");

   localMatPtr->setPosition(Point3F(10, 0, 0));
   tMat.toGLobal();
   localMatPtr->dumpMatrix("local");
   globalMatPtr->dumpMatrix("global");

}


DefineEngineFunction(testMatrixVec, void, (),,
   "testMatrixVec\n"
   "@ingroup Math")
{
   testMatrixVec();
}

DefineEngineMethod(MatrixVec, push, void, (S32 inRootID, TransformF inTransform),,"")
{
   object->push(inRootID, inTransform.getMatrix());
}

DefineEngineMethod(MatrixVec, dumpVals, void,(), , "")
{
   for (U32 i = 0; i < object->refLocal()->size(); i++)
   {
      object->local(i)->dumpMatrix((String("matL[") + String::ToString(i)+String("]")).c_str());
      object->global(i)->dumpMatrix((String("matG[") + String::ToString(i) + String("]")).c_str());
      Con::printf("\n");
   }
}

DefineEngineMethod(MatrixVec, getTransform, TransformF, (S32 id, bool global), (0, false), "")
{
   MatrixF* space = (global) ? object->global(id) : object->local(id);
   return TransformF(*space);
}

DefineEngineMethod(MatrixVec, getPosition, Point3F, (S32 id, bool global), (0, false), "")
{
   MatrixF* space = (global) ? object->global(id) : object->local(id);
   return space->getPosition();
}

DefineEngineMethod(MatrixVec, getRotation, AngAxisF, (S32 id, bool global), (0, false), "")
{
   MatrixF* space = (global) ? object->global(id) : object->local(id);
   AngAxisF aa(*space);
   aa.axis.normalize();
   return aa;
}

DefineEngineMethod(MatrixVec, getEuler, EulerF, (S32 id, bool global), (0, false), "")
{
   MatrixF* space = (global) ? object->global(id) : object->local(id);
   return space->toEuler();
}
