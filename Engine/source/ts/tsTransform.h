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

#ifndef _TSTRANSFORM_H_
#define _TSTRANSFORM_H_

#ifndef _MMATH_H_
#include "math/mMath.h"
#endif

#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

class Stream;

/// compressed quaternion class
struct Quat16
{
   enum { MAX_VAL = 0x7fff };

   S16 x, y, z, w;

   void read(Stream *);
   void write(Stream *);

   void identity();
   QuatF getQuatF() const;
   QuatF &getQuatF( QuatF * q ) const;
   void set( const QuatF & q );
   S32 operator==( const Quat16 & q ) const;
   S32 operator!=( const Quat16 & q ) const { return !(*this == q); }
};

/// class to handle general scaling case
///
/// transform = rot * scale * inv(rot)
struct TSScale
{
   QuatF            mRotate;
   Point3F          mScale;

   void identity() { mRotate.identity(); mScale.set( 1.0f,1.0f,1.0f ); }
   S32 operator==( const TSScale & other ) const { return mRotate==other.mRotate && mScale==other.mScale; }
};

/// struct for encapsulating ts transform related static functions
struct TSTransform
{
   static Point3F & interpolate(const Point3F & p1, const Point3F & p2, F32 t, Point3F *);
   static F32       interpolate(F32             p1, F32             p2, F32 t);
   static QuatF   & interpolate(const QuatF   & q1, const QuatF   & q2, F32 t, QuatF   *);
   static TSScale & interpolate(const TSScale & s1, const TSScale & s2, F32 t, TSScale *);

   static void      setMatrix(const QuatF     &  q, MatrixF *);
   static void      setMatrix(const QuatF     &  q, const Point3F & p, MatrixF *);

   static void      applyScale(F32 scale, MatrixF *);
   static void      applyScale(const Point3F & scale, MatrixF *);
   static void      applyScale(const TSScale & scale, MatrixF *);
};

inline Point3F & TSTransform::interpolate(const Point3F & p1, const Point3F & p2, F32 t, Point3F * p)
{
   p->x = p1.x + t * (p2.x-p1.x);
   p->y = p1.y + t * (p2.y-p1.y);
   p->z = p1.z + t * (p2.z-p1.z);
   return *p;
}

inline F32 TSTransform::interpolate(F32 p1, F32 p2, F32 t)
{
   return p1 + t*(p2-p1);
}

inline TSScale & TSTransform::interpolate(const TSScale & s1, const TSScale & s2, F32 t, TSScale * s)
{
   TSTransform::interpolate(s1.mRotate,s2.mRotate,t,&s->mRotate);
   TSTransform::interpolate(s1.mScale,s2.mScale,t,&s->mScale);
   return *s;
}

inline void TSTransform::setMatrix( const QuatF & q, const Point3F & p, MatrixF * pDest )
{
   q.setMatrix(pDest);
   pDest->setColumn(3,p);
}

inline void TSTransform::setMatrix( const QuatF & q, MatrixF * pDest )
{
   q.setMatrix(pDest);
}

/// <summary>
/// leaf class for matrix chains
/// </summary>
class MatrixNode
{
public:
   /// <summary>
   /// data
   /// </summary>
   MatrixF* mNodeTransform;
   MatrixF* mWorldTransform;

   /// <summary>
   /// constraints
   /// </summary>
   struct constraint
   {
      Point3F mRotRange[2] = { Point3F(F32_MIN, F32_MIN, F32_MIN), Point3F(F32_MAX, F32_MAX, F32_MAX) };
      Point3F mPosRange[2] = { Point3F(F32_MIN, F32_MIN, F32_MIN), Point3F(F32_MAX, F32_MAX, F32_MAX) };
   } mConstraints;
   /// <summary>
   /// parent matrix ID
   /// </summary>
   U32 mParentID = -1;
   /// <summary>
   /// child matrix IDs
   /// </summary>
   Vector<U32> mBranch;
   MatrixNode(const MatrixF* inMatrix = &MatrixF::Identity, bool relative = false, const MatrixF* basis = &MatrixF::Identity)
   {
      mNodeTransform = new MatrixF(inMatrix->getForwardVector(),inMatrix->getPosition());
      MatrixF mat = *mNodeTransform;
      if (relative)
      {
         mat.mul(*basis);
      }
      mWorldTransform = new MatrixF(mat.getForwardVector(), mat.getPosition());
   };
   MatrixNode(VectorF offsetRot, Point3F offsetPos, bool relative = false, const MatrixF* basis = &MatrixF::Identity)
   {
      mNodeTransform = new MatrixF(offsetRot, offsetPos);
      MatrixF mat = *mNodeTransform;
      if (relative)
      {
         mat.mul(*basis);
      }
      mWorldTransform = new MatrixF(mat.getForwardVector(), mat.getPosition());
   };

