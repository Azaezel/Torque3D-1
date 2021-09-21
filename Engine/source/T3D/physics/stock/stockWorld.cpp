#include "platform/platform.h"
#include "T3D/physics/stock/stockWorld.h"

#include "T3D/physics/physicsUserData.h"
#include "core/stream/bitStream.h"
#include "platform/profiler.h"
#include "sim/netConnection.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "scene/sceneRenderState.h"
#include "T3D/gameBase/gameProcess.h"
#include "T3D/physics/stock/stockBody.h"

StockWorld::StockWorld()
   :  mProcessList(NULL),
      mIsSimulating(false),
      mErrorReport(false),
      mTickCount(0),
      mIsEnabled(false),
      mEditorTimeScale(1.0f)
{
}

StockWorld::~StockWorld()
{
}

void StockWorld::addBody(PhysicsBody* body)
{
   /// for now just fire everything in here.
   mNonStaticBodies.push_back(body);
}

bool StockWorld::initWorld(bool isServer, ProcessList *processList)
{
   /// just cos.
   if (!processList)
      return false;

   mIsServer = isServer;

   mProcessList = processList;
   mProcessList->preTickSignal().notify(this, &StockWorld::getPhysicsResults );
   mProcessList->postTickSignal().notify(this, &StockWorld::tickPhysics, 1000.0f);

   return true;
}

void StockWorld::_destroy()
{
   // Release the tick processing signals.
   if (mProcessList)
   {
      mProcessList->preTickSignal().remove(this, &StockWorld::getPhysicsResults);
      mProcessList->postTickSignal().remove(this, &StockWorld::tickPhysics);
      mProcessList = NULL;
   }
}

void StockWorld::tickPhysics(U32 elapsedMs)
{
   if (!mIsEnabled)
      return;

   AssertFatal(!mIsSimulating, "StockWorld::tickPhysics() - Already Simulating!");
   AssertFatal(elapsedMs != 0 &&
      (elapsedMs % TickMs) == 0, "StockWorld::tickPhysics() - Got bad elapsed time!");

   PROFILE_SCOPE(StockWorld_tickPhysics);

   const F32 elapsedSec = (F32)elapsedMs * 0.001f;

   stepWorld(elapsedSec * mEditorTimeScale, smPhysicsMaxSubSteps, smPhysicsStepTime);

   mIsSimulating = true;

}

void StockWorld::stepWorld(F32 elapsed, U32 steps, F32 stepTime)
{
   for (U32 i = 0; i < mNonStaticBodies.size(); i++)
   {
      StockBody* body = static_cast<StockBody*>(mNonStaticBodies[i]);
      if (!body->isDynamic())
         continue;

      body->updateWorkingCollisionSet(-1, elapsed);
      //body->updateForces(elapsed);
      body->updatePos(elapsed);
   }
}

void StockWorld::getPhysicsResults()
{
   if (!mIsSimulating)
      return;

   PROFILE_SCOPE(StockWorld_GetPhysicsResults);

   mIsSimulating = false;
   mTickCount++;
}

void StockWorld::setEnabled(bool enabled)
{
   mIsEnabled = enabled;

   if (!mIsEnabled)
      getPhysicsResults();
}

PhysicsBody* StockWorld::castRay(const Point3F& start, const Point3F& end, U32 bodyTypes)
{
   /*for (U32 i = 0; i < mNonStaticBodies.size(); i++)
   {
      mNonStaticBodies[i]->cas
   }*/
   return NULL;
}

