#ifndef _T3D_STOCKBODY_H_
#define _T3D_STOCKBODY_H_

#ifndef _T3D_PHYSICS_PHYSICSBODY_H_
#include "T3D/physics/physicsBody.h"
#endif

#ifndef _REFBASE_H_
#include "core/util/refBase.h"
#endif

#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif

#ifndef _T3D_STOCKWORLD_H_
#include "T3D/physics/stock/stockWorld.h"
#endif
#ifndef _T3D_STOCKCOLLISION_H_
#include "T3D/physics/stock/stockCollision.h"
#endif
#include "T3D/rigid.h"
#include "collision/collision.h"

class StockWorld;
class StockCollision; /// the reason why this should be folded in. it should be called shape.

class StockBody : public PhysicsBody
{
   StockWorld *mWorld;
   StrongRefPtr<StockCollision> mColShape;
   U32 mColMask;
   U32 mBodyFlags;
   /// holds the same (few more) as mRigid.
   Point3F mForce;
   Point3F mTorque;

   F32 mMass;
   bool mIsDynamic;
   bool mIsEnabled;
   bool mSleep;
   S32 mSleepCount;

   MatrixF mObjectInertia;        ///< Moment of inertia
   MatrixF mInvObjectInertia;     ///< Inverse moment of inertia
   MatrixF mInvWorldInertia;      ///< Inverse moment of inertia in world space
   Point3F mWorldCenterOfMass;    ///< CofM in world space
   F32 mOneOverMass;              ///< 1 / mass
   //MatrixF *mCenterOfMass;
   Point3F mCenterOfMass;
   MatrixF *mInvCenterOfMass;
   MatrixF mObjInertia;
   Point3F mLinVelocity;
   Point3F mAngVelocity;
   Point3F mLinFactor;
   Point3F mAngFactor;
   Point3F mLinPosition;          ///< Current position
   QuatF   mAngPosition;          ///< Current rotation
   Point3F mLinMomentum;          ///< Linear momentum
   Point3F mAngMomentum;          ///< Angular momentum
   F32 mAngSleep;
   F32 mLinSleep;
   F32 mAngDamp;
   F32 mLinDamp;
   F32 mRestitution;
   F32 mFriction;
   F32 mStaticFriction;

   //
   Box3F mWorkingQueryBox;
   CollisionList mCollisionList;
   CollisionList mContacts;

public:
   enum PublicConstants {
       CollisionTimeoutValue = 250      ///< Timeout in ms.
   };

   /// @name Collision Notification
   /// This is used to keep us from spamming collision notifications. When
   /// a collision occurs, we add to this list; then we don't notify anyone
   /// of the collision until the CollisionTimeout expires (which by default
   /// occurs in 1/10 of a second).
   ///
   /// @see notifyCollision(), queueCollision()
   /// @{
   struct CollisionTimeout
   {
      CollisionTimeout* next;
      SceneObject* object;
      U32 objectNumber;
      SimTime expireTime;
      VectorF vector;
   };
   CollisionTimeout* mTimeoutList;
   static CollisionTimeout* sFreeTimeoutList;

public:
   ///---------------------------------------------------------------
   /// TODO: For multi-shape collisions check the influence
   ///       of each shapes bbox rather than the body.
   ///       Costly but means each collision mesh can have
   ///       different properties if required.
   ///       example: vehicles, doors, ragdoll.
   ///---------------------------------------------------------------
   StockBody();
   virtual ~StockBody();

   bool isAsleep() { return mSleep; }
   void awaken() { mSleep = false; }
   PhysicsUserData getUser() { return mUserData; }
   /// shocking that PhysicsBody doesn't have this as default.
   void setColMask(const U32 mask) { mColMask = mask; }
   const U32 getColMask() const { return mColMask; }
   /// important for our stepworld.
   const U32 getBodyTypes() const { return mBodyFlags; }

   // PhysicsBody
   virtual bool init(PhysicsCollision *shape, F32 mass, U32 bodyFlags, SceneObject *obj, PhysicsWorld *world);
   virtual bool isDynamic() const { return mIsDynamic; }
   
   virtual void setSleepThreshold(F32 linear, F32 angular) { mLinSleep = linear; mAngSleep = angular; }
   virtual void setDamping(F32 linear, F32 angular) { mLinDamp = linear; mAngDamp = angular; }
   virtual void getState(PhysicsState *outState);
   virtual F32 getMass() const { return mMass; }
   virtual Point3F getCMassPosition() const;
   virtual void setLinVelocity(const Point3F &vel) { mLinVelocity = vel; }
   virtual void setAngVelocity(const Point3F &vel) { mAngVelocity = vel; }
   virtual Point3F getLinVelocity() const { return mLinVelocity; }
   virtual Point3F getAngVelocity() const { return mAngVelocity; }
   virtual void setSleeping(bool sleeping) { mSleep = sleeping; };
   virtual void setMaterial(F32 restitution, F32 friction, F32 staticFriction);
   virtual void applyCorrection(const MatrixF &xfm);
   void applyDamping(F32 delta);
   void setCMassTransform(const MatrixF& xfm);

   virtual void applyImpulse(const Point3F &origin, const Point3F &force);
   void applyTorqueImpulse(const Point3F & torque);
   virtual void applyTorque(const Point3F &torque);
   virtual void applyForce(const Point3F &force);
   virtual void findContact(SceneObject **contactObject, VectorF *contactNormal, Vector<SceneObject*> *outOverlapObjects) const;
   virtual void moveKinematicTo(const MatrixF &xfm);

   virtual bool isValid() { return mColShape != nullptr; }

   virtual PhysicsWorld* getWorld() { return mWorld; }
   virtual void setTransform(const MatrixF& xfm);
   virtual MatrixF& getTransform(MatrixF* outMatrix);
   virtual Box3F getWorldBounds();
   virtual void setSimulationEnabled(bool enabled);
   virtual bool isSimulationEnabled() { return mIsEnabled; }
   virtual PhysicsCollision* getColShape() { return mColShape; }

   //internal functions for the sim handling
   bool castRay(const Point3F& start, const Point3F& end, RayInfo* info);
   void updateWorkingCollisionSet(const U32 mask, const F32 dt);
   void updateForces(F32 dt);
   void updatePos(F32 dt);
   bool updateCollision(F32 dt);
   bool resolveCollision(CollisionList& cList);
   bool resolveContacts(CollisionList& cList, F32 dt);
   void checkTriggers();
   void clearForces();

   void queueCollision(SceneObject* obj, const VectorF& vec);
   void notifyCollision();

   CollisionList getCollisionList() { return mCollisionList; }
   CollisionList getContactsList() { return mContacts; }

   //rigid sim functions
private:
   void translateCenterOfMass(const Point3F& oldPos, const Point3F& newPos);

public:
   void integrate(F32 delta);

   void updateInertialTensor();
   void updateVelocity();
   void updateCenterOfMass();

   bool resolveCollision(const Point3F& p, const Point3F& normal, StockBody*);
   bool resolveCollision(const Point3F& p, const Point3F& normal);

   F32  getZeroImpulse(const Point3F& r, const Point3F& normal);
   F32  getKineticEnergy();
   void getOriginVector(const Point3F& r, Point3F* v);
   void setCenterOfMass(const Point3F& v);
   void getVelocity(const Point3F& p, Point3F* r);

   void setObjectInertia(const Point3F& r);
   void setObjectInertia();
   void invertObjectInertia();

   bool checkSleepCondition();
   void setSleep();

   MatrixF getTransform();
   //void setTransform(const MatrixF& mat);
};


#endif // !_T3D_STOCKBODY_H_
