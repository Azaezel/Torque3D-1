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

#include "wrappers.h"
#include "core/util/path.h"
bool gGitRunning = false;
//general subsystem
DefineEngineFunction(git_init, String, (), ,
        "@brief initialize libGit2.\n\n")
{
   if (gGitRunning) return  "Error git_init already called";
   S32 error = git_libgit2_init();
	if (error < 0) {
		const git_error *e = git_error_last();
		return String::ToString("Error %d/%d: %s\n", error, e->klass, e->message);
	}
   gGitRunning = true;
	return "";
}

DefineEngineFunction(git_shutdown, String, (), ,
        "@brief Logs a message to the console.\n\n"
        "@param message The message text.\n"
        "@note By default, messages will appear white in the console.\n"
        "@ingroup Logging")
{
   if (!gGitRunning) return  "Error git_shutdown already called";
   S32 error = git_libgit2_shutdown();
	if (error < 0) {
		const git_error *e = git_error_last();
		return String::ToString("Error %d/%d: %s\n", error, e->klass, e->message);
	}
   gGitRunning = false;
	return "";
}

S32 fetch_progress(
   const git_indexer_progress* stats,
   void* payload)
{
   gitProgress* pd = (gitProgress*)payload;

   if (stats->total_objects > 0)
      pd->mPercent = stats->received_objects / stats->total_objects;
   else
      pd->mPercent = 1.0f;

   //Con::warnf("fetch_progress %d/%d", stats->received_objects, stats->total_objects);
   if (pd->mSessionPtr)
      pd->mSessionPtr->updateProgress(gitObject::fetch, pd);

   return 0;
}

void checkout_progress(
   StringTableEntry path,
   size_t cur,
   size_t tot,
   void* payload)
{
   //Con::warnf("checkout_progress %d/%d", cur, tot);
   gitProgress* pd = (gitProgress*)payload;

   if (tot > 0)
      pd->mPercent = cur / tot;
   else
      pd->mPercent = 1.0f;

   if (pd->mSessionPtr)
      pd->mSessionPtr->updateProgress(gitObject::checkout, pd);
}

//session object
IMPLEMENT_CONOBJECT(gitObject);

IMPLEMENT_CALLBACK(gitObject, onProgress, void, (S32 stage, F32 fetchPct, F32 checkoutPct), (stage, fetchPct, checkoutPct),
   "Called every 32ms on the control.");
IMPLEMENT_CALLBACK(gitObject, onStart, void, (S32 stage), (stage),
   "Called when the control starts to scroll.");
IMPLEMENT_CALLBACK(gitObject, onComplete, void, (S32 stage), (stage),
   "Called when the child control has been scrolled in entirety.");

gitObject::gitObject()
   : mRepo(NULL),
   mUrl(StringTable->EmptyString()),
   mLocalPath(StringTable->EmptyString()),
   mRepoDesc(StringTable->EmptyString()),
   mCloneOpts(GIT_CLONE_OPTIONS_INIT),
   mFetchOpts(GIT_FETCH_OPTIONS_INIT),
   mMergeOpts(GIT_MERGE_OPTIONS_INIT),
   mCheckoutOpts(GIT_CHECKOUT_OPTIONS_INIT),
   mHasUpdates(false)
{
   mCallOnAdvanceTime = false;
   for (U32 stage = 0; stage < stageCount; stage++)
   {
      mCurPercent[stage] = 0;
      mProgress_data[stage].mPercent = 0;
      mProgress_data[stage].mSessionPtr = this;
   }

   mFetchOpts.callbacks.transfer_progress = fetch_progress;
   mFetchOpts.callbacks.payload = &mProgress_data[fetch];

   mCheckoutOpts.checkout_strategy = GIT_CHECKOUT_SAFE;
   mCheckoutOpts.progress_cb = checkout_progress;
   mCheckoutOpts.progress_payload = &mProgress_data[checkout];

   mCloneOpts.checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
   mCloneOpts.checkout_opts.progress_cb = checkout_progress;
   mCloneOpts.checkout_opts.progress_payload = &mProgress_data[checkout];
   mCloneOpts.fetch_opts.callbacks.transfer_progress = fetch_progress;
   mCloneOpts.fetch_opts.callbacks.payload = &mProgress_data[fetch];

}

bool gitObject::onAdd()
{
   if (!Parent::onAdd())
      return false;

   mRepo = NULL;
   for (U32 stage = 0; stage < stageCount; stage++)
   {
      mCurPercent[stage] = 0;
      mProgress_data[stage].mPercent = 0;
      mProgress_data[stage].mSessionPtr = this;
   }
   setProcessTicks(false);
   return true;
}

