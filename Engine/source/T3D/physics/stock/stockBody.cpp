#include "platform/platform.h"
#include "T3D/physics/stock/stockBody.h"

#include "scene/sceneObject.h"
#include "T3D/physics/stock/stockWorld.h"
#include "T3D/physics/stock/stockCollision.h"
#include "math/mBox.h"
#include "console/console.h"

StockBody::StockBody()
   :  mWorld(NULL),
      mMass(0.0f),
      mCenterOfMass(NULL),
      mInvCenterOfMass(NULL),
      mIsDynamic(false),
      mIsEnabled(false),
      mSleep(false)
{
   mObjInertia.identity();
}

StockBody::~StockBody()
{
}

bool StockBody::init(PhysicsCollision * shape, F32 mass, U32 bodyFlags, SceneObject * obj, PhysicsWorld * world)
{
   mWorld = (StockWorld*)world;

   mColShape = (StockCollision*)shape;
   //stockCollisionShape * stColShape = mColShape->getShape();
   MatrixF localXfm = mColShape->getLocalTransform();
   Point3F localInertia(0, 0, 0);

   /// parity for now but this has got to go!
   mIsDynamic = mass > 0.0f;
   if (mIsDynamic)
   {
      mObjInertia.identity();
      F32 rad = 1.0f;
      F32* f = mObjInertia;
      f[0] = f[5] = f[10] = (0.4f * mass * rad * rad);
   }

   if (!localXfm.isIdentity())
   {
      mCenterOfMass = new MatrixF(localXfm);
      mInvCenterOfMass = new MatrixF(*mCenterOfMass);
      mInvCenterOfMass->inverse();
   }

   mMass = mass;

   mWorld->addBody(this);

   mUserData.setObject(obj);
   mUserData.setBody(this);
   /// everything should be awake on init.
   mSleep = false;

   return true;
}

void StockBody::getState(PhysicsState * outState)
{
   MatrixF trans;

   if (mInvCenterOfMass)
      trans.mul(*mCenterOfMass, *mInvCenterOfMass);
   else
      trans = *mCenterOfMass;

   outState->position = trans.getPosition();
   outState->orientation.set(trans);
   outState->linVelocity = getLinVelocity();
   outState->angVelocity = getAngVelocity();

   outState->momentum = (1.0f / (1 / mMass)) * outState->linVelocity;
   outState->sleeping = mSleep;

}

Point3F StockBody::getCMassPosition() const
{
   return mCenterOfMass->getPosition();
}

void StockBody::setMaterial(F32 restitution, F32 friction, F32 staticFriction)
{
   mRestitution = restitution;
   mFriction = friction;
   mStaticFriction = staticFriction;
}

void StockBody::setTransform(const MatrixF & xfm)
{
   if (mCenterOfMass)
   {
      MatrixF transform;
      transform.mul(xfm, *mCenterOfMass);
   }

   if (isDynamic())
   {
      setLinVelocity(Point3F::Zero);
      setAngVelocity(Point3F::Zero);
   }

}

MatrixF& StockBody::getTransform(MatrixF *outMatrix)
{
   if (mInvCenterOfMass)
      outMatrix->mul(*mInvCenterOfMass, *mCenterOfMass);
   else
      *outMatrix = *mCenterOfMass;

   return *outMatrix;

}

Box3F StockBody::getWorldBounds()
{
   SceneObject* obj = mUserData.getObject();
   return obj->getWorldBox();
}

void StockBody::setSimulationEnabled(bool enabled)
{
   if (mIsEnabled == enabled)
      return;

   mIsEnabled = enabled;
}
