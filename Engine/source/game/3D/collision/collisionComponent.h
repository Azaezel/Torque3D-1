#pragma once

#include "game/components/component.h"
#include "game/components/componentInstance.h"

#include "game/directors/directorManager.h"

#ifndef _CONVEX_H_
#include "collision/convex.h"
#endif
#ifndef _COLLISION_H_
#include "collision/collision.h"
#endif
#ifndef _T3D_PHYSICS_PHYSICSWORLD_H_
#include "T3D/physics/physicsWorld.h"
#endif
#ifndef _CLIPPEDPOLYLIST_H_
#include "collision/clippedPolyList.h"
#endif
#ifndef _SCENECONTAINER_H_
#include "scene/sceneContainer.h"
#endif

class CollisionDirector;
class TSMesh;
class TSShapeInstance;
class TSMaterialList;
class PhysicsCollision;

struct CollisionContactInfo
{
   bool contacted, move;
   SceneObject* contactObject;
   VectorF  idealContactNormal;
   VectorF  contactNormal;
   Point3F  contactPoint;
   F32	   contactTime;
   S32	   contactTimer;
   BaseMatInstance* contactMaterial;

   Vector<SceneObject*> overlapObjects;

   void clear()
   {
      contacted = move = false;
      contactObject = NULL;
      contactNormal.set(0, 0, 0);
      contactTime = 0.f;
      contactTimer = 0;
      idealContactNormal.set(0, 0, 1);
      contactMaterial = NULL;
      overlapObjects.clear();
   }

   CollisionContactInfo() { clear(); }

};

class CollisionComponent : public Component
{
   typedef Component Parent;

private:
   void onShapeChange() {}

public:
   CollisionComponent();

   DECLARE_CONOBJECT(CollisionComponent);

   bool onAdd();
   static void initPersistFields();
   static void consoleInit();

   /// <summary>
   /// See Component::packData()
   /// </summary>
   virtual void packData(BitStream* stream);
   /// <summary>
   /// See Component::unpackData()
   /// </summary>
   virtual void unpackData(BitStream* stream);

   /// <summary>
   /// This sets up some common, boilerplate signal calls that hook into the Director notifications
   /// Namely, when a component is added or removed, the signals will inform the associated director
   /// And the event can be handled
   /// </summary>
   COMP_REGISTER_SIGNALS(CollisionComponent);

   /// <summary>
   /// Creates a CollisionComponentInstance based on this template CollisionComponent.
   /// Also invokes setupFields to ensure that the CollisionComponentInstance is fully templated from this CollisionComponent
   /// Once created, the CollisionComponent is added to it's own static master list for self-management and retention
   /// </summary>
   /// <param name="owner">Owner ComponentObject to associate to the CollisionComponentInstance</param>
   /// <returns>The created ComponentInstance</returns>
   virtual ComponentInstance* createInstance(ComponentObject* owner);
};

class TSShapeInstance;

//
class CollisionComponentInstance : public ComponentInstance
{
   typedef ComponentInstance Parent;

   friend CollisionComponent;
   friend CollisionDirector;

public:
   // CollisionTimeout
   // This struct lets us track our collisions and estimate when they've have timed out and we'll need to act on it.
   struct CollisionTimeout
   {
      CollisionTimeout* next;
      SceneObject* object;
      U32 objectNumber;
      SimTime expireTime;
      VectorF vector;
   };

   Signal< void(SceneObject*) > onCollisionSignal;
   Signal< void(SceneObject*) > onContactSignal;

protected:
   PhysicsWorld* mPhysicsWorld;
   PhysicsBody* mPhysicsRep;

   CollisionTimeout* mTimeoutList;
   static CollisionTimeout* sFreeTimeoutList;

   CollisionList mCollisionList;
   Vector<CollisionComponentInstance*> mCollisionNotifyList;

   CollisionContactInfo mContactInfo;

   U32 CollisionMoveMask;

   bool mBlockColliding;

   bool mCollisionInited;

   void handleCollisionNotifyList();

   void queueCollision(SceneObject* obj, const VectorF& vec);

   /// checkEarlyOut
   /// This function lets you trying and early out of any expensive collision checks by using simple extruded poly boxes representing our objects
   /// If it returns true, we know we won't hit with the given parameters and can successfully early out. If it returns false, our test case collided
   /// and we should do the full collision sim.
   bool checkEarlyOut(Point3F start, VectorF velocity, F32 time, Box3F objectBox, Point3F objectScale,
      Box3F collisionBox, U32 collisionMask, CollisionWorkingList& colWorkingList);


private:
   static Vector<CollisionComponentInstance*> sComponentInstanceList;

   //Connection* mConnection;

public:
   DECLARE_CONOBJECT(CollisionComponentInstance);

   /// <summary>
   /// Obligatory default constructor
   /// </summary>
   CollisionComponentInstance() { mComponentData = nullptr; mOwner = nullptr; }
   /// <summary>
   /// The main constructor actually utilized by DOCs
   /// This will assign the template componentData and owner ComponentObject for this componentInstance
   /// </summary>
   /// <param name="componentData">Template Component</param>
   /// <param name="owner">Owner ComopnentObject</param>
   CollisionComponentInstance(const CollisionComponent& componentData, const ComponentObject& owner);
   ~CollisionComponentInstance();

