#include "platform/platform.h"
#include "T3D/physics/stock/stockBody.h"

#include "scene/sceneObject.h"
#include "T3D/physics/stock/stockWorld.h"
#include "T3D/physics/stock/stockCollision.h"
#include "math/mBox.h"
#include "console/console.h"
#include "collision/clippedPolyList.h"
#include "collision/convex.h"
#include "T3D/shapeBase.h"

static F32 sTractionDistance = 0.04f;

static U32 sCollisionMoveMask = TerrainObjectType |
                                 WaterObjectType |
                                 PlayerObjectType |
                                 StaticShapeObjectType |
                                 VehicleObjectType |
                                 PhysicalZoneObjectType |
                                 PathShapeObjectType;

static F32 sRestTol = 0.5;             // % of gravity energy to be at rest
static S32 sRestCount = 10;            // Consecutive ticks before comming to rest
static F32 sCollisionTol = 0.3;
static F32 sContactTol = 0.2;

Chunker<StockBody::CollisionTimeout> sTimeoutChunker;
StockBody::CollisionTimeout* StockBody::sFreeTimeoutList = 0;

StockBody::StockBody()
   :  mWorld(NULL),
      mMass(0.0f),
      mIsDynamic(false),
      mIsEnabled(false),
      mSleep(false),
      mLinFactor(1, 1, 1),
      mAngFactor(1, 1, 1),
      mTimeoutList(NULL)
{
   /// need to make sure this is OOBB.
   mAABB = Box3F(Point3F(0, 0, 0), Point3F(0, 0, 0));
   mOOBB = Box3F(Point3F(0, 0, 0), Point3F(0, 0, 0));
   mObjInertia.identity();
   mInvCenterOfMass = new MatrixF();

   mForce.set(0.0f, 0.0f, 0.0f);
   mTorque.set(0.0f, 0.0f, 0.0f);
   mLinVelocity.set(0.0f, 0.0f, 0.0f);
   mLinPosition.set(0.0f, 0.0f, 0.0f);
   mLinMomentum.set(0.0f, 0.0f, 0.0f);
   mAngVelocity.set(0.0f, 0.0f, 0.0f);
   mAngMomentum.set(0.0f, 0.0f, 0.0f);
   mAngPosition.identity();

   mInvWorldInertia.identity();

   mCenterOfMass.set(0.0f, 0.0f, 0.0f);
   mWorldCenterOfMass = mLinPosition;
   mInvObjectInertia.identity();
   mRestitution = 0.3f;
   mFriction = 0.5f;
}

StockBody::~StockBody()
{
   CollisionTimeout* ptr = mTimeoutList;
   while (ptr) {
      CollisionTimeout* cur = ptr;
      ptr = ptr->next;
      cur->next = sFreeTimeoutList;
      sFreeTimeoutList = cur;
   }
}

bool StockBody::init(PhysicsCollision * shape, F32 mass, U32 bodyFlags, SceneObject * obj, PhysicsWorld * world)
{
   mWorld = (StockWorld*)world;

   mColShape = (StockCollision*)shape;
   mColShape->setObject(obj);

   //stockCollisionShape * stColShape = mColShape->getShape();
   MatrixF localXfm = mColShape->getLocalTransform();
   Point3F localInertia(0, 0, 0);

   mBodyFlags = bodyFlags;

   if (mBodyFlags & BF_DYNAMIC)
      mIsDynamic = true;

   if (mIsDynamic)
   {
      mObjInertia.identity();
      F32 rad = 1.0f;
      F32* f = mObjInertia;
      f[0] = f[5] = f[10] = (0.4f * mMass * rad * rad);
   }

   if (!localXfm.isIdentity())
   {
      //mCenterOfMass = new MatrixF(localXfm);
      mCenterOfMass = localXfm.getPosition();
      mInvCenterOfMass = new MatrixF(localXfm);
      mInvCenterOfMass->setPosition(mCenterOfMass);
      mInvCenterOfMass->inverse();
   }

   mMass = mass;

   if (mMass != 0)
      mOneOverMass = 1 / mMass;
   else
      mOneOverMass = 0;

   mWorld->addBody(this);

   mAABB = obj->getWorldBox();
   mOOBB = obj->getObjBox();

   mUserData.setObject(obj);
   mUserData.setBody(this);
   /// everything should be awake on init.
   mSleep = false;
   mIsEnabled = true;

   mConvex.init(obj);
   Box3F objBox = obj->getObjBox();
   objBox.getCenter(&mConvex.mCenter);
   mConvex.mSize.x = objBox.len_x() / 2.0;
   mConvex.mSize.y = objBox.len_y() / 2.0;
   mConvex.mSize.z = objBox.len_z() / 2.0;
   mWorkingQueryBox.minExtents.set(-1e9f, -1e9f, -1e9f);
   mWorkingQueryBox.maxExtents.set(-1e9f, -1e9f, -1e9f);

   return true;
}

