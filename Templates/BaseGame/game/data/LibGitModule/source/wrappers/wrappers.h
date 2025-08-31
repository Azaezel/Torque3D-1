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

#ifndef _GIT_WRAPPERS_H_
#define _GIT_WRAPPERS_H_

#include "git2.h"

#ifndef _ENGINEAPI_H_
#include "console/engineAPI.h"
#endif

#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif

#ifndef _SCRIPTOBJECTS_H_
#include <console/scriptObjects.h>
#endif

//session object

class gitObject;

struct gitProgress
{
   F32 mPercent;
   gitObject* mSessionPtr;
};

class gitObject : public ScriptTickObject
{
   typedef ScriptTickObject Parent;

public:
   enum stages
   {
      fetch,
      checkout,
      stageCount
   };

protected:
   git_repository* mRepo;
   StringTableEntry mRepoDesc;
   git_clone_options mCloneOpts;
   gitProgress mProgress_data[stageCount];
   F32 mCurPercent[stageCount];
   git_fetch_options mFetchOpts;
   git_merge_options mMergeOpts;
   git_checkout_options mCheckoutOpts;
   bool mHasUpdates;

public:
   StringTableEntry mUrl;
   StringTableEntry mLocalPath;
   StringTableEntry mRemoteName;
   StringTableEntry mBranchName;
   
   gitObject();
   bool onAdd() override;
   void onRemove() override;

   void interpolateTick(F32 delta) override {};
   void processTick() override;
   void advanceTime(F32 timeDelta) override {};
   void updateProgress(U32 stage, gitProgress *progress);

   S32 openRepo(StringTableEntry path = NULL, StringTableEntry url = NULL);
   S32 cloneRepo(StringTableEntry path = NULL, StringTableEntry url = NULL);
   bool checkState(StringTableEntry remoteName = NULL, StringTableEntry branchName = NULL);
   void update(StringTableEntry remoteName = NULL, StringTableEntry branchName = NULL);

   void closeRepo();
   static void initPersistFields();
   DECLARE_CONOBJECT(gitObject);

   DECLARE_CALLBACK(void, onProgress, (S32 stage, F32 fetchPct, F32 checkoutPct));
   DECLARE_CALLBACK(void, onStart, (S32 stage));
   DECLARE_CALLBACK(void, onComplete, (S32 stage));
};
#endif
