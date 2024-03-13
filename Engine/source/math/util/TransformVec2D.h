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
#ifndef _TransformVec2D_H_
#define _TransformVec2D_H_
#include "math/util/relationVec.h"

class Box2DF : public RectF
{
private:
   typedef RectF Parent;
public:
   F32 rot;
   Box2DF() : RectF(0.0f,0.0f,1.0f,1.0f), rot(0.0f) {};
   Box2DF(F32 inrot) : RectF(0.0f, 0.0f, 1.0f, 1.0f), rot(inrot) {};
   Box2DF(RectF inrect) : RectF(inrect), rot(0.0f) {};
   Box2DF(RectF inrect, F32 inrot) : RectF(inrect), rot(inrot) {};
   Box2DF(const Point2F& in_rMin, const Point2F& in_rExtent) : RectF(in_rMin, in_rExtent), rot(0.0f) {};
   Box2DF(const Point2F& in_rMin, const Point2F& in_rExtent, F32 inrot) : RectF(in_rMin, in_rExtent), rot(inrot) {};
   Box2DF(const F32 in_left, const F32 in_top, const F32 in_width, const F32 in_height) : RectF(in_left, in_top, in_width, in_height), rot(0.0f) {};
   Box2DF(const F32 in_left, const F32 in_top, const F32 in_width, const F32 in_height, F32 inrot) : RectF(in_left, in_top, in_width, in_height), rot(inrot) {};
   Box2DF& mul(const Box2DF& origin); ///< M * a -> M
   Box2DF& mul(const Box2DF& origin, const Box2DF& local); ///< a * b -> M
   String toString();
   Box2DF fromString(String inString);
};

inline Box2DF& Box2DF::mul(const Box2DF& origin)
{
   // get incomming angle
   float s = mSin(origin.rot);
   float c = mCos(origin.rot);

   // translate point back to origin:
   point -= origin.point;

   // rotate point
   Point2F rotPoint = Point2F(point.x * c - point.y * s, point.x * s + point.y * c);

   // translate point back:
   extent * origin.extent;
   point = rotPoint + origin.point * extent;
   return (*this);
}

inline Box2DF& Box2DF::mul(const Box2DF& origin, const Box2DF& local)
{
   // get incomming angle
   float s = mSin(origin.rot + local.rot);
   float c = mCos(origin.rot + local.rot);

   // translate point back to origin:
   Point2F relPoint = local.point - origin.point;

   // rotate point
   Point2F rotPoint = Point2F(local.point.x * c - local.point.y * s, local.point.x * s + local.point.y * c);

   // translate point back:
   Point2F relScale = local.extent * origin.extent;
   relPoint = rotPoint + local.point* relScale;
   Box2DF newBox = Box2DF(relPoint, relScale, origin.rot * local.rot);
   return (*this);
}

inline String Box2DF::toString()
{
   return String::ToString("%g %g &g &g %g", point.x, point.y, extent.x, extent.y, rot );
}

inline Box2DF Box2DF::fromString(String inString)
{
   Box2DF outVal;
   Vector<String> elements;
   inString.split(" ", elements);
   U32 i = 0;
   outVal.point.x = dAtof(elements[i++].c_str());
   outVal.point.y = dAtof(elements[i++].c_str());
   outVal.extent.y = dAtof(elements[i++].c_str());
   outVal.extent.y = dAtof(elements[i++].c_str());
   outVal.rot = dAtof(elements[i++].c_str());
   return outVal;
}

typedef Constraint<Point2F, F32> Constraint2D; //pos xy + rotation
typedef RelationVec<Box2DF, Point2F, F32> RelationVec2D;

template<> inline String Constraint2D::toString()
{
   String retval = String::ToString("%g %g", mPosRanges[MinPos].x, mPosRanges[MinPos].y);
   retval += String::ToString(" %g %g", mPosRanges[MaxPos].x, mPosRanges[MaxPos].y);
   retval += String::ToString(" %g", mRotRanges[MinRot]);
   retval += String::ToString(" %g", mRotRanges[MaxRot]);
   retval += String::ToString(" %g %g", mPosRanges[MinScale].x, mPosRanges[MinScale].y);
   retval += String::ToString(" %g %g", mPosRanges[MaxScale].x, mPosRanges[MaxScale].y);
   return retval;
};

