#ifndef GL_CIRCULAR_VOLATILE_BUFFER_H
#define GL_CIRCULAR_VOLATILE_BUFFER_H

#include "gfx/gl/gfxGLDevice.h"
#include "gfx/gl/gfxGLUtils.h"

class GLCircularVolatileBuffer
{
public:
   GLCircularVolatileBuffer(GLuint binding) 
      : mBinding(binding),
      mBufferName(0),
      mBufferPtr(nullptr),
      mBufferSize(0),
      mBufferFreePos(0),
      mCurrentRangeStart(0)
   { 
      init();
   }

   ~GLCircularVolatileBuffer()
   {
      waitAll();
      glDeleteBuffers(1, &mBufferName);
   }

   void init()
   {
      glGenBuffers(1, &mBufferName);

      PRESERVE_BUFFER( mBinding );
      glBindBuffer(mBinding, mBufferName);
     
      const U32 cSizeInMB = 10;
      mBufferSize = (cSizeInMB << 20);

      if( GFXGL->mCapabilities.bufferStorage )
      {      
         const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
         glBufferStorage(mBinding, mBufferSize, NULL, flags);
         mBufferPtr = glMapBufferRange(mBinding, 0, mBufferSize, flags);
      }
      else
      {
         glBufferData(mBinding, mBufferSize, NULL, GL_DYNAMIC_DRAW);
      }
   }

   void lock(const U32 size, U32 offsetAlign, U32& outOffset, void*& outPtr)
   {
      AssertFatal(size > 0, "Size must be > 0");

      align(mBufferFreePos, offsetAlign);

      // Wrap-around
      if (mBufferFreePos + size > mBufferSize)
      {
         if (mCurrentRangeStart < mBufferFreePos)
         {
            protectRange(mCurrentRangeStart, mBufferFreePos - 1);
         }

         mBufferFreePos = 0;
         mCurrentRangeStart = 0;

         align(mBufferFreePos, offsetAlign);
      }

      waitOverlap(mBufferFreePos, mBufferFreePos + size - 1);

      outOffset = mBufferFreePos;

      if (GFXGL->mCapabilities.bufferStorage)
      {
         outPtr = static_cast<U8*>(mBufferPtr) + mBufferFreePos;
      }
      else
      {
         PRESERVE_BUFFER(mBinding);
         glBindBuffer(mBinding, mBufferName);

         outPtr = glMapBufferRange(
            mBinding,
            outOffset,
            size,
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT
         );
      }

      mBufferFreePos += size;
      align(mBufferFreePos, 4);
   }

   void unlock()
   {
      if (!GFXGL->mCapabilities.bufferStorage)
      {
         PRESERVE_BUFFER(mBinding);
         glBindBuffer(mBinding, mBufferName);
         glUnmapBuffer(mBinding);
      }
   }

   U32 getHandle() const { return mBufferName; }

   void protectUsedRange()
   {
      if (mCurrentRangeStart < mBufferFreePos)
      {
         protectRange(mCurrentRangeStart, mBufferFreePos - 1);
         mCurrentRangeStart = mBufferFreePos;
      }
   }

protected:

   struct FenceRange
   {
      U32 start;
      U32 end;
      GLsync fence;
   };

   GLuint mBinding;
   GLuint mBufferName;
   void *mBufferPtr;
   U32 mBufferSize;
   U32 mBufferFreePos;
   U32 mCurrentRangeStart;
   Vector<FenceRange> mFenceRanges;

   FrameAllocatorLockableHelper mFrameAllocator;

   static void align(U32& value, U32 alignment)
   {
      if (alignment)
         value = (value + alignment - 1) & ~(alignment - 1);
   }

   static bool overlaps(U32 a0, U32 a1, U32 b0, U32 b1)
   {
      return a0 <= b1 && b0 <= a1;
   }

   void protectRange(U32 start, U32 end)
   {
      FenceRange r;
      r.start = start;
      r.end = end;
      r.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
      mFenceRanges.push_back(r);
   }

   void waitOverlap(U32 start, U32 end)
   {
      for (auto it = mFenceRanges.begin(); it != mFenceRanges.end(); )
      {
         if (!overlaps(start, end, it->start, it->end))
         {
            ++it;
            continue;
         }

         // Poll until signaled
         while (true)
         {
            GLenum r = glClientWaitSync(it->fence, 0, 0);
            if (r == GL_ALREADY_SIGNALED ||
               r == GL_CONDITION_SATISFIED)
               break;
         }

         glDeleteSync(it->fence);
         mFenceRanges.erase(it);
      }
   }

   void waitAll()
   {
      for (auto& r : mFenceRanges)
      {
         while (true)
         {
            GLenum s = glClientWaitSync(r.fence, 0, 0);
            if (s == GL_ALREADY_SIGNALED ||
               s == GL_CONDITION_SATISFIED)
               break;
         }
         glDeleteSync(r.fence);
      }
      mFenceRanges.clear();
   }
};


#endif
