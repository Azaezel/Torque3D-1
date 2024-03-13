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
#ifndef _TransformVec3D_H_
#define _TransformVec3D_H_
#include "math/util/relationVec.h"

typedef Constraint<Point3F, Point3F> Constraint3D;
typedef RelationVec<MatrixF, Point3F, Point3F> RelationVec3D;

template<> inline String Constraint3D::toString()
{
   String retval = String::ToString("%g %g %g", mPosRanges[MinPos].x, mPosRanges[MinPos].y, mPosRanges[MinPos].z);
   retval += String::ToString(" %g %g %g", mPosRanges[MaxPos].x, mPosRanges[MaxPos].y, mPosRanges[MaxPos].z);
   retval += String::ToString(" %g %g %g", mRotRanges[MinRot].x, mRotRanges[MinRot].y, mRotRanges[MinRot].z);
   retval += String::ToString(" %g %g %g", mRotRanges[MaxRot].x, mRotRanges[MaxRot].y, mRotRanges[MaxRot].z);
   retval += String::ToString(" %g %g %g", mPosRanges[MinScale].x, mPosRanges[MinScale].y, mPosRanges[MinScale].z);
   retval += String::ToString(" %g %g %g", mPosRanges[MaxScale].x, mPosRanges[MaxScale].y, mPosRanges[MaxScale].z);
   return retval;
};

template<> inline Constraint3D Constraint3D::fromString(String inString)
{
   Constraint3D outVal;
   Vector<String> elements;
   inString.split(" ", elements);

   //check to ensure we have PosDimensions*MaxPosTypes+RotDimensions*MaxRotTypes
   AssertWarn(elements.size() ==  (3*MaxPosTypes+3*MaxRotTypes), avar("fromString got %d entries, expected %d", elements.size(), (3*MaxPosTypes+3*MaxRotTypes)));

   U32 i = 0;
   Point3F posRange[2];
   posRange[0] = Point3F(dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()));
   posRange[1] = Point3F(dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()));
   outVal.setPosRange(posRange);

   Point3F rotRange[2];
   rotRange[0] = Point3F(dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()));
   rotRange[1] = Point3F(dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()));
   outVal.setRotRange(rotRange);

   Point3F scaleRange[2];
   scaleRange[0] = Point3F(dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()));
   scaleRange[1] = Point3F(dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()));
   outVal.setScaleRange(scaleRange);

   return outVal;
};


template<> inline void RelationVec3D::toGLobal()
{
   //first, allocate a copy of local size for global space
   if (mGlobal.size() < mLocal.size())
      mGlobal.setSize(mLocal.size());

   //next, itterate throughout the vector, multiplying matricies down the root/branch chains to shift those to worldspace
   for (U32 id = 0; id < mLocal.size(); id++)
   {
      MatrixF curMat = copyLocal(id);
      RelationNode* node = relation(id);

      // multiply transforms...
      if (node->mRoot < 0)
         setGlobal(id, curMat);
      else
      {
         curMat.mul(copyGlobal(node->mRoot), copyLocal(id));
         setGlobal(id, curMat);
      }
   }
   mCachedResult = true;
};

template<> inline void RelationVec3D::setPosition(S32 id, Point3F pos)
{
   mLocal[id].setPosition(pos);
   mCachedResult = false;
};

template<> inline void RelationVec3D::setRotation(S32 id, Point3F rot)
{
   Point3F pos = mLocal[id].getPosition();
   mLocal[id].set(rot, pos);
   mCachedResult = false;
};

template<> inline void RelationVec3D::setScale(S32 id, Point3F scale)
{
   mLocal[id].normalize();
   mLocal[id].scale(scale);
   mCachedResult = false;
};

template<> inline Point3F RelationVec3D::getPosition(S32 id)
{
   return global(id)->getPosition();
};
template<> inline Point3F RelationVec3D::getRotation(S32 id)
{
   return global(id)->getForwardVector();
};
template<> inline Point3F RelationVec3D::getScale(S32 id)
{
   return global(id)->getScale();
};

template<> inline void RelationVec3D::translate(S32 id, Point3F posDelta)
{
   Point3F endPos = mClamp3D(mLocal[id].getPosition() + posDelta, getConstraint(id)->getPosRange().min, getConstraint(id)->getPosRange().max);
   mLocal[id].setPosition(endPos);
   mCachedResult = false;
};

template<> inline void RelationVec3D::scale(S32 id, Point3F scaleDelta)
{
   Point3F endcale = mClamp3D(mLocal[id].getScale() * scaleDelta, getConstraint(id)->getScaleRange().min, getConstraint(id)->getScaleRange().max);
   mLocal[id].normalize();
   mLocal[id].scale(endcale);
   mCachedResult = false;
};

template<> inline void RelationVec3D::rotate(S32 id, U32 axis, F32 radianDelta)
{
   Point3F pos = mLocal[id].getPosition();
   Point3F scale = mLocal[id].getScale();
   VectorF curVec;
   VectorF delta;

   switch (axis)
   {
   case 0:
   {
      curVec = mLocal[id].getRightVector();
      delta = VectorF(radianDelta, 0, 0);
   }break;
   case 1:
   {
      curVec = mLocal[id].getForwardVector();
      delta = VectorF(0, radianDelta, 0);

   }break;
   case 2:
   {
      curVec = mLocal[id].getUpVector();
      delta = VectorF(0, 0, radianDelta);

   }break;
   default://bad axis
      return;
   }
   Point3F endRot = mClamp3D(curVec + delta, getConstraint(id)->getRotRange().min, getConstraint(id)->getRotRange().max);
   mLocal[id].set(endRot, pos);
   mLocal[id].scale(scale);
   mCachedResult = false;
};

template<> inline void RelationVec3D::orbit(S32 id, U32 axis, F32 radianDelta)
{
   VectorF curVec;
   VectorF delta;

   switch (axis)
   {
   case 0:
   {
      curVec = mLocal[id].getRightVector();
      delta = VectorF(radianDelta, 0, 0);
   }break;
   case 1:
   {
      curVec = mLocal[id].getForwardVector();
      delta = VectorF(0, radianDelta, 0);

   }break;
   case 2:
   {
      curVec = mLocal[id].getUpVector();
      delta = VectorF(0, 0, radianDelta);

   }break;
   default://bad axis
      return;
   }
   Point3F endRot = mClamp3D(curVec + delta, getConstraint(id)->getRotRange().min, getConstraint(id)->getRotRange().max);
   MatrixF temp = MatrixF(endRot);
   mLocal[id].mul(temp);
   mCachedResult = false;
};

class TransformVec3D : public RelationVec3D, public SimObject
{
   typedef SimObject Parent;
public:

   //type specific I/O
   void setTransform(S32 id, AngAxisF trans)
   {
      MatrixF mat;
      trans.setMatrix(&mat);
      setLocal(id, mat);
      setCached(false);
   };
   void constrain(S32 id)
   {
      translate(id, Point3F(0, 0, 0));
      rotate(id,0,0);
      scale(id, Point3F(1, 1, 1));
      setCached(false);
   };

   TransformVec3D() {};
   DECLARE_CONOBJECT(TransformVec3D);
};
#endif
