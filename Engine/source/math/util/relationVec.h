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

#ifndef _RELATIONVEC_H_
#define _RELATIONVEC_H_

#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

#ifndef _CONSTRAINTS_H_
#include "math/util/constraints.h"
#endif

struct RelationNode
{
   S32 mRoot = -1;
   Vector<S32> mBranch;
   RelationNode(S32 root = -1) : mRoot(root) {};
};

template<typename Transform, typename Dimensions> class RelationVec
{
public:
   //initializers
   RelationVec() {}
   RelationVec(Transform inTransform) {}
   //add/remove
   void push(S32 rootId, Transform inMat)
   {
      mLocal.push_back(inMat);
      mRelation.push_back(RelationNode(rootId));
      if (rootId > -1) mRelation[rootId].mBranch.push_back(mLocal.size() - 1);
      setCached(false); //if we've added to the RelationVec, the cache is no longer valid
   }
   void setLocal(S32 id, Transform to) { mLocal[id] = to; mCachedResult = false; }
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
   //base math
   void setPosition(S32 id, Dimensions pos) { mCachedResult = false; };
   void setRotation(S32 id, Dimensions rot) { mCachedResult = false; };
   void setScale(S32 id, Dimensions trans) { mCachedResult = false; };
   //incremental math
   void translate(S32 id, Dimensions pos) { mCachedResult = false; };
   void rotate(S32 id, U32 axis, F32 radians) { mCachedResult = false; };
   void orbit(S32 id, U32 axis, F32 radians) { mCachedResult = false; };
   void scale(S32 id, Dimensions rot) { mCachedResult = false; };
   //constraints
   Vector<Constraint<Dimensions>>* refConstraints() { return &mConstraints; };
   void setConstraint(S32 id, Constraint<Dimensions> ranges)
   {
      //first, allocate a copy of local size for global space
      if (refConstraints()->size() < refLocal()->size())
         refConstraints()->setSize(refLocal()->size());
      mConstraints[id] = ranges;
      setCached(false);
   };
   Constraint<Dimensions>* getConstraint(S32 id) { return &mConstraints[id]; };
private:
   Vector<Transform> mLocal;
   Vector<Transform> mGlobal;
   Vector<RelationNode> mRelation;
   Vector<Constraint<Dimensions>> mConstraints;
   bool mCachedResult = false;
};
#endif