template<> inline Constraint2D Constraint2D::fromString(String inString)
{
   Constraint2D outVal;
   Vector<String> elements;
   inString.split(" ", elements);

   //check to ensure we have PosDimensions*MaxPosTypes+RotDimensions*MaxRotTypes
   AssertWarn(elements.size() == (3*MaxPosTypes+MaxRotTypes), avar("fromString got %d entries, expected %d", elements.size(), (3*MaxPosTypes+MaxRotTypes)));

   U32 i = 0;
   Box2DF posRange;
   posRange = Box2DF(dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()));
   outVal.setPosRange(posRange.point,posRange.extent);

   F32 rotRange[2];
   rotRange[0] = dAtof(elements[i++].c_str());
   rotRange[1] = dAtof(elements[i++].c_str());
   outVal.setRotRange(rotRange);

   Point2F scaleRange[2];
   scaleRange[0] = Point2F(dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()));
   scaleRange[1] = Point2F(dAtof(elements[i++].c_str()), dAtof(elements[i++].c_str()));
   outVal.setScaleRange(scaleRange);

   return outVal;
};

template<> inline void RelationVec2D::toGLobal()
{
   //first, allocate a copy of local size for global space
   if (mGlobal.size() < mLocal.size())
      mGlobal.setSize(mLocal.size());

   //next, itterate throughout the vector, multiplying matricies down the root/branch chains to shift those to worldspace
   for (U32 id = 0; id < mLocal.size(); id++)
   {
      Box2DF curMat = copyLocal(id);
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

template<> inline void RelationVec2D::setPosition(S32 id, Point2F pos)
{
   mLocal[id].point.x = pos.x;
   mLocal[id].point.y = pos.y;
   mCachedResult = false;
};

template<> inline void RelationVec2D::setRotation(S32 id, F32 rot)
{
   mLocal[id].rot = rot;
   mCachedResult = false;
};

template<> inline void RelationVec2D::setScale(S32 id, Point2F scale)
{
   mLocal[id].extent = scale;
   mCachedResult = false;
};

template<> inline Point2F RelationVec2D::getPosition(S32 id)
{
   return global(id)->point;
};

template<> inline F32 RelationVec2D::getRotation(S32 id)
{
   return global(id)->rot;
};

template<> inline Point2F RelationVec2D::getScale(S32 id)
{
   return global(id)->extent;
};

template<> inline void RelationVec2D::translate(S32 id, Point2F posDelta)
{
   Point2F endPos = mClamp2D(mLocal[id].point + posDelta, getConstraint(id)->getPosRange().min, getConstraint(id)->getPosRange().max);
   mLocal[id].point = endPos;
   mCachedResult = false;
};

template<> inline void RelationVec2D::scale(S32 id, Point2F scaleDelta)
{
   Point2F endcale = mClamp2D(mLocal[id].extent * scaleDelta, getConstraint(id)->getScaleRange().min, getConstraint(id)->getScaleRange().max);
   mLocal[id].extent = endcale;
   mCachedResult = false;
};

template<> inline void RelationVec2D::rotate(S32 id, U32 axis, F32 radianDelta)
{
   F32 endRot = mClampF(mLocal[id].rot + radianDelta, getConstraint(id)->getRotRange().min, getConstraint(id)->getRotRange().max);
   mLocal[id].rot = endRot;
   mCachedResult = false;
};

template<> inline void RelationVec2D::orbit(S32 id, U32 axis, F32 radianDelta)
{
   F32 endRot = mClampF(mLocal[id].rot + radianDelta, getConstraint(id)->getRotRange().min, getConstraint(id)->getRotRange().max);
   Box2DF temp = Box2DF(mLocal[id].point, mLocal[id].extent, endRot);
   mLocal[id].mul(temp);
   mCachedResult = false;
};

class TransformVec2D : public RelationVec2D, public SimObject
{
   typedef SimObject Parent;
public:

   //type specific I/O
   void setTransform(S32 id, Box2DF trans)
   {
      setLocal(id, trans);
      setCached(false);
   };

   void constrain(S32 id)
   {
      translate(id, Point2F(0, 0));
      rotate(id, 0, 0);
      scale(id, Point2F(1, 1));
      setCached(false);
   };

   TransformVec2D() {};
   DECLARE_CONOBJECT(TransformVec2D);
};
#endif