void StockBody::getState(PhysicsState * outState)
{
   MatrixF trans;

   getTransform(&trans);

   outState->position = trans.getPosition();
   outState->orientation.set(trans);
   outState->linVelocity = getLinVelocity();
   outState->angVelocity = getAngVelocity();

   outState->momentum = (1.0f / (1 / mMass)) * outState->linVelocity;
   outState->sleeping = mSleep;

}

Point3F StockBody::getCMassPosition() const
{
   return mCenterOfMass;
}

void StockBody::setMaterial(F32 mRestitution, F32 mFriction, F32 staticFriction)
{
   mRestitution = mRestitution;
   mFriction = mFriction;
   mStaticFriction = staticFriction;
}

void StockBody::setCMassTransform(const MatrixF & xfm)
{
   mCenterOfMass = xfm.getPosition();
}

/*void StockBody::setTransform(const MatrixF& xfm)
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
}*/

MatrixF& StockBody::getTransform(MatrixF *outMatrix)
{
   /*if (mInvCenterOfMass)
      outMatrix->mul(*mInvCenterOfMass, *mWorldCenterOfMass);
   else*/
      outMatrix->setPosition(mWorldCenterOfMass);

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

void StockBody::applyCorrection(const MatrixF & xfm)
{
   if (mCenterOfMass)
   {
      MatrixF trans;
      trans.mul(xfm, *mCenterOfMass);
      setCMassTransform(trans);
   }
   else
      setCMassTransform(xfm);
}

void StockBody::applyDamping(F32 delta)
{
   mLinVelocity *= mPow((1.0f - mLinDamp), delta);
   mAngVelocity *= mPow((1.0f - mAngDamp), delta);
}

void StockBody::applyTorque(const Point3F& torque)
{
   Point3F tv;
   mInvWorldInertia.mulV((torque *mAngFactor), &tv);
   mTorque += tv;
}

void StockBody::applyForce(const Point3F& force)
{
   mForce += force * mLinFactor;
}

void StockBody::applyImpulse(const Point3F& r, const Point3F& impulse)
{
   mSleep = false;

   mLinVelocity += impulse * mLinFactor * mOneOverMass;
   Point3F tv;
   mCross(r, impulse, &tv);

   applyTorqueImpulse(mLinFactor * tv);
}

void StockBody::applyTorqueImpulse(const Point3F& torque)
{
   Point3F tv;
   mInvWorldInertia.mulV(torque * mAngFactor, &tv);
   mAngVelocity += tv;
}

void StockBody::clearForces()
{
   mForce = Point3F::Zero;
   mTorque = Point3F::Zero;
}

void StockBody::findContact(SceneObject** contactObject, VectorF* contactNormal, Vector<SceneObject*>* outOverlapObjects) const
{
   this->findContact(contactObject, contactNormal, outOverlapObjects);
}

void StockBody::_findContact(SceneObject** contactObject, VectorF* contactNormal, Vector<SceneObject*>* outOverlapObjects)
{
   MatrixF bodyTransform;
   Point3F pos;

   mAngPosition.setMatrix(&bodyTransform);
   bodyTransform.setColumn(3, mLinPosition);

   bodyTransform.getColumn(3, &pos);

   Box3F mScaledBox = mConvex.getBoundingBox();

   Box3F wBox;
   Point3F exp(0, 0, sTractionDistance);
   wBox.minExtents = pos + mScaledBox.minExtents - exp;
   wBox.maxExtents.x = pos.x + mScaledBox.maxExtents.x;
   wBox.maxExtents.y = pos.y + mScaledBox.maxExtents.y;
   wBox.maxExtents.z = pos.z + mScaledBox.minExtents.z + sTractionDistance;

   static ClippedPolyList polyList;
   polyList.clear();
   polyList.doConstruct();
   polyList.mNormal.set(0.0f, 0.0f, 0.0f);
   polyList.setInterestNormal(Point3F(0.0f, 0.0f, -1.0f));

   polyList.mPlaneList.setSize(6);
   polyList.mPlaneList[0].setYZ(wBox.minExtents, -1.0f);
   polyList.mPlaneList[1].setXZ(wBox.maxExtents, 1.0f);
   polyList.mPlaneList[2].setYZ(wBox.maxExtents, 1.0f);
   polyList.mPlaneList[3].setXZ(wBox.minExtents, -1.0f);
   polyList.mPlaneList[4].setXY(wBox.minExtents, -1.0f);
   polyList.mPlaneList[5].setXY(wBox.maxExtents, 1.0f);
   Box3F plistBox = wBox;

   // Expand build box as it will be used to collide with items.
   // PickupRadius will be at least the size of the box.
   /*F32 pd = (F32)mDataBlock->pickupDelta;
   wBox.minExtents.x -= pd; wBox.minExtents.y -= pd;
   wBox.maxExtents.x += pd; wBox.maxExtents.y += pd;
   wBox.maxExtents.z = pos.z + mScaledBox.maxExtents.z;*/

   // Build list from convex states here...
   CollisionWorkingList& rList = mConvex.getWorkingList();
   CollisionWorkingList* pList = rList.wLink.mNext;
   while (pList != &rList)
   {
      Convex* pConvex = pList->mConvex;

      U32 objectMask = pConvex->getObject()->getTypeMask();

      if ((objectMask & sCollisionMoveMask) &&
         !(objectMask & PhysicalZoneObjectType))
      {
         Box3F convexBox = pConvex->getBoundingBox();
         if (plistBox.isOverlapped(convexBox))
            pConvex->getPolyList(&polyList);
      }
      else
         outOverlapObjects->push_back(pConvex->getObject());

      pList = pList->wLink.mNext;
   }

   if (!polyList.isEmpty())
   {
      // Pick flattest surface
      F32 bestVd = -1.0f;
      ClippedPolyList::Poly* poly = polyList.mPolyList.begin();
      ClippedPolyList::Poly* end = polyList.mPolyList.end();
      for (; poly != end; poly++)
      {
         F32 vd = poly->plane.z;       // i.e.  mDot(Point3F(0,0,1), poly->plane);
         if (vd > bestVd)
         {
            bestVd = vd;
            *contactObject = poly->object;
            *contactNormal = poly->plane;
         }
      }
   }
}

void StockBody::moveKinematicTo(const MatrixF& xfm)
{

}

bool StockBody::castRay(const Point3F& start, const Point3F& end, RayInfo* info)
{
   if (mColShape && mColShape->getConvexList())
   {
      Convex* convexList = mColShape->getConvexList();
   }

   return false;
}

void StockBody::updateWorkingCollisionSet()
{
   // It is assumed that we will never accelerate more than 10 m/s for gravity...
   //
   //Point3F scaledVelocity = mLinVelocity.len * dt;
   

   // Check to see if it is actually necessary to construct the new working list,
   //  or if we can use the cached version from the last query.  We use the x
   //  component of the min member of the mWorkingQueryBox, which is lame, but
   //  it works ok.
   bool updateSet = false;
   /// predicted transform
   F32 len = (mLinVelocity.len() + 50) * TickSec;
   MatrixF transform;
   getTransform(&transform);
   transform.setPosition(transform.getPosition() + mLinVelocity * TickSec);
   //Box3F convexBox = mColShape->getConvexList()->getBoundingBox(transform, mUserData.getObject()->getScale());
   /// make convex box from shapes AABB.
   Box3F convexBox = mAABB;
   /// move it to the predicted position.
   convexBox.setCenter(transform.getPosition());
   F32 l = (len * 1.1f) + 0.1f;  // from Convex::updateWorkingList
   const Point3F  lPoint(l, l, l);
   convexBox.minExtents -= lPoint;
   convexBox.maxExtents += lPoint;

   // Check containment
   if (mWorkingQueryBox.minExtents.x != -1e9f)
   {
      if (mWorkingQueryBox.isContained(convexBox) == false)
         // Needed region is outside the cached region.  Update it.
         updateSet = true;
   }
   else
   {
      // Must update
      updateSet = true;
   }
   // Actually perform the query, if necessary
   if (updateSet == true)
   {
      const Point3F  twolPoint(2.0f * l, 2.0f * l, 2.0f * l);
      mWorkingQueryBox = convexBox;
      mWorkingQueryBox.minExtents -= twolPoint;
      mWorkingQueryBox.maxExtents += twolPoint;

      //disableCollision();

      //We temporarily disable the collisions of anything mounted to us so we don't accidentally walk into things we've attached to us
      /*for (SceneObject* ptr = mMount.list; ptr; ptr = ptr->getMountLink())
      {
         ptr->disableCollision();
      }*/

      mConvex.updateWorkingList(mWorkingQueryBox, sCollisionMoveMask
         /*isGhost() ? sClientCollisionContactMask : sServerCollisionContactMask*/);

      //And now re-enable the collisions of the mounted things
      /*for (SceneObject* ptr = mMount.list; ptr; ptr = ptr->getMountLink())
      {
         ptr->enableCollision();
      }

      enableCollision();*/
   }
}

void StockBody::updateForces(F32 dt)
{

   mLinVelocity += mForce * (mOneOverMass * dt);
   Point3F torque;

   mInvWorldInertia.mulV(mTorque * dt, &torque);
   mAngVelocity += torque;

   // If we're still mSleep, make sure we're not accumulating anything
   if (mSleep)
      setSleep();
}

void StockBody::updatePos(F32 dt)
{
   PROFILE_SCOPE(RigidShape_UpdatePos);

   Point3F origVelocity = mLinVelocity;

   updateForces(dt);

   // Update collision information based on our current pos.
   bool collided = false;
   if (!mSleep && mIsEnabled)
   {
      collided = updateCollision(dt);

      // Now that all the forces have been processed, lets
      // see if we're at rest.  Basically, if the kinetic energy of
      // the rigid body is less than some percentage of the energy added
      // by gravity for a short period, we're considered at rest.
      // This should really be part of the rigid class...
      if (mCollisionList.getCount())
      {
         F32 k = getKineticEnergy();
         F32 G = mWorld->getGravity().z * dt;
         F32 Kg = 0.5 * mMass * G * G;
         if (k < sRestTol * Kg && ++mSleepCount > sRestCount)
            setSleep();
      }
      else
         mSleepCount = 0;
   }

   // Integrate forward
   if (!mSleep && mIsEnabled)
      integrate(dt);

   // Deal with client and server scripting, sounds, etc.
   /*if (mWorld->isServer())
   {
      // Check triggers and other objects that we normally don't
      // collide with.  This function must be called before notifyCollision
      // as it will queue collision.
      checkTriggers();

      // Invoke the onCollision notify callback for all the objects
      // we've just hit.
      notifyCollision();

      // Server side impact script callback
      if (collided)
      {
         VectorF collVec = mLinVelocity - origVelocity;
         F32 collSpeed = collVec.len();
         if (collSpeed > mDataBlock->minImpactSpeed)
            onImpact(collVec);
      }

      // Water script callbacks
      if (!inLiquid && mWaterCoverage != 0.0f)
      {
         mDataBlock->onEnterLiquid_callback(this, mWaterCoverage, mLiquidType.c_str());
         inLiquid = true;
      }
      else if (inLiquid && mWaterCoverage == 0.0f)
      {
         mDataBlock->onLeaveLiquid_callback(this, mLiquidType.c_str());
         inLiquid = false;
      }

   }
   else {

      // Play impact sounds on the client.
      if (collided) {
         F32 collSpeed = (mLinVelocity - origVelocity).len();
         S32 impactSound = -1;
         if (collSpeed >= mDataBlock->hardImpactSpeed)
            impactSound = RigidShapeData::Body::HardImpactSound;
         else
            if (collSpeed >= mDataBlock->softImpactSpeed)
               impactSound = RigidShapeData::Body::SoftImpactSound;

         if (impactSound != -1 && mDataBlock->body.sound[impactSound] != NULL)
            SFX->playOnce(mDataBlock->body.sound[impactSound], &getTransform());
      }

      // Water volume sounds
      F32 vSpeed = getVelocity().len();
      if (!inLiquid && mWaterCoverage >= 0.8f) {
         if (vSpeed >= mDataBlock->hardSplashSoundVel)
            SFX->playOnce(mDataBlock->waterSound[RigidShapeData::ImpactHard], &getTransform());
         else
            if (vSpeed >= mDataBlock->medSplashSoundVel)
               SFX->playOnce(mDataBlock->waterSound[RigidShapeData::ImpactMedium], &getTransform());
            else
               if (vSpeed >= mDataBlock->softSplashSoundVel)
                  SFX->playOnce(mDataBlock->waterSound[RigidShapeData::ImpactSoft], &getTransform());
         inLiquid = true;
      }
      else
         if (inLiquid && mWaterCoverage < 0.8f) {
            if (vSpeed >= mDataBlock->exitSplashSoundVel)
               SFX->playOnce(mDataBlock->waterSound[RigidShapeData::ExitWater], &getTransform());
            inLiquid = false;
         }
   }*/
}

bool StockBody::updateCollision(F32 dt)
{
   // Update collision information
   MatrixF mat, cmat;
   mat = mConvex.getTransform();
   getTransform(&mat);
   cmat = mConvex.getTransform();

   mCollisionList.clear();
   CollisionState* state = mConvex.findClosestState(cmat, mUserData.getObject()->getScale(), sCollisionTol);
   if (state && state->mDist <= sCollisionTol)
   {
      //resolveDisplacement(ns,state,dt);
      mConvex.getCollisionInfo(cmat, mUserData.getObject()->getScale(), &mCollisionList, sCollisionTol);
   }

   // Resolve collisions
   bool collided = resolveCollision(mCollisionList);
   resolveContacts(mCollisionList, dt);
   return collided;
}

bool StockBody::resolveCollision(CollisionList& cList)
{
   // Apply impulses to resolve collision
   bool collided = false;
   for (S32 i = 0; i < cList.getCount(); i++)
   {
      Collision& c = cList[i];
      if (c.distance < sCollisionTol)
      {
         // Velocity into surface
         Point3F v, r;
         getOriginVector(c.point, &r);
         getVelocity(r, &v);
         F32 vn = mDot(v, c.normal);

         // Only interested in velocities greater than sContactTol,
         // velocities less than that will be dealt with as contacts
         // "constraints".
         if (vn < -sContactTol)
         {

            // Apply impulses to the rigid body to keep it from
            // penetrating the surface.
            if (c.object->getTypeMask() & VehicleObjectType)
            {
               StockBody* otherBody = dynamic_cast<StockBody*>(c.object);
               if (otherBody)
                  resolveCollision(cList[i].point, cList[i].normal, otherBody);
               else
                  resolveCollision(cList[i].point, cList[i].normal);
            }
            else resolveCollision(cList[i].point, cList[i].normal);
            collided = true;

            // Keep track of objects we collide with
            if (!mWorld->isServer() && c.object->getTypeMask() & ShapeBaseObjectType)
            {
               ShapeBase* col = static_cast<ShapeBase*>(c.object);
               queueCollision(col, v - col->getVelocity());
            }
         }
      }
   }

   return collided;
}

void StockBody::queueCollision(SceneObject* obj, const VectorF& vec)
{
   // Add object to list of collisions.
   SimTime time = Sim::getCurrentTime();
   S32 num = obj->getId();

   CollisionTimeout** adr = &mTimeoutList;
   CollisionTimeout* ptr = mTimeoutList;
   while (ptr) {
      if (ptr->objectNumber == num) {
         if (ptr->expireTime < time) {
            ptr->expireTime = time + CollisionTimeoutValue;
            ptr->object = obj;
            ptr->vector = vec;
         }
         return;
      }
      // Recover expired entries
      if (ptr->expireTime < time) {
         CollisionTimeout* cur = ptr;
         *adr = ptr->next;
         ptr = ptr->next;
         cur->next = sFreeTimeoutList;
         sFreeTimeoutList = cur;
      }
      else {
         adr = &ptr->next;
         ptr = ptr->next;
      }
   }

   // New entry for the object
   if (sFreeTimeoutList != NULL)
   {
      ptr = sFreeTimeoutList;
      sFreeTimeoutList = ptr->next;
      ptr->next = NULL;
   }
   else
   {
      ptr = sTimeoutChunker.alloc();
   }

   ptr->object = obj;
   ptr->objectNumber = obj->getId();
   ptr->vector = vec;
   ptr->expireTime = time + CollisionTimeoutValue;
   ptr->next = mTimeoutList;

   mTimeoutList = ptr;
}

void StockBody::notifyCollision()
{
   // Notify all the objects that were just stamped during the queueing
   // process.
   /*SimTime expireTime = Sim::getCurrentTime() + CollisionTimeoutValue;
   for (CollisionTimeout* ptr = mTimeoutList; ptr; ptr = ptr->next)
   {
      if (ptr->expireTime == expireTime && ptr->object)
      {
         SimObjectPtr<SceneObject> safePtr(ptr->object);
         SimObjectPtr<ShapeBase> safeThis(this);
         onCollision(ptr->object, ptr->vector);
         ptr->object = 0;

         if (!bool(safeThis))
            return;

         if (bool(safePtr))
            safePtr->onCollision(this, ptr->vector);

         if (!bool(safeThis))
            return;
      }
   }*/
}

//----------------------------------------------------------------------------
/** Resolve contact forces
Resolve contact forces using the "penalty" method. Forces are generated based
on the depth of penetration and the moment of inertia at the point of contact.
*/
bool StockBody::resolveContacts(CollisionList& cList, F32 dt)
{
   // Use spring forces to manage contact constraints.
   bool collided = false;
   Point3F t, p(0, 0, 0), l(0, 0, 0);
   for (S32 i = 0; i < cList.getCount(); i++)
   {
      const Collision& c = cList[i];
      if (c.distance < sCollisionTol)
      {
         // Velocity into the surface
         Point3F v, r;
         getOriginVector(c.point, &r);
         getVelocity(r, &v);
         F32 vn = mDot(v, c.normal);

         // Only interested in velocities less than mDataBlock->contactTol,
         // velocities greater than that are dealt with as collisions.
         if (mFabs(vn) < sContactTol)
         {
            collided = true;

            // Penetration mForce. This is actually a spring which
            // will seperate the body from the collision surface.
            F32 zi = 2 * mFabs(getZeroImpulse(r, c.normal));
            F32 s = (sCollisionTol - c.distance) * zi - ((vn / sContactTol) * zi);
            Point3F f = c.normal * s;

            // mFriction impulse, calculated as a function of the
            // amount of mForce it would take to stop the motion
            // perpendicular to the normal.
            Point3F uv = v - (c.normal * vn);
            F32 ul = uv.len();
            if (s > 0 && ul)
            {
               uv /= -ul;
               F32 u = ul * getZeroImpulse(r, uv);
               s *= mFriction;
               if (u > s)
                  u = s;
               f += uv * u;
            }

            // Accumulate forces
            p += f;
            mCross(r, f, &t);
            l += t;
         }
      }
   }

   // Contact constraint forces act over time...
   mLinMomentum += p * dt;
   mAngMomentum += l * dt;
   updateVelocity();
   return true;
}

void StockBody::checkTriggers()
{
   /*MatrixF transform;
   getTransform(&transform);
   Box3F bbox = mColShape->getConvexList()->getBoundingBox(transform, getScale());
   gServerContainer.findObjects(bbox, sTriggerMask, findCallback, this);*/
}

//----------------------------------------------------------------------------
//Rigid sim functions
//----------------------------------------------------------------------------

void StockBody::integrate(F32 delta)
{
   // Update Angular position
   F32 angle = mAngVelocity.len();
   if (angle != 0.0f) {
      QuatF dq;
      F32 sinHalfAngle;
      mSinCos(angle * delta * -0.5f, sinHalfAngle, dq.w);
      sinHalfAngle *= 1.0f / angle;
      dq.x = mAngVelocity.x * sinHalfAngle;
      dq.y = mAngVelocity.y * sinHalfAngle;
      dq.z = mAngVelocity.z * sinHalfAngle;
      QuatF tmp = mAngPosition;
      mAngPosition.mul(tmp, dq);
      mAngPosition.normalize();

      // Rotate the position around the center of mMass
      Point3F lp = mLinPosition - mWorldCenterOfMass;
      dq.mulP(lp, &mLinPosition);
      mLinPosition += mWorldCenterOfMass;
   }

   // Update angular momentum
   mAngMomentum = mAngMomentum + mTorque * delta;

   // Update linear position, momentum
   mLinPosition = mLinPosition + mLinVelocity * delta;
   mLinMomentum = mLinMomentum + mForce * delta;
   mLinVelocity = mLinMomentum * mOneOverMass;

   // Update dependent state variables
   updateInertialTensor();
   updateVelocity();
   updateCenterOfMass();
}

void StockBody::updateVelocity()
{
   mLinVelocity.x = mLinMomentum.x * mOneOverMass;
   mLinVelocity.y = mLinMomentum.y * mOneOverMass;
   mLinVelocity.z = mLinMomentum.z * mOneOverMass;
   mInvWorldInertia.mulV(mAngMomentum, &mAngVelocity);
}

void StockBody::updateInertialTensor()
{
   MatrixF iv, qmat;
   mAngPosition.setMatrix(&qmat);
   iv.mul(qmat, mInvObjectInertia);
   qmat.transpose();
   mInvWorldInertia.mul(iv, qmat);
}

void StockBody::updateCenterOfMass()
{
   // Move the center of mMass into world space
   mAngPosition.mulP(mCenterOfMass, &mWorldCenterOfMass);
   mWorldCenterOfMass += mLinPosition;
}

//-----------------------------------------------------------------------------
/** Resolve collision with another rigid body
   Computes & applies the collision impulses needed to keep the bodies
   from interpenetrating.

   tg: This function was commented out... I uncommented it, but haven't
   double checked the math.
*/
bool StockBody::resolveCollision(const Point3F& p, const Point3F& normal, StockBody* rigid)
{
   mSleep = false;
   Point3F v1, v2, r1, r2;
   getOriginVector(p, &r1);
   getVelocity(r1, &v1);
   rigid->getOriginVector(p, &r2);
   rigid->getVelocity(r2, &v2);

   // Make sure they are converging
   F32 nv = mDot(v1, normal);
   nv -= mDot(v2, normal);
   if (nv > 0.0f)
      return false;

   // Compute impulse
   F32 d, n = -nv * (2.0f + mRestitution * rigid->mRestitution);
   Point3F a1, b1, c1;
   mCross(r1, normal, &a1);
   mInvWorldInertia.mulV(a1, &b1);
   mCross(b1, r1, &c1);

   Point3F a2, b2, c2;
   mCross(r2, normal, &a2);
   rigid->mInvWorldInertia.mulV(a2, &b2);
   mCross(b2, r2, &c2);

   Point3F c3 = c1 + c2;
   d = mOneOverMass + rigid->mOneOverMass + mDot(c3, normal);
   Point3F impulse = normal * (n / d);

   applyImpulse(r1, impulse);
   impulse.neg();
   rigid->applyImpulse(r2, impulse);
   return true;
}

//-----------------------------------------------------------------------------
/** Resolve collision with an immovable object
   Computes & applies the collision impulse needed to keep the body
   from penetrating the given surface.
*/
bool StockBody::resolveCollision(const Point3F& p, const Point3F& normal)
{
   mSleep = false;
   Point3F v, r;
   getOriginVector(p, &r);
   getVelocity(r, &v);
   F32 n = -mDot(v, normal);
   if (n >= 0.0f) {

      // Collision impulse, straight forward mForce stuff.
      F32 d = getZeroImpulse(r, normal);
      F32 j = n * (1.0f + mRestitution) * d;
      Point3F impulse = normal * j;

      // mFriction impulse, calculated as a function of the
      // amount of mForce it would take to stop the motion
      // perpendicular to the normal.
      Point3F uv = v + (normal * n);
      F32 ul = uv.len();
      if (ul) {
         uv /= -ul;
         F32 u = ul * getZeroImpulse(r, uv);
         j *= mFriction;
         if (u > j)
            u = j;
         impulse += uv * u;
      }

      //
      applyImpulse(r, impulse);
   }
   return true;
}

//-----------------------------------------------------------------------------
/** Calculate the inertia along the given vector
   This function can be used to calculate the amount of mForce needed to
   affect a change in velocity along the specified normal applied at
   the given point.
*/
F32 StockBody::getZeroImpulse(const Point3F& r, const Point3F& normal)
{
   Point3F a, b, c;
   mCross(r, normal, &a);
   mInvWorldInertia.mulV(a, &b);
   mCross(b, r, &c);
   return 1 / (mOneOverMass + mDot(c, normal));
}

F32 StockBody::getKineticEnergy()
{
   Point3F w;
   QuatF qmat = mAngPosition;
   qmat.inverse();
   qmat.mulP(mAngVelocity, &w);
   const F32* f = mInvObjectInertia;
   return 0.5f * ((mMass * mDot(mLinVelocity, mLinVelocity)) +
      w.x * w.x / f[0] +
      w.y * w.y / f[5] +
      w.z * w.z / f[10]);
}

void StockBody::getOriginVector(const Point3F& p, Point3F* r)
{
   *r = p - mWorldCenterOfMass;
}

void StockBody::setCenterOfMass(const Point3F& newCenter)
{
   // Sets the center of mass relative to the origin.
   mCenterOfMass = newCenter;

   // Update world center of mass
   mAngPosition.mulP(mCenterOfMass, &mWorldCenterOfMass);
   mWorldCenterOfMass += mLinPosition;
}

void StockBody::translateCenterOfMass(const Point3F& oldPos, const Point3F& newPos)
{
   // I + mMass * (crossmatrix(mCenterOfMass)^2 - crossmatrix(newCenter)^2)
   MatrixF oldx, newx;
   oldx.setCrossProduct(oldPos);
   newx.setCrossProduct(newPos);
   for (S32 row = 0; row < 3; row++)
      for (S32 col = 0; col < 3; col++) {
         F32 n = newx(row, col), o = oldx(row, col);
         mObjectInertia(row, col) += mMass * ((o * o) - (n * n));
      }

   // Make sure the matrix is symetrical
   mObjectInertia(1, 0) = mObjectInertia(0, 1);
   mObjectInertia(2, 0) = mObjectInertia(0, 2);
   mObjectInertia(2, 1) = mObjectInertia(1, 2);
}

void StockBody::getVelocity(const Point3F& r, Point3F* v)
{
   mCross(mAngVelocity, r, v);
   *v += mLinVelocity;
}

MatrixF StockBody::getTransform()
{
   MatrixF transform;
   mAngPosition.setMatrix(&transform);
   transform.setColumn(3, mLinPosition);

   return transform;
}

void StockBody::setTransform(const MatrixF& mat)
{
   mAngPosition.set(mat);
   mat.getColumn(3, &mLinPosition);

   // Update center of mMass
   mAngPosition.mulP(mCenterOfMass, &mWorldCenterOfMass);
   mWorldCenterOfMass += mLinPosition;

   mAABB = mUserData.getObject()->getWorldBox();
   mOOBB = mUserData.getObject()->getObjBox();

}


//----------------------------------------------------------------------------
/** Set the rigid body moment of inertia
   The moment is calculated as a box with the given dimensions.
*/
void StockBody::setObjectInertia(const Point3F& r)
{
   // Rotational moment of inertia of a box
   F32 ot = mMass / 12.0f;
   F32 a = r.x * r.x;
   F32 b = r.y * r.y;
   F32 c = r.z * r.z;

   mObjectInertia.identity();
   F32* f = mObjectInertia;
   f[0] = ot * (b + c);
   f[5] = ot * (c + a);
   f[10] = ot * (a + b);

   invertObjectInertia();
   updateInertialTensor();
}


//----------------------------------------------------------------------------
/** Set the rigid body moment of inertia
   The moment is calculated as a unit sphere.
*/
void StockBody::setObjectInertia()
{
   mObjectInertia.identity();
   F32 radius = 1.0f;
   F32* f = mObjectInertia;
   f[0] = f[5] = f[10] = (0.4f * mMass * radius * radius);
   invertObjectInertia();
   updateInertialTensor();
}

void StockBody::invertObjectInertia()
{
   mInvObjectInertia = mObjectInertia;
   mInvObjectInertia.fullInverse();
}


//----------------------------------------------------------------------------
bool StockBody::checkSleepCondition()
{
   //   F32 k = getKineticEnergy(mWorldToObj);
   //   F32 G = -mForce.z * mOneOverMass * 0.032;
   //   F32 Kg = 0.5 * mMass * G * G;
   //   if (k < Kg * restTol)
   //      setAtRest();
   return mSleep;
}

void StockBody::setSleep()
{
   mSleep = true;
   mLinVelocity.set(0.0f, 0.0f, 0.0f);
   mLinMomentum.set(0.0f, 0.0f, 0.0f);
   mAngVelocity.set(0.0f, 0.0f, 0.0f);
   mAngMomentum.set(0.0f, 0.0f, 0.0f);
}
