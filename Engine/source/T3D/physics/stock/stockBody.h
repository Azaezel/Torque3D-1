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

class StockWorld;
class StockCollision; /// the reason why this should be folded in. it should be called shape.

class StockBody : public PhysicsBody
{
   StockWorld *mWorld;
   StrongRefPtr<StockCollision> mColShape;
   U32 mColMask;
   U32 mBodyFlags;
   /// holds the same (few more) as mRigid.
   Point3F force;
   Point3F torque;

   F32 mMass;
   bool mIsDynamic;
   bool mIsEnabled;
   bool mSleep;

   MatrixF *mCenterOfMass;
   MatrixF *mInvCenterOfMass;
   MatrixF mObjInertia;
   Point3F mLinVelocity;
   Point3F mAngVeloctiy;
   F32 mAngSleep;
   F32 mLinSleep;
   F32 mAngDamp;
   F32 mLinDamp;
   F32 mRestitution;
   F32 mFriction;
   F32 mStaticFriction;

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

   ///Main physics loop of stuffs.
   void integrateVelocities(F32 step);

   // PhysicsBody
   virtual bool init(PhysicsCollision *shape, F32 mass, U32 bodyFlags, SceneObject *obj, PhysicsWorld *world);
   virtual bool isDynamic() const { return mIsDynamic; }
   
   virtual void setSleepThreshold(F32 linear, F32 angular) { mLinSleep = linear; mAngSleep = angular; }
   virtual void setDamping(F32 linear, F32 angular) { mLinDamp = linear; mAngDamp = angular; }
   virtual void getState(PhysicsState *outState);
   virtual F32 getMass() const { return mMass; }
   virtual Point3F getCMassPosition() const;
   virtual void setLinVelocity(const Point3F &vel) { mLinVelocity = vel; }
   virtual void setAngVelocity(const Point3F &vel) { mAngVeloctiy = vel; }
   virtual Point3F getLinVelocity() const { return mLinVelocity; }
   virtual Point3F getAngVelocity() const { return mAngVeloctiy; }
   virtual void setSleeping(bool sleeping) { mSleep = sleeping; };
   virtual void setMaterial(F32 restitution, F32 friction, F32 staticFriction);
   virtual void applyCorrection(const MatrixF &xfm);
   void setCMassTransform(const MatrixF& xfm);

   virtual void applyImpulse(const Point3F &origin, const Point3F &force);
   virtual void applyTorque(const Point3F &torque);
   virtual void applyForce(const Point3F &force);
   virtual void findContact(SceneObject **contactObject, VectorF *contactNormal, Vector<SceneObject*> *outOverlapObjects) const;
   virtual void moveKinematicTo(const MatrixF &xfm);

   virtual bool isValid() { return mColShape != nullptr; }

   virtual PhysicsWorld* getWorld() { return mWorld; }
   virtual void setTransform(const MatrixF &xfm);
   virtual MatrixF& getTransform(MatrixF *outMatrix);
   virtual Box3F getWorldBounds();
   virtual void setSimulationEnabled(bool enabled);
   virtual bool isSimulationEnabled() { return mIsEnabled; }
   virtual PhysicsCollision* getColShape() { return mColShape; }


};


#endif // !_T3D_STOCKBODY_H_
