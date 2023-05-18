#include "math/util/relationVec.h"
#include "console/simBase.h"
#include "console/engineAPI.h"

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

DefineEngineMethod(MatrixVec, push, void, (S32 inRootID, MatrixF inMat),,"")
{
   object->push(inRootID, inMat);
}

DefineEngineMethod(MatrixVec, dumpVals, void,(), , "")
{
   for (U32 i = 0; i < object->refLocal()->size(); i++)
   {
      object->local(i)->dumpMatrix((String("mat[") + String::ToString(i)+String("]")).c_str());
   }
}

DefineEngineMethod(MatrixVec, getTranform, MatrixF, (S32 id, bool global), (0, false), "")
{
   MatrixF* space = (global) ? object->global(id) : object->local(id);
   return *space;
}

DefineEngineMethod(MatrixVec, getPosition, Point3F, (S32 id, bool global), (0, false), "")
{
   MatrixF* space = (global) ? object->global(id) : object->local(id);
   return space->getPosition();
}

DefineEngineMethod(MatrixVec, getRotation, Point3F, (S32 id, bool global), (0, false), "")
{
   MatrixF* space = (global) ? object->global(id) : object->local(id);
   return space->getForwardVector();
}
