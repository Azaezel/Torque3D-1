#include "gui/core/guiCanvas.h"
#include "console/engineAPI.h"
#include "console/script.h"
#include "gfx/gfxDebugEvent.h"

#include "gfx/gl/gfxGLDevice.h"

#ifndef TORQUE_BASIC_GPU_PROFILER
   //#define TORQUE_BASIC_GPU_PROFILER
#endif

class GLTimer
{
public:      

   void begin()
   {
      glBeginQuery(GL_TIME_ELAPSED, mQueryId);
   }

   void end()
   {
     glEndQuery(GL_TIME_ELAPSED);
   }

   F64 getTime()
   {
      GLuint64 time;
      glGetQueryObjectui64v(mQueryId, GL_QUERY_RESULT, &time);
      return static_cast<F64>(time)/1000000.0f;
   }      

   class Data
   {
   public:
      
      Data()  {}

      void init()
      {

      }

      void onBeginFrame()
      {
         
      }

      void onEndFrame()
      {
         
      }
   };

   typedef Data DataType;

    GLTimer(GFXDevice *device, Data &data) : mName(NULL), mData(&data)
   {
      glGenQueries(1, &mQueryId);
   }

    GLTimer() : mName(NULL), mData(NULL), mQueryId(0)
    {

    }

    GLTimer& operator=(const GLTimer &b)
    {
       mName = b.mName;
       mQueryId = b.mQueryId;
       return *this;
    }

    StringTableEntry mName; 

protected:
   Data *mData;
   GLuint mQueryId;
   
};


#ifdef TORQUE_BASIC_GPU_PROFILER

#include "gfx/gfxProfiler.h"


GFXProfiler<GLTimer> gfxProfiler;

DefineEngineFunction(printGFXGLTimers, void,(), ,"")
{
   gfxProfiler.printTimes();
}

#endif

bool initGLProfiler(GFXDevice::GFXDeviceEventType ev)
{
   if(ev != GFXDevice::deInit || GFX->getAdapterType() != OpenGL)        
      return true;

   Con::evaluatef("GlobalActionMap.bindCmd(keyboard, \"alt F4\", \"printGFXGLTimers();\");");
   return true;
}

void GFXGLDevice::enterDebugEvent(ColorI color, const char *name)
{
#ifdef TORQUE_BASIC_GPU_PROFILER
   gfxProfiler.enterDebugEvent(color, name);
#endif

   if (mCapabilities.khrDebug)
   {
      glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name);
   }
   else if(mCapabilities.extDebugMarker)
   {
      glPushGroupMarkerEXT(0, name);
   }
}

void GFXGLDevice::leaveDebugEvent()
{
#ifdef TORQUE_BASIC_GPU_PROFILER
   gfxProfiler.leaveDebugEvent();
#endif

   if (mCapabilities.khrDebug)
   {
      glPopDebugGroup();
   }
   else if(mCapabilities.extDebugMarker)
   {
      glPopGroupMarkerEXT();
   }
}

void GFXGLDevice::setDebugMarker(ColorI color, const char *name)
{
   if (mCapabilities.khrDebug)
   {
      glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0,
         GL_DEBUG_SEVERITY_NOTIFICATION, -1, name);
   }
   else if(mCapabilities.extDebugMarker)
   {
      glInsertEventMarkerEXT(0, name);
   }
}

#ifdef TORQUE_BASIC_GPU_PROFILER

AFTER_MODULE_INIT(Sim)
{
   // GFXGLDevice Profiler
   GuiCanvas::getGuiCanvasFrameSignal().notify(&gfxProfiler, &GFXProfiler<GLTimer>::onEndFrame);
   GFXDevice::getDeviceEventSignal().notify( &initGLProfiler );   
}

#endif