   static void initPersistFields();

   /// <summary>
   /// Destroys this ComponentInstance, removing it from the static list
   /// </summary>
   virtual void destroyInstance();

   /// <summary>
   /// Called by CollisionDirector when it runs, this will render our shapeInstance(if we have one)
   /// </summary>
   /// <param name="transform">The transform to render at</param>
   virtual void update();

   /// <summary>
   /// See ComponentInstance::packUpdate();
   /// </summary>
   virtual U32 packUpdate(NetConnection* con, U32 mask, BitStream* stream);
   /// <summary>
   /// See ComponentInstance::unpackUpdate();
   /// </summary>
   virtual void unpackUpdate(NetConnection* con, BitStream* stream);

#pragma region Collision
   //Setup
   virtual void prepCollision() {};

   /// checkCollisions
   // This is our main function for checking if a collision is happening based on the start point, velocity and time
   // We do the bulk of the collision checking in here
   //virtual bool checkCollisions( const F32 travelTime, Point3F *velocity, Point3F start )=0;

   CollisionList* getCollisionList() { return &mCollisionList; }

   void clearCollisionList() { mCollisionList.clear(); }

   void clearCollisionNotifyList() { mCollisionNotifyList.clear(); }

   Collision* getCollision(S32 col);

   CollisionContactInfo* getContactInfo() { return &mContactInfo; }

   enum PublicConstants {
      CollisionTimeoutValue = 250
   };

   bool doesBlockColliding() { return mBlockColliding; }

   /// handleCollisionList
   /// This basically takes in a CollisionList and calls handleCollision for each.
   void handleCollisionList(CollisionList& collisionList, VectorF velocity);

   /// handleCollision
   /// This will take a collision and queue the collision info for the object so that in knows about the collision.
   void handleCollision(Collision& col, VectorF velocity);

   virtual bool checkCollisions(const F32 travelTime, Point3F* velocity, Point3F start);
   virtual bool updateCollisions(F32 time, VectorF vector, VectorF velocity);
   virtual void updateWorkingCollisionSet(const U32 mask);

   //
   bool buildConvexOpcode(TSShapeInstance* sI, S32 dl, const Box3F& bounds, Convex* c, Convex* list);
   bool buildMeshOpcode(TSMesh* mesh, const MatrixF& meshToObjectMat, const Box3F& bounds, Convex* convex, Convex* list);

   bool castRayOpcode(S32 dl, const Point3F& startPos, const Point3F& endPos, RayInfo* info);
   bool castRayMeshOpcode(TSMesh* mesh, const Point3F& s, const Point3F& e, RayInfo* info, TSMaterialList* materials);

   virtual bool castRay(const Point3F& start, const Point3F& end, RayInfo* info) { return false; }

   virtual PhysicsCollision* getCollisionData() {
      return nullptr;
   }

   virtual PhysicsBody* getPhysicsRep()
   {
      return mPhysicsRep;
   }

   void buildConvex(const Box3F& box, Convex* convex) {}
   bool buildPolyList(PolyListContext context, AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere) { return false; }

   //
   Point3F getContactNormal();
   bool hasContact();
   S32 getCollisionCount();
   Point3F getCollisionNormal(S32 collisionIndex);
   F32 getCollisionAngle(S32 collisionIndex, Point3F upVector);
   S32 getBestCollision(Point3F upVector);
   F32 getBestCollisionAngle(VectorF upVector);

   Signal< void(PhysicsCollision* collision) > onCollisionChanged;
#pragma endregion Collision
};

//
class CollisionDirector : public Director
{
   typedef Director Parent;
   friend DirectorManager;

   /// <summary>
   /// A struct containing an owner ComponentObject and its relevent components
   /// If this Director tracks if it's a valid ref or not, so if any of the required components
   /// are missing, invalid, or disabled, we can easily skip it and move on without needing to
   /// re-juggle lists or dependency tracking
   /// </summary>
   struct CollisionEntityRef
   {
      ComponentObject* owner;
      StrongRefPtr<CollisionComponentInstance> controlObj;

      bool isValid()
      {
         if (owner != nullptr && !controlObj.isNull())
            return true;

         return false;
      }
   };

private:
   /// <summary>
   /// A list of validated CollisionEntityRef entries.
   /// </summary>
   Vector<CollisionEntityRef> mValidEntriesList;

public:
   CollisionDirector();
   ~CollisionDirector();

   void registerComponent(ComponentObject* owner, const Component& comp);
   void unregisterComponent(ComponentObject* owner, const Component& comp);

   /// <summary>
   /// The main update function. Will iterate over valid CollisionEntityRef's and invoke them to update
   /// In our case, this means we take the mesh ref to a CollisionComponentInstance and have it render the shape
   /// </summary>
   virtual void update();
};
