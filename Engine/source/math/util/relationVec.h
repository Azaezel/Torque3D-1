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

#pragma once

#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif

#ifndef _CONSOLEOBJECT_H_
#include "console/simObject.h"
#endif

struct RelationNode
{
   S32 mRoot = -1;
   Vector<S32> mBranch;
   RelationNode(S32 root = -1) : mRoot(root) {};
};

template<class Transform> class RelationVec
{
public:
   //initializers
   RelationVec() {}
   RelationVec(Transform inTransform) {}
   //add/remove
   void push(S32 rootId, MatrixF inMat)
   {
      mLocal.push_back(inMat);
      mRelation.push_back(RelationNode(rootId));
      if (rootId > -1) mRelation[rootId].mBranch.push_back(mLocal.size());
      setCached(false); //if we've added to the RelationVec, the cache is no longer valid
   }
   void setLocal(S32 id, Transform to) { mLocal[id] = to; setCached(false); }
   void setGlobal(S32 id, Transform to) { mGlobal[id] = to; }
   //reference the raw vectors in thier entrieties
   Vector<Transform>* refLocal() { return &mLocal; };
   Vector<Transform>* refGlobal() { return &mGlobal; };
   Vector<RelationNode>* refRelation() { return &mRelation; };
   //get pointers to individual elements
   Transform* local(S32 id) { return &mLocal[id]; };
   Transform* global(S32 id, bool recalc = false) { if (recalc || !isCached()) toGLobal(); return &mGlobal[id]; }
   void toGLobal() { mCachedResult = false; };
   RelationNode* relation(S32 id) { return &mRelation[id]; };
   //for those cases where you *must* take the performance hit and do a copy
   Transform copyLocal(S32 id) { return mLocal[id]; };
   Transform copyGlobal(S32 id) { return mGlobal[id]; };
   void setCached(bool cached) { mCachedResult = cached; };
   bool isCached() { return mCachedResult; };
private:
   Vector<Transform> mLocal;
   Vector<Transform> mGlobal;
   Vector<RelationNode> mRelation;
   bool mCachedResult = false;
};

template<> inline void RelationVec<MatrixF>::toGLobal()
{
   //first, allocate a copy of local size for global space
   if (refGlobal()->size() < refLocal()->size())
      refGlobal()->setSize(refLocal()->size());

   //next, itterate throughout the vector, multiplying matricies down the root/branch chains to shift those to worldspace
   for (U32 id = 0; id < refLocal()->size(); id++)
   {
      MatrixF curMat = copyLocal(id);
      for (U32 branchID = 0; branchID < relation(id)->mBranch.size(); branchID++)
      {
         RelationNode* node = relation(id);
         if (node->mBranch[branchID] >= 0)
         {
            MatrixF childmat = copyLocal(node->mBranch[branchID]);
            childmat.mul(curMat);

         }

      }
      setGlobal(id,curMat);
   }
   setCached(true);
};
class MatrixVec : public RelationVec<MatrixF>, public SimObject
{
   typedef SimObject Parent;
public:

   MatrixVec() {};
   DECLARE_CONOBJECT(MatrixVec);
};
