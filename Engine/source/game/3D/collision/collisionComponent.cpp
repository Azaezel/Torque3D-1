#include "collisionComponent.h"
#include "gfx/gfxDrawUtil.h"
#include "game/Entity.h"
#include <gfx/gfxTransformSaver.h>
#include "scene/sceneRenderState.h"
#include "renderInstance/renderPassManager.h"
#include "materials/baseMatInstance.h"

#include "T3D/trigger.h"
#include "ts/tsShapeInstance.h"

#include "collision/extrudedPolyList.h"
#include "opcode/Opcode.h"
#include "opcode/Ice/IceAABB.h"
#include "opcode/Ice/IcePoint.h"
#include "opcode/OPC_AABBTree.h"
#include "opcode/OPC_AABBCollider.h"
#include "collision/clippedPolyList.h"

IMPLEMENT_CO_DATABLOCK_V1(CollisionComponent);

IMPL_COMP_REGISTER_SIGNALS(CollisionComponent);

CollisionComponent::CollisionComponent() : Component()
{
}

bool CollisionComponent::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

void CollisionComponent::consoleInit()
{
   Parent::consoleInit();

   //We'll register the CollisionDirector to the DirectorManager, so it's ready to go at runtime
   DirectorManager::get()->mDirectors.push_back(new CollisionDirector());
}

void CollisionComponent::initPersistFields()
{
   Parent::initPersistFields();
}

void CollisionComponent::packData(BitStream* stream)
{
   Parent::packData(stream);
}

void CollisionComponent::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);
}

ComponentInstance* CollisionComponent::createInstance(ComponentObject* owner)
{
   CollisionComponentInstance* compInst = new CollisionComponentInstance(*this, *owner);

   if (!compInst->registerObject())
   {
      Con::errorf("CollisionComponent::createInstance() - failed to create instance");
      return nullptr;
   }

   setupFields(compInst, true);
   CollisionComponentInstance::sComponentInstanceList.push_back(compInst);

   return compInst;
}

//==================================================================================================
//
//==================================================================================================
IMPLEMENT_CONOBJECT(CollisionComponentInstance);

Vector< CollisionComponentInstance*> CollisionComponentInstance::sComponentInstanceList;

CollisionComponentInstance::CollisionComponentInstance(const CollisionComponent& componentData, const ComponentObject& owner)
{
   mComponentData = &componentData;
   mOwner = &owner;

   mBlockColliding = true;

   CollisionMoveMask = (TerrainObjectType | PlayerObjectType |
      StaticShapeObjectType | VehicleObjectType |
      VehicleBlockerObjectType | DynamicShapeObjectType | StaticObjectType | EntityObjectType | TriggerObjectType);

   mPhysicsRep = nullptr;
   mPhysicsWorld = nullptr;

   mTimeoutList = nullptr;
}

CollisionComponentInstance::~CollisionComponentInstance()
{
   SAFE_DELETE(mPhysicsRep);
}

void CollisionComponentInstance::initPersistFields()
{
   Parent::initPersistFields();
}

void CollisionComponentInstance::destroyInstance()
{
   CollisionComponentInstance::sComponentInstanceList.remove(this);

   delete this;
}

void CollisionComponentInstance::update()
{
}

//
U32 CollisionComponentInstance::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   return retMask;
}

void CollisionComponentInstance::unpackUpdate(NetConnection* con, BitStream* stream)
{
   Parent::unpackUpdate(con, stream);
}

#pragma region Collision
bool CollisionComponentInstance::checkCollisions(const F32 travelTime, Point3F* velocity, Point3F start)
{
   return false;
}

bool CollisionComponentInstance::updateCollisions(F32 time, VectorF vector, VectorF velocity)
{
   return false;
}

void CollisionComponentInstance::updateWorkingCollisionSet(const U32 mask)
{
}

