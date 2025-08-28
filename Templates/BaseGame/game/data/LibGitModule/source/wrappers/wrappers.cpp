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

//general subsystem
DefineEngineFunction(git_init, String, (), ,
        "@brief initialize libGit2.\n\n")
{
   S32 error = git_libgit2_init();
	if (error < 0) {
		const git_error *e = git_error_last();
		return String::ToString("Error %d/%d: %s\n", error, e->klass, e->message);
	}
	return "";
}

DefineEngineFunction(git_shutdown, String, (), ,
        "@brief Logs a message to the console.\n\n"
        "@param message The message text.\n"
        "@note By default, messages will appear white in the console.\n"
        "@ingroup Logging")
{
   S32 error = git_libgit2_shutdown();
	if (error < 0) {
		const git_error *e = git_error_last();
		return String::ToString("Error %d/%d: %s\n", error, e->klass, e->message);
	}
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

   Con::warnf("fetch_progress %d/%d", stats->received_objects, stats->total_objects);
   if (pd->mSessionPtr)
      pd->mSessionPtr->updateProgress(pd);

   return 0;
}

void checkout_progress(
   const char* path,
   size_t cur,
   size_t tot,
   void* payload)
{
   gitProgress* pd = (gitProgress*)payload;
   if (pd->mSessionPtr)
      pd->mSessionPtr->updateProgress(pd);
}

//session object
IMPLEMENT_CONOBJECT(gitObject);

IMPLEMENT_CALLBACK(gitObject, onProgress, void, (), (),
   "Called every 32ms on the control.");
IMPLEMENT_CALLBACK(gitObject, onStart, void, (), (),
   "Called when the control starts to scroll.");
IMPLEMENT_CALLBACK(gitObject, onComplete, void, (), (),
   "Called when the child control has been scrolled in entirety.");

gitObject::gitObject()
   : mRepo(NULL),
   mCurPercent(0),
   mUrl(StringTable->EmptyString()),
   mLocalPath(StringTable->EmptyString()),
   mRepoDesc(StringTable->EmptyString())
{
   mCallOnAdvanceTime = false;
   mProgress_data.mPercent = 0;
   mProgress_data.mSessionPtr = this;

   mClone_opts = GIT_CLONE_OPTIONS_INIT;
   mClone_opts.checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
   mClone_opts.checkout_opts.progress_cb = checkout_progress;
   mClone_opts.checkout_opts.progress_payload = &mProgress_data;
   mClone_opts.fetch_opts.callbacks.transfer_progress = fetch_progress;
   mClone_opts.fetch_opts.callbacks.payload = &mProgress_data;
}

bool gitObject::onAdd()
{
   if (!Parent::onAdd())
      return false;

   mRepo = NULL;
   mProgress_data.mPercent = 0;
   mCurPercent = 0;
   mProgress_data.mSessionPtr = this;
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
   if (mCurPercent != mProgress_data.mPercent)
   {
      Con::warnf("tick");
      if (mProgress_data.mPercent == 1.0f)
      {
         onComplete_callback();
         setProcessTicks(false);
      }
      else if (mCurPercent == 0)
         onStart_callback();
      else
         onProgress_callback();

      mCurPercent = mProgress_data.mPercent;
   }
}

S32 gitObject::openRepo(StringTableEntry path, StringTableEntry url)
{
   closeRepo();
   git_repository_init_options opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;

   /* Customize options */
   opts.flags |= GIT_REPOSITORY_INIT_MKPATH; /* mkdir as needed to create repo */
   opts.origin_url = url;
   S32 errCode = git_repository_init_ext(&mRepo, path, &opts);
   return errCode;
}

S32 gitObject::cloneRepo(StringTableEntry path, StringTableEntry url)
{
   mProgress_data = { NULL };

   setProcessTicks(true);
   S32 errCode = git_clone(&mRepo, url, path, &mClone_opts);
   return errCode;
}

void gitObject::updateProgress(gitProgress* progress)
{
   mProgress_data.mPercent = progress->mPercent;
}

void gitObject::closeRepo()
{
   git_repository_free(mRepo);
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

DefineEngineMethod(gitObject, closeRepo, void, ( ),,
   "@brief closes the current repository\n\n")
{

   object->closeRepo();
}
