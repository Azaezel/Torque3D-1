#ifndef _T3D_STOCKWORLD_H_
#define _T3D_STOCKWORLD_H_

#ifndef _T3D_PHYSICS_PHYSICSWORLD_H_
#include "T3D/physics/physicsWorld.h"
#endif

#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

class ProcessList;
class PhysicsBody;

class StockWorld : public PhysicsWorld
{
protected:
   F32 mEditorTimeScale;
   bool mErrorReport;
   bool mIsEnabled;
   bool mIsSimulating;
   U32 mTickCount;

   ProcessList *mProcessList;

   Vector<PhysicsBody*> mNonStaticBodies;

   void _destroy();

public:

   StockWorld();
   virtual ~StockWorld();

   void addBody(PhysicsBody* body);

   virtual bool initWorld(bool isServer,ProcessList *processList);

   void tickPhysics(U32 elapsedMs);
   void stepWorld(F32 elapsed, U32 steps, F32 stepTime);
   void getPhysicsResults();

   virtual void onDebugDraw(const SceneRenderState *state) {}
   virtual void reset() {}
   virtual bool isEnabled() const { return mIsEnabled; }

   void setEnabled(bool enabled);
   bool getEnabled() const { return mIsEnabled; }

   void setEditorTimeScale(F32 timeScale) { mEditorTimeScale = timeScale; }
   const F32 getEditorTimeScale() const { return mEditorTimeScale; }

   virtual void destroyWorld() {}
   virtual bool castRay(const Point3F &startPnt, const Point3F &endPnt, RayInfo *ri, const Point3F &impulse) { return false; }
   virtual PhysicsBody* castRay(const Point3F &start, const Point3F &end, U32 bodyTypes) { return NULL; }
   virtual void explosion(const Point3F &pos, F32 radius, F32 forceMagnitude) {}

};


#endif // !_T3D_STOCKWORLD_H_