void CollisionComponentInstance::handleCollisionList(CollisionList& collisionList, VectorF velocity)
{
   Collision bestCol;

   mCollisionList = collisionList;

   for (U32 i = 0; i < collisionList.getCount(); ++i)
   {
      Collision& colCheck = collisionList[i];

      if (colCheck.object)
      {
         if (colCheck.object->getTypeMask() & PlayerObjectType)
         {
            handleCollision(colCheck, velocity);
         }
         else if (colCheck.object->getTypeMask() & TriggerObjectType)
         {
            // We've hit it's bounding box, that's close enough for triggers
            Trigger* pTrigger = static_cast<Trigger*>(colCheck.object);

            Entity* ent = dynamic_cast<Entity*>(this->getOwnerObjectPtr());
            //pTrigger->potentialEnterObject(ent);
         }
         else if (colCheck.object->getTypeMask() & DynamicShapeObjectType)
         {
            Con::printf("HIT A GENERICALLY DYNAMIC OBJECT");
            handleCollision(colCheck, velocity);
         }
         else if (colCheck.object->getTypeMask() & EntityObjectType)
         {
            Entity* ent = dynamic_cast<Entity*>(colCheck.object);
            if (ent)
            {
               ComponentInstance* foundComp = ent->getComponentInstance(StringTable->insert("Collision"));
               if (foundComp != nullptr)
               {
                  CollisionComponentInstance* colObject = static_cast<CollisionComponentInstance*>(foundComp);

                  Entity* ent = dynamic_cast<Entity*>(this->getOwnerObjectPtr());
                  colObject->onCollisionSignal.trigger(ent);

                  //TODO: properly do this
                  Collision oppositeCol = colCheck;
                  oppositeCol.object = ent;

                  colObject->handleCollision(oppositeCol, velocity);
               }
            }
         }
         else
         {
            handleCollision(colCheck, velocity);
         }
      }
   }
}

void CollisionComponentInstance::handleCollision(Collision& col, VectorF velocity)
{
   if (col.object && (mContactInfo.contactObject == NULL ||
      col.object->getId() != mContactInfo.contactObject->getId()))
   {
      queueCollision(col.object, velocity - col.object->getVelocity());

      Entity* ent = dynamic_cast<Entity*>(getOwnerObjectPtr());

      //do the callbacks to script for this collision
      if (isMethod("onCollision"))
      {
         S32 matId = col.material != NULL ? col.material->getMaterial()->getId() : 0;
         Con::executef(this, "onCollision", col.object, col.normal, col.point, matId, velocity);
      }

      if (ent->isMethod("onCollisionEvent"))
      {
         S32 matId = col.material != NULL ? col.material->getMaterial()->getId() : 0;
         Con::executef(ent, "onCollisionEvent", col.object, col.normal, col.point, matId, velocity);
      }
   }
}

void CollisionComponentInstance::handleCollisionNotifyList()
{
   //special handling for any collision components we should notify that a collision happened.
   for (U32 i = 0; i < mCollisionNotifyList.size(); ++i)
   {
      Entity* ent = dynamic_cast<Entity*>(getOwnerObjectPtr());
      mCollisionNotifyList[i]->onCollisionSignal.trigger(ent);
   }

   mCollisionNotifyList.clear();
}

Chunker<CollisionComponentInstance::CollisionTimeout> sCollisionTimeoutChunker;
CollisionComponentInstance::CollisionTimeout* CollisionComponentInstance::sFreeTimeoutList = 0;