   void setConstraints(Point3F rotMin, Point3F rotMax, Point3F posMin = Point3F(F32_MIN, F32_MIN, F32_MIN), Point3F posMax = Point3F(F32_MAX, F32_MAX, F32_MAX))
   {
      mConstraints.mPosRange[0] = rotMin;
      mConstraints.mRotRange[1] = rotMax;

      mConstraints.mPosRange[0] = posMin;
      mConstraints.mPosRange[1] = posMax;
   }

   void constrain()
   {
      Point3F rot = mNodeTransform->getForwardVector();
      Point3F pos = mNodeTransform->getPosition();

      rot.setMax(mConstraints.mPosRange[0]);
      rot.setMin(mConstraints.mPosRange[1]);

      pos.setMax(mConstraints.mPosRange[0]);
      pos.setMin(mConstraints.mPosRange[1]);

      mNodeTransform->set(rot, pos);
   }

   ~MatrixNode()
   {
      delete(mNodeTransform);
      delete(mWorldTransform);
   };
};

class NodeTransforms
{
public:
   NodeTransforms() {};
   ~NodeTransforms() { for (U32 i = 0; i < mMatrixList.size(); i++) { pop_Link(i); } };

   /// <summary>
   /// generate an array of matricies with optional offsets for translation and rotation 
   /// </summary>
   /// <param name="count"></param>
   /// <param name="offsetRot"></param>
   /// <param name="offsetPos"></param>
   /// <param name="basis"></param>
   /// <param name="itterative"></param>
   NodeTransforms(U32 count, Point3F offsetRot, Point3F offsetPos, bool itterative = false, bool relative = false)
   {
      mMatrixList.setSize(count);
      if (itterative)
      {
         push_Link(0, offsetRot, offsetPos, relative);
         for (U32 i = 1; i < count; i++)
            push_Link(i - 1, offsetRot, offsetPos, relative);
      }
      else
      {
         for (U32 i = 0; i < count; i++)
            push_Link(0, offsetRot * i, offsetPos * i, relative);
      }
   };

   /// <summary>
   ///  generate an array of matricies with optional offsets for translation and rotation from a specified vector of points and rots
   /// </summary>
   /// <param name="count"></param>
   /// <param name="offsetRot"></param>
   /// <param name="offsetPos"></param>
   /// <param name="itterative"></param>
   NodeTransforms(U32 count, Vector < Point3F> offsetRot, Vector<Point3F> offsetPos, bool relative = false, bool itterative = false)
   {
      mMatrixList.setSize(count);
      if (itterative)
      {
         push_Link(-1, offsetRot[0], offsetPos[0], relative);
         for (U32 i = 1; i < count; i++)
            push_Link(i - 1, offsetRot[i], offsetPos[i], relative);
      }
      else
      {
         push_Link(0, offsetRot[0], offsetPos[0], relative);
         for (U32 i = 1; i < count; i++)
            push_Link(0, offsetRot[i] * i, offsetPos[i] * i, relative);
      }
   };

   /// <summary>
   /// create a new MatrixLink and register it's associations
   /// </summary>
   /// <param name="parentId"></param>
   /// <param name="offsetRot"></param>
   /// <param name="offsetPos"></param>
   /// <param name="basis"></param>
   void push_Link(S32 parentId, Point3F offsetRot, Point3F offsetPos, bool relative = false)
   {
      MatrixF mat = MatrixF::Identity;
      mat.set(offsetRot, offsetPos);
      if (parentId >= 0)
      {
         if (relative)
            mat = *(GetLinkLocal(parentId));
         else
            mat = *(GetLinkGlobal(parentId));
      }
      MatrixNode* link = new MatrixNode(offsetRot, offsetPos, relative, &mat);
      mMatrixList.push_back(link); //add the new link to the overall vector
      if (parentId > -1)
         mMatrixList[parentId]->mBranch.push_back(mMatrixList.size()); //tell the parent node it has a new child
   };

   void push_Link(S32 parentId, MatrixF inMat, bool relative = false)
   {
      MatrixF mat = inMat;
      if (parentId >= 0)
      {
         if (relative)
            mat = *(GetLinkLocal(parentId));
         else
            mat = *(GetLinkGlobal(parentId));
      }
      MatrixNode* link = new MatrixNode(&inMat, relative, &mat);
      mMatrixList.push_back(link); //add the new link to the overall vector

      if (parentId > -1)
         mMatrixList[parentId]->mBranch.push_back(mMatrixList.size()); //tell the parent node it has a new child
   };

   /// <summary>
   /// delete a MatrixLink and free it's associations
   /// </summary>
   /// <param name="id"></param>
   void pop_Link(S32 id)
   {
      delete(mMatrixList[id]);
      mMatrixList.erase(id);
   };

   /// <summary>
   /// set one link as a child of another
   /// </summary>
   /// <param name="id"></param>
   /// <param name="parentID"></param>
   /// <param name="relative">:use parent orientation?</param>
   void setLinkParent(S32 id, S32 parentID, bool relative)
   {
      mMatrixList[id]->mParentID = parentID;
      if (parentID > -1)
      {
         mMatrixList[parentID]->mBranch.push_back_unique(id);
      }
      if (relative)
      {
         calcLinkGlobal(id);
         for (U32 i = 0; i < mMatrixList[id]->mBranch.size(); i++)
         {
            calcLinkGlobal(mMatrixList[id]->mBranch[i]);
         }
      }
   };