void gitObject::onRemove()
{
   closeRepo();
   Parent::onRemove();
}

void gitObject::processTick()
{
   Parent::processTick();

   bool allDone = true;
   for (U32 stage = 0; stage < stageCount; stage++)
   {
      if (mCurPercent[stage] != mProgress_data[stage].mPercent)
      {
         if (mProgress_data[stage].mPercent >= 1.0f)
         {
            onComplete_callback(stage);
         }
         else if (mCurPercent[stage] == 0)
         {
            onStart_callback(stage);
         }
         else
         {
            onProgress_callback(stage, mProgress_data[fetch].mPercent, mProgress_data[checkout].mPercent);
         }

         mCurPercent[stage] = mProgress_data[stage].mPercent;
      }

      if (mProgress_data[stage].mPercent < 1.0f) {
         allDone = false;
      }
   }

   if (allDone) {
      setProcessTicks(false);
   }
}

S32 gitObject::openRepo(StringTableEntry path, StringTableEntry url)
{
   if (!gGitRunning) return GIT_ERROR_INVALID;

   closeRepo();
   StringTableEntry path_to_use = path ? path : mLocalPath;

   // First, try to open the repository.
   int errCode = git_repository_open(&mRepo, path_to_use);
   if (errCode == 0) {
      mLocalPath = path_to_use;
      mUrl = url;
      // The repository is already open, and 'origin' is likely set by the clone.
      return 0;
   }

   // If opening failed because it's not a repository, try to initialize it.
   if (errCode == GIT_ENOTFOUND) {
      git_repository_init_options opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
      opts.flags |= GIT_REPOSITORY_INIT_MKPATH;
      opts.origin_url = url;
      errCode = git_repository_init_ext(&mRepo, path_to_use, &opts);
      if (errCode == 0) {
         mLocalPath = path_to_use;
         mUrl = url;
         return 0;
      }
   }

   // Handle any other errors.
   Con::errorf("Git: Failed to open or initialize repository at '%s'. Error: %s", path_to_use, git_error_last()->message);
   return errCode;
}

S32 gitObject::cloneRepo(StringTableEntry path, StringTableEntry url)
{
   if (!gGitRunning) return GIT_ERROR_INVALID;
   mProgress_data[fetch] = {NULL};
   mProgress_data[checkout] = { NULL };

   setProcessTicks(true);
   S32 errCode = git_clone(&mRepo, url, path, &mCloneOpts);
   return errCode;
}

void gitObject::updateProgress(U32 stage, gitProgress* progress)
{
   mProgress_data[stage].mPercent = progress->mPercent;
}

bool gitObject::checkState(StringTableEntry remoteName, StringTableEntry branchName)
{
   if (!gGitRunning || !mRepo)
   {
      Con::errorf("Git: Cannot perform check. Git not ready or repository not open.");
      return false;
   }

   git_remote* remote = nullptr;
   StringTableEntry remoteToUse = remoteName ? remoteName : mRemoteName;

   if (!mRepo || !remoteToUse || git_remote_lookup(&remote, mRepo, remoteToUse) < 0)
   {
      Con::errorf("Git: Repository or remote not valid for fetch.");
      return false;
   }

   if (git_remote_fetch(remote, nullptr, &mFetchOpts, nullptr) < 0)
   {
      Con::errorf("Git: Failed to fetch remote '%s': %s", remoteToUse, git_error_last()->message);
      git_remote_free(remote);
      return false;
   }

   git_annotated_commit* theirHead = nullptr;
   git_reference* remoteRef = nullptr;
   mHasUpdates = false;

   StringTableEntry branchToUse = branchName ? branchName : mBranchName;
   char remoteBranchRef[256];
   dSprintf(remoteBranchRef, sizeof(remoteBranchRef), "refs/remotes/%s/%s", remoteToUse, branchToUse);

   if (git_reference_lookup(&remoteRef, mRepo, remoteBranchRef) == 0 &&
      git_annotated_commit_from_ref(&theirHead, mRepo, remoteRef) == 0)
   {
      git_merge_analysis_t analysis;
      git_merge_preference_t preference;
      git_merge_analysis(&analysis, &preference, mRepo, (const git_annotated_commit**)&theirHead, 1);

      if (analysis & (GIT_MERGE_ANALYSIS_FASTFORWARD | GIT_MERGE_ANALYSIS_NORMAL)) {
         mHasUpdates = true;
      }
   }

   if (theirHead) git_annotated_commit_free(theirHead);
   if (remoteRef) git_reference_free(remoteRef);
   git_remote_free(remote);
   return mHasUpdates;
}