void CollisionComponentInstance::queueCollision(SceneObject* obj, const VectorF& vec)
{
   // Add object to list of collisions.
   SimTime time = Sim::getCurrentTime();
   S32 num = obj->getId();

   CollisionTimeout** adr = &mTimeoutList;
   CollisionTimeout* ptr = mTimeoutList;
   while (ptr)
   {
      if (ptr->objectNumber == num)
      {
         if (ptr->expireTime < time)
         {
            ptr->expireTime = time + CollisionTimeoutValue;
            ptr->object = obj;
            ptr->vector = vec;
         }
         return;
      }
      // Recover expired entries
      if (ptr->expireTime < time)
      {
         CollisionTimeout* cur = ptr;
         *adr = ptr->next;
         ptr = ptr->next;
         cur->next = sFreeTimeoutList;
         sFreeTimeoutList = cur;
      }
      else
      {
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
      ptr = sCollisionTimeoutChunker.alloc();
   }

   ptr->object = obj;
   ptr->objectNumber = obj->getId();
   ptr->vector = vec;
   ptr->expireTime = time + CollisionTimeoutValue;
   ptr->next = mTimeoutList;

   mTimeoutList = ptr;
}

bool CollisionComponentInstance::checkEarlyOut(Point3F start, VectorF velocity, F32 time, Box3F objectBox, Point3F objectScale,
   Box3F collisionBox, U32 collisionMask, CollisionWorkingList& colWorkingList)
{
   Point3F end = start + velocity * time;
   Point3F distance = end - start;

   Box3F scaledBox = objectBox;
   scaledBox.minExtents.convolve(objectScale);
   scaledBox.maxExtents.convolve(objectScale);

   if (mFabs(distance.x) < objectBox.len_x() &&
      mFabs(distance.y) < objectBox.len_y() &&
      mFabs(distance.z) < objectBox.len_z())
   {
      // We can potentially early out of this.  If there are no polys in the clipped polylist at our
      //  end position, then we can bail, and just set start = end;
      Box3F wBox = scaledBox;
      wBox.minExtents += end;
      wBox.maxExtents += end;

      static EarlyOutPolyList eaPolyList;
      eaPolyList.clear();
      eaPolyList.mNormal.set(0.0f, 0.0f, 0.0f);
      eaPolyList.mPlaneList.clear();
      eaPolyList.mPlaneList.setSize(6);
      eaPolyList.mPlaneList[0].set(wBox.minExtents, VectorF(-1.0f, 0.0f, 0.0f));
      eaPolyList.mPlaneList[1].set(wBox.maxExtents, VectorF(0.0f, 1.0f, 0.0f));
      eaPolyList.mPlaneList[2].set(wBox.maxExtents, VectorF(1.0f, 0.0f, 0.0f));
      eaPolyList.mPlaneList[3].set(wBox.minExtents, VectorF(0.0f, -1.0f, 0.0f));
      eaPolyList.mPlaneList[4].set(wBox.minExtents, VectorF(0.0f, 0.0f, -1.0f));
      eaPolyList.mPlaneList[5].set(wBox.maxExtents, VectorF(0.0f, 0.0f, 1.0f));

      // Build list from convex states here...
      CollisionWorkingList& rList = colWorkingList;
      CollisionWorkingList* pList = rList.wLink.mNext;
      while (pList != &rList)
      {
         Convex* pConvex = pList->mConvex;

         if (pConvex->getObject()->getTypeMask() & collisionMask)
         {
            Box3F convexBox = pConvex->getBoundingBox();

            if (wBox.isOverlapped(convexBox))
            {
               // No need to separate out the physical zones here, we want those
               //  to cause a fallthrough as well...
               pConvex->getPolyList(&eaPolyList);
            }
         }
         pList = pList->wLink.mNext;
      }

      if (eaPolyList.isEmpty())
      {
         return true;
      }
   }

   return false;
}


Collision* CollisionComponentInstance::getCollision(S32 col)
{
   if (col < mCollisionList.getCount() && col >= 0)
      return &mCollisionList[col];
   else
      return NULL;
}

Point3F CollisionComponentInstance::getContactNormal()
{
   return mContactInfo.contactNormal;
}

bool CollisionComponentInstance::hasContact()
{
   if (mContactInfo.contactObject)
      return true;
   else
      return false;
}

S32 CollisionComponentInstance::getCollisionCount()
{
   return mCollisionList.getCount();
}

Point3F CollisionComponentInstance::getCollisionNormal(S32 collisionIndex)
{
   if (collisionIndex < 0 || mCollisionList.getCount() < collisionIndex)
      return Point3F::Zero;

   return mCollisionList[collisionIndex].normal;
}

F32 CollisionComponentInstance::getCollisionAngle(S32 collisionIndex, Point3F upVector)
{
   if (collisionIndex < 0 || mCollisionList.getCount() < collisionIndex)
      return 0.0f;

   return mRadToDeg(mAcos(mDot(mCollisionList[collisionIndex].normal, upVector)));
}

S32 CollisionComponentInstance::getBestCollision(Point3F upVector)
{
   S32 bestCollision = -1;

   F32 bestAngle = 360.f;
   S32 count = mCollisionList.getCount();
   for (U32 i = 0; i < count; ++i)
   {
      F32 angle = mRadToDeg(mAcos(mDot(mCollisionList[i].normal, upVector)));

      if (angle < bestAngle)
      {
         bestCollision = i;
         bestAngle = angle;
      }
   }

   return bestCollision;
}

F32 CollisionComponentInstance::getBestCollisionAngle(VectorF upVector)
{
   S32 bestCol = getBestCollision(upVector);

   if (bestCol == -1)
      return 0;

   return getCollisionAngle(bestCol, upVector);
}

//
bool CollisionComponentInstance::buildConvexOpcode(TSShapeInstance* sI, S32 dl, const Box3F& bounds, Convex* c, Convex* list)
{
   AssertFatal(dl >= 0 && dl < sI->getShape()->details.size(), "TSShapeInstance::buildConvexOpcode");

   TSShape* shape = sI->getShape();

   // nothing emitted yet...
   bool emitted = false;

   /*const MatrixF& objMat = mOwner->getObjToWorld();
   const Point3F& objScale = mOwner->getScale();

   // get subshape and object detail
   const TSDetail* detail = &shape->details[dl];
   S32 ss = detail->subShapeNum;
   S32 od = detail->objectDetailNum;

   S32 start = shape->subShapeFirstObject[ss];
   S32 end = shape->subShapeNumObjects[ss] + start;
   if (start < end)
   {
      MatrixF initialMat = objMat;
      Point3F initialScale = objScale;

      // set up for first object's node
      MatrixF mat;
      MatrixF scaleMat(true);
      F32* p = scaleMat;
      p[0] = initialScale.x;
      p[5] = initialScale.y;
      p[10] = initialScale.z;
      const MatrixF* previousMat = &sI->mMeshObjects[start].getTransform();
      mat.mul(initialMat, scaleMat);
      mat.mul(*previousMat);

      // Update our bounding box...
      Box3F localBox = bounds;
      MatrixF otherMat = mat;
      otherMat.inverse();
      otherMat.mul(localBox);

      // run through objects and collide
      for (S32 i = start; i < end; i++)
      {
         TSShapeInstance::MeshObjectInstance* meshInstance = &sI->mMeshObjects[i];

         if (od >= meshInstance->object->numMeshes)
            continue;

         if (&meshInstance->getTransform() != previousMat)
         {
            // different node from before, set up for this node
            previousMat = &meshInstance->getTransform();

            if (previousMat != NULL)
            {
               mat.mul(initialMat, scaleMat);
               mat.mul(*previousMat);

               // Update our bounding box...
               otherMat = mat;
               otherMat.inverse();
               localBox = bounds;
               otherMat.mul(localBox);
            }
         }

         // collide... note we pass the original mech transform
         // here so that the convex data returned is in mesh space.
         TSMesh* mesh = meshInstance->getMesh(od);
         if (mesh && !meshInstance->forceHidden && meshInstance->visible > 0.01f && localBox.isOverlapped(mesh->getBounds()))
            emitted |= buildMeshOpcode(mesh, *previousMat, localBox, c, list);
         else
            emitted |= false;
      }
   }*/

   return emitted;
}

bool CollisionComponentInstance::buildMeshOpcode(TSMesh* mesh, const MatrixF& meshToObjectMat,
   const Box3F& nodeBox, Convex* convex, Convex* list)
{
   /*PROFILE_SCOPE(MeshCollider_buildConvexOpcode);

   // This is small... there is no win for preallocating it.
   Opcode::AABBCollider opCollider;
   opCollider.SetPrimitiveTests(true);

   // This isn't really needed within the AABBCollider as
   // we don't use temporal coherance... use a static to
   // remove the allocation overhead.
   static Opcode::AABBCache opCache;

   IceMaths::AABB opBox;
   opBox.SetMinMax(Point(nodeBox.minExtents.x, nodeBox.minExtents.y, nodeBox.minExtents.z),
      Point(nodeBox.maxExtents.x, nodeBox.maxExtents.y, nodeBox.maxExtents.z));
   Opcode::CollisionAABB opCBox(opBox);

   if (!opCollider.Collide(opCache, opCBox, *mesh->mOptTree))
      return false;

   U32 cnt = opCollider.GetNbTouchedPrimitives();
   const udword *idx = opCollider.GetTouchedPrimitives();

   Opcode::VertexPointers vp;
   for (S32 i = 0; i < cnt; i++)
   {
      // First, check our active convexes for a potential match (and clean things
      // up, too.)
      const U32 curIdx = idx[i];

      // See if the square already exists as part of the working set.
      bool gotMatch = false;
      CollisionWorkingList& wl = convex->getWorkingList();
      for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext)
      {
         if (itr->mConvex->getType() != TSPolysoupConvexType)
            continue;

         const MeshColliderPolysoupConvex *chunkc = static_cast<MeshColliderPolysoupConvex*>(itr->mConvex);

         if (chunkc->getObject() != mOwner)
            continue;

         if (chunkc->mesh != mesh)
            continue;

         if (chunkc->idx != curIdx)
            continue;

         // A match! Don't need to add it.
         gotMatch = true;
         break;
      }

      if (gotMatch)
         continue;

      // Get the triangle...
      mesh->mOptTree->GetMeshInterface()->GetTriangle(vp, idx[i]);

      Point3F a(vp.Vertex[0]->x, vp.Vertex[0]->y, vp.Vertex[0]->z);
      Point3F b(vp.Vertex[1]->x, vp.Vertex[1]->y, vp.Vertex[1]->z);
      Point3F c(vp.Vertex[2]->x, vp.Vertex[2]->y, vp.Vertex[2]->z);

      // Transform the result into object space!
      meshToObjectMat.mulP(a);
      meshToObjectMat.mulP(b);
      meshToObjectMat.mulP(c);

      //If we're not doing debug rendering on the client, then set up our convex list as normal
      PlaneF p(c, b, a);
      Point3F peak = ((a + b + c) / 3.0f) - (p * 0.15f);

      // Set up the convex...
      MeshColliderPolysoupConvex *cp = new MeshColliderPolysoupConvex();

      list->registerObject(cp);
      convex->addToWorkingList(cp);

      cp->mesh = mesh;
      cp->idx = curIdx;
      cp->mObject = mOwner;

      cp->normal = p;
      cp->verts[0] = a;
      cp->verts[1] = b;
      cp->verts[2] = c;
      cp->verts[3] = peak;

      // Update the bounding box.
      Box3F &bounds = cp->box;
      bounds.minExtents.set(F32_MAX, F32_MAX, F32_MAX);
      bounds.maxExtents.set(-F32_MAX, -F32_MAX, -F32_MAX);

      bounds.minExtents.setMin(a);
      bounds.minExtents.setMin(b);
      bounds.minExtents.setMin(c);
      bounds.minExtents.setMin(peak);

      bounds.maxExtents.setMax(a);
      bounds.maxExtents.setMax(b);
      bounds.maxExtents.setMax(c);
      bounds.maxExtents.setMax(peak);
   }

   return true;*/
   return false;
}

bool CollisionComponentInstance::castRayOpcode(S32 dl, const Point3F& startPos, const Point3F& endPos, RayInfo* info)
{
   // if dl==-1, nothing to do
   //if (dl == -1 || !getShapeInstance())
   return false;

   /*TSShape *shape = getShapeInstance()->getShape();

   AssertFatal(dl >= 0 && dl < shape->details.size(), "TSShapeInstance::castRayOpcode");

   info->t = 100.f;

   // get subshape and object detail
   const TSDetail * detail = &shape->details[dl];
   S32 ss = detail->subShapeNum;
   if (ss < 0)
      return false;

   S32 od = detail->objectDetailNum;

   // nothing emitted yet...
   bool emitted = false;

   const MatrixF* saveMat = NULL;
   S32 start = shape->subShapeFirstObject[ss];
   S32 end = shape->subShapeNumObjects[ss] + start;
   if (start<end)
   {
      MatrixF mat;
      const MatrixF * previousMat = &getShapeInstance()->mMeshObjects[start].getTransform();
      mat = *previousMat;
      mat.inverse();
      Point3F localStart, localEnd;
      mat.mulP(startPos, &localStart);
      mat.mulP(endPos, &localEnd);

      // run through objects and collide
      for (S32 i = start; i<end; i++)
      {
         TSShapeInstance::MeshObjectInstance * meshInstance = &getShapeInstance()->mMeshObjects[i];

         if (od >= meshInstance->object->numMeshes)
            continue;

         if (&meshInstance->getTransform() != previousMat)
         {
            // different node from before, set up for this node
            previousMat = &meshInstance->getTransform();

            if (previousMat != NULL)
            {
               mat = *previousMat;
               mat.inverse();
               mat.mulP(startPos, &localStart);
               mat.mulP(endPos, &localEnd);
            }
         }

         // collide...
         TSMesh * mesh = meshInstance->getMesh(od);
         if (mesh && !meshInstance->forceHidden && meshInstance->visible > 0.01f)
         {
            if (castRayMeshOpcode(mesh, localStart, localEnd, info, getShapeInstance()->mMaterialList))
            {
               saveMat = previousMat;
               emitted = true;
            }
         }
      }
   }

   if (emitted)
   {
      saveMat->mulV(info->normal);
      info->point = endPos - startPos;
      info->point *= info->t;
      info->point += startPos;
   }

   return emitted;*/
}

static Point3F	texGenAxis[18] =
{
   Point3F(0,0,1), Point3F(1,0,0), Point3F(0,-1,0),
   Point3F(0,0,-1), Point3F(1,0,0), Point3F(0,1,0),
   Point3F(1,0,0), Point3F(0,1,0), Point3F(0,0,1),
   Point3F(-1,0,0), Point3F(0,1,0), Point3F(0,0,-1),
   Point3F(0,1,0), Point3F(1,0,0), Point3F(0,0,1),
   Point3F(0,-1,0), Point3F(-1,0,0), Point3F(0,0,-1)
};

bool CollisionComponentInstance::castRayMeshOpcode(TSMesh* mesh, const Point3F& s, const Point3F& e, RayInfo* info, TSMaterialList* materials)
{
   Opcode::RayCollider ray;
   Opcode::CollisionFaces cfs;

   IceMaths::Point dir(e.x - s.x, e.y - s.y, e.z - s.z);
   const F32 rayLen = dir.Magnitude();
   IceMaths::Ray vec(Point(s.x, s.y, s.z), dir.Normalize());

   ray.SetDestination(&cfs);
   ray.SetFirstContact(false);
   ray.SetClosestHit(true);
   ray.SetPrimitiveTests(true);
   ray.SetCulling(true);
   ray.SetMaxDist(rayLen);

   AssertFatal(ray.ValidateSettings() == NULL, "invalid ray settings");

   // Do collision.
   bool safety = ray.Collide(vec, *mesh->mOptTree);
   AssertFatal(safety, "CollisionComponent::castRayOpcode - no good ray collide!");

   // If no hit, just skip out.
   if (cfs.GetNbFaces() == 0)
      return false;

   // Got a hit!
   AssertFatal(cfs.GetNbFaces() == 1, "bad");
   const Opcode::CollisionFace& face = cfs.GetFaces()[0];

   // If the cast was successful let's check if the t value is less than what we had
   // and toggle the collision boolean
   // Stupid t... i prefer coffee
   const F32 t = face.mDistance / rayLen;

   if (t < 0.0f || t > 1.0f)
      return false;

   if (t <= info->t)
   {
      info->t = t;

      // Calculate the normal.
      Opcode::VertexPointers vp;
      mesh->mOptTree->GetMeshInterface()->GetTriangle(vp, face.mFaceID);

      if (materials && vp.MatIdx >= 0 && vp.MatIdx < materials->size())
         info->material = materials->getMaterialInst(vp.MatIdx);

      // Get the two edges.
      IceMaths::Point baseVert = *vp.Vertex[0];
      IceMaths::Point a = *vp.Vertex[1] - baseVert;
      IceMaths::Point b = *vp.Vertex[2] - baseVert;

      IceMaths::Point n;
      n.Cross(a, b);
      n.Normalize();

      info->normal.set(n.x, n.y, n.z);

      // generate UV coordinate across mesh based on 
      // matching normals, this isn't done by default and is 
      // primarily of interest in matching a collision point to 
      // either a GUI control coordinate or finding a hit pixel in texture space
      if (info->generateTexCoord)
      {
         baseVert = *vp.Vertex[0];
         a = *vp.Vertex[1];
         b = *vp.Vertex[2];

         Point3F facePoint = (1.0f - face.mU - face.mV) * Point3F(baseVert.x, baseVert.y, baseVert.z)
            + face.mU * Point3F(a.x, a.y, a.z) + face.mV * Point3F(b.x, b.y, b.z);

         U32 faces[1024];
         U32 numFaces = 0;
         for (U32 i = 0; i < mesh->mOptTree->GetMeshInterface()->GetNbTriangles(); i++)
         {
            if (i == face.mFaceID)
            {
               faces[numFaces++] = i;
            }
            else
            {
               IceMaths::Point n2;

               mesh->mOptTree->GetMeshInterface()->GetTriangle(vp, i);

               baseVert = *vp.Vertex[0];
               a = *vp.Vertex[1] - baseVert;
               b = *vp.Vertex[2] - baseVert;
               n2.Cross(a, b);
               n2.Normalize();

               F32 eps = .01f;
               if (mFabs(n.x - n2.x) < eps && mFabs(n.y - n2.y) < eps && mFabs(n.z - n2.z) < eps)
               {
                  faces[numFaces++] = i;
               }
            }

            if (numFaces == 1024)
            {
               // too many faces in this collision mesh for UV generation
               return true;
            }

         }

         Point3F min(F32_MAX, F32_MAX, F32_MAX);
         Point3F max(-F32_MAX, -F32_MAX, -F32_MAX);

         for (U32 i = 0; i < numFaces; i++)
         {
            mesh->mOptTree->GetMeshInterface()->GetTriangle(vp, faces[i]);

            for (U32 j = 0; j < 3; j++)
            {
               a = *vp.Vertex[j];

               if (a.x < min.x)
                  min.x = a.x;
               if (a.y < min.y)
                  min.y = a.y;
               if (a.z < min.z)
                  min.z = a.z;

               if (a.x > max.x)
                  max.x = a.x;
               if (a.y > max.y)
                  max.y = a.y;
               if (a.z > max.z)
                  max.z = a.z;

            }

         }

         // slerp
         Point3F s = ((max - min) - (facePoint - min)) / (max - min);

         // compute axis
         S32		bestAxis = 0;
         F32      best = 0.f;

         for (U32 i = 0; i < 6; i++)
         {
            F32 dot = mDot(info->normal, texGenAxis[i * 3]);
            if (dot > best)
            {
               best = dot;
               bestAxis = i;
            }
         }

         Point3F xv = texGenAxis[bestAxis * 3 + 1];
         Point3F yv = texGenAxis[bestAxis * 3 + 2];

         S32 sv, tv;

         if (xv.x)
            sv = 0;
         else if (xv.y)
            sv = 1;
         else
            sv = 2;

         if (yv.x)
            tv = 0;
         else if (yv.y)
            tv = 1;
         else
            tv = 2;

         // handle coord translation
         if (bestAxis == 2 || bestAxis == 3)
         {
            S32 x = sv;
            sv = tv;
            tv = x;

            if (yv.z < 0)
               s[sv] = 1.f - s[sv];
         }

         if (bestAxis < 2)
         {
            if (yv.y < 0)
               s[sv] = 1.f - s[sv];
         }

         if (bestAxis > 3)
         {
            s[sv] = 1.f - s[sv];
            if (yv.z > 0)
               s[tv] = 1.f - s[tv];

         }

         // done!
         info->texCoord.set(s[sv], s[tv]);

      }

      return true;
   }

   return false;
}
#pragma endregion Collision
//==================================================================================================
//
//==================================================================================================
CollisionDirector::CollisionDirector() : Director()
{
   //Establish the timing we'll need
   mTimingGroup = DirectorManager::TimingGroup::PreSim;

   //Here, we listen to the CollisionComponent's add and remove signaling.
   //If a CollisionComponent(or in other directors, any other components we care about) are added/removed
   //we can process the component and it's owner to track valid entries the director actually cares about
   CollisionComponent::getAddedComponentSignal().notify(this, &CollisionDirector::registerComponent);
   CollisionComponent::getRemovedComponentSignal().notify(this, &CollisionDirector::unregisterComponent);
}

CollisionDirector::~CollisionDirector()
{
   mValidEntriesList.clear();
}

void CollisionDirector::registerComponent(ComponentObject* owner, const Component& comp)
{
   //We have a valid component we care about added to a ComponentObject
   //So lets create a ref and add it to the list if it's valid
   //Because this is called whenever a component this director cares about is added
   //We can only worry about Objects that match to ALL requirements. Otherwise, we can
   //completely ignore it for this director's purposes
   CollisionEntityRef ref;
   ref.owner = owner;
   ref.controlObj = owner->getComponentInstance<CollisionComponentInstance>();
   //ref.transform = owner->getComponentInstance<Transform3DComponentInstance>();

   //If all valid, we finally add it
   if (ref.isValid())
      mValidEntriesList.push_back(ref);
}

void CollisionDirector::unregisterComponent(ComponentObject* owner, const Component& comp)
{
   //A component's been removed, so track down the entry and remove it from our valid list
   for (U32 i = 0; i < mValidEntriesList.size(); i++)
   {
      if (mValidEntriesList[i].owner == owner)
      {
         mValidEntriesList.erase(i);
         return;
      }
   }
}

void CollisionDirector::update()
{
   //Now we loop over all the valid entries we've got and go to work
   for (U32 i = 0; i < mValidEntriesList.size(); i++)
   {
      CollisionEntityRef& ref = mValidEntriesList[i];

      //All good, so we'll pass in the stuff the component needs to do it's work, and let it crunch.
      //In other directors, we may have structs to pack complex data for the components to work off of.
      //The reson we do this is to keep the work the components do compartmentalized.
      //This keeps it more cache friendly, and also threadsafe when we don't have to worry about the components
      //needing to reach out to any other objects while they work.
      //ref.controlObj->update(move);
      ref.controlObj->update();
   }
}