   /// <summary>
   /// translate the location of a given node and all of it's children
   /// </summary>
   /// <param name="id"></param>
   /// <param name="offsetPos"></param>
   /// <param name="basis"></param>
   /// <param name="itterative"></param>
   void TranslateLink(S32 id, Point3F offsetPos, bool relative = false)
   {
      if (relative)
      {
         mMatrixList[id]->mNodeTransform->setPosition(mMatrixList[id]->mNodeTransform->getPosition() + offsetPos);
      }
      else
      {
         mMatrixList[id]->mNodeTransform->setPosition(offsetPos);
      }
      mMatrixList[id]->constrain();
      calcLinkGlobal(id);
      for (U32 i = 0; i < mMatrixList[id]->mBranch.size(); i++)
      {
         calcLinkGlobal(mMatrixList[id]->mBranch[i]);
      }
   };

   /// <summary>
   /// rotate the location of a given node and all of it's children
   /// </summary>
   /// <param name="id"></param>
   /// <param name="offsetRot"></param>
   /// <param name="basis"></param>
   /// <param name="itterative"></param>
   void RotateLink(S32 id, Point3F offsetRot, bool relative = false)
   {
      if (relative)
      {
         mMatrixList[id]->mNodeTransform->set(mMatrixList[id]->mNodeTransform->getForwardVector() + offsetRot, mMatrixList[id]->mNodeTransform->getPosition());
      }
      else
      {
         mMatrixList[id]->mNodeTransform->set(offsetRot, mMatrixList[id]->mNodeTransform->getPosition());
      }

      mMatrixList[id]->constrain();
      calcLinkGlobal(id);
      for (U32 i = 0; i < mMatrixList[id]->mBranch.size(); i++)
      {
         calcLinkGlobal(mMatrixList[id]->mBranch[i]);
      }
   };

   /// <summary>
   /// 
   /// </summary>
   /// <param name="id"></param>
   /// <param name="offsetPos"></param>
   /// <param name="offsetRot"></param>
   /// <param name="basis"></param>
   /// <param name="itterative"></param>
   void OrbitLink(S32 id, Point3F offsetPos, Point3F offsetRot, bool relative = false)
   {
      //todo: orbital mechanics
      mMatrixList[id]->constrain();
      calcLinkGlobal(id);
      for (U32 i = 0; i < mMatrixList[id]->mBranch.size(); i++)
      {
         calcLinkGlobal(mMatrixList[id]->mBranch[i]);
      }
   };

   /// <summary>
   /// get the matrix relative to ne of it's parents
   /// </summary>
   /// <param name="id"></param>
   /// <param name="rootId"></param>
   /// <returns></returns>
   const MatrixF* GetLinkLocal(S32 id)
   {
      if (id < mMatrixList.size())
      {
         return mMatrixList[id]->mNodeTransform;
      }
      else
         return &MatrixF::Identity;
   };

   void setLink(S32 id, Point3F rot, Point3F pos, Point3F scale)
   {
      if (id < mMatrixList.size())
      {
         delete(mMatrixList[id]->mNodeTransform);
         mMatrixList[id]->mNodeTransform = new MatrixF(rot, pos);
         mMatrixList[id]->mNodeTransform->scale(scale);
      }
   };

   /// <summary>
   /// calculate the matrix relative to origin
   /// </summary>
   /// <param name="_identity"></param>
   void calcLinkGlobal(S32 id)
   {
      MatrixF mat = *(mMatrixList[id]->mNodeTransform);
      S32 parentID = mMatrixList[id]->mParentID;
      while (parentID < 0)
      {
         mat.mul(*mMatrixList[parentID]->mNodeTransform);
         parentID = mMatrixList[parentID]->mParentID;
      }
      mMatrixList[id]->mWorldTransform->set(mat.getForwardVector(), mat.getPosition());
   }

   /// <summary>
   /// get the cached matrix relative to origin
   /// </summary>
   /// <param name="id"></param>
   /// <returns></returns>
   const MatrixF* GetLinkGlobal(S32 id)
   {
      if (id < mMatrixList.size())
         return mMatrixList[id]->mWorldTransform;
      else
         return &MatrixF::Identity;
   };

   /// <summary>
   /// derive list from an ArrayObject of the form (rotation, position)
   /// </summary>
   void fromConsole() {};

   /// <summary>
   /// put results into an ArrayObject of the form (rotation, position)
   /// </summary>
   void toConsole() {};

   /// <summary>
   /// explicitly resize mMatrixList
   /// </summary>
   /// <param name="size"></param>
   /// <returns></returns>
   U32 setSize(U32 size) { return mMatrixList.setSize(size); };
   S32 size() { return mMatrixList.size(); };
   Vector<MatrixNode*> mMatrixList;
};

#endif