void gitObject::update(StringTableEntry remoteName, StringTableEntry branchName)
{
   if (!gGitRunning || !mRepo)
   {
      Con::errorf("Git: Cannot perform update. Git not ready or repository not open.");
      return;
   }

   git_annotated_commit* theirHead = nullptr;
   git_reference* remoteRef = nullptr;

   StringTableEntry remoteToUse = remoteName ? remoteName : mRemoteName;
   StringTableEntry branchToUse = branchName ? branchName : mBranchName;

   char* remoteBranchRef;
   dSprintf(remoteBranchRef, sizeof(remoteBranchRef), "refs/remotes/%s/%s", remoteToUse, branchToUse);

   if (git_reference_lookup(&remoteRef, mRepo, remoteBranchRef) < 0 ||
      git_annotated_commit_from_ref(&theirHead, mRepo, remoteRef) < 0)
   {
      Con::errorf("Git: Remote reference or annotated commit not valid for merge.");
      return;
   }

   if (git_merge(mRepo, (const git_annotated_commit**)&theirHead, 1, &mMergeOpts, &mCheckoutOpts) < 0)
   {
      Con::errorf("Git: Failed to merge changes: %s", git_error_last()->message);
   }
   else
   {
      Con::printf("Git: Merge from remote '%s' successful.", remoteToUse);
   }

   git_repository_state_cleanup(mRepo);
   git_annotated_commit_free(theirHead);
   git_reference_free(remoteRef);
}

void gitObject::closeRepo()
{
   if (!gGitRunning) return;
   git_repository_free(mRepo);
   mRepo = NULL;
}

void gitObject::initPersistFields()
{
   addField("localPath", TypeString, Offset(mLocalPath, gitObject), "repository URL");
   addField("URL", TypeString, Offset(mUrl, gitObject), "repository URL");
}

DefineEngineMethod(gitObject, openRepo, String, (StringTableEntry localPath, StringTableEntry url),("", ""),
   "@brief opens a repository\n\n"
   "@param localPath location of hard drive directory\n\n"
   "@param URL location of remote directory\n\n")
{
   Torque::Path path = Torque::Path(*localPath ? localPath : object->mLocalPath);

   S32 error = object->openRepo(path.getFullPath(), *url ? url : object->mUrl);
   if (error < 0) {
      const git_error* e = git_error_last();
      return String::ToString("Error %d/%d: %s\n", error, e->klass, e->message);
   }
   return "";
}

DefineEngineMethod(gitObject, cloneRepo, String, (StringTableEntry localPath, StringTableEntry url), ("", ""),
   "@brief clones a repository\n\n"
   "@param localPath location of hard drive directory\n\n"
   "@param URL location of remote directory\n\n")
{
   Torque::Path path = Torque::Path(*localPath ? localPath : object->mLocalPath);

   S32 error = object->cloneRepo(path.getFullPath(), *url ? url : object->mUrl);
   if (error < 0) {
      const git_error* e = git_error_last();
      return String::ToString("Error %d/%d: %s\n", error, e->klass, e->message);
   }
   return "";
}

DefineEngineMethod(gitObject, checkState, bool, (StringTableEntry remoteName, StringTableEntry branchName), ("origin", "main"),
   "@brief Fetch updates from the remote repository and check if a merge is needed.\n\n"
   "@param remoteName Name of the remote, defaults to 'origin'.\n\n"
   "@param branchName Name of the branch, defaults to 'main'.\n\n"
   "@return True if updates are available, false otherwise.\n\n")
{
   return object->checkState(remoteName, branchName);
}

DefineEngineMethod(gitObject, update, void, (StringTableEntry remoteName, StringTableEntry branchName), ("origin", "main"),
   "@brief Checks for updates and merges them into the current branch.\n\n"
   "@param remoteName Name of the remote, defaults to 'origin'.\n\n"
   "@param branchName Name of the branch, defaults to 'main'.\n\n")
{
   if (object->checkState(remoteName, branchName))
   {
      object->update(remoteName, branchName);
   }
}

DefineEngineMethod(gitObject, closeRepo, void, ( ),,
   "@brief closes the current repository\n\n")
{

   object->closeRepo();
}
