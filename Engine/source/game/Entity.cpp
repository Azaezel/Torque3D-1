#include "Entity.h"

#include "math/mathIO.h"
#include "scene/sceneRenderState.h"
#include "core/stream/bitStream.h"
#include "materials/sceneData.h"
#include "gfx/gfxDebugEvent.h"
#include "gfx/gfxTransformSaver.h"
#include "renderInstance/renderPassManager.h"

// Client prediction
static F32 sMinWarpTicks = 0.5f;       // Fraction of tick at which instant warp occurs
static S32 sMaxWarpTicks = 3;          // Max warp duration in ticks
static S32 sMaxPredictionTicks = 30;   // Number of ticks to predict

IMPLEMENT_CO_NETOBJECT_V1(Entity);

ConsoleDocClass(Entity,
   "@brief \n" );

//-----------------------------------------------------------------------------
// Object setup and teardown
//-----------------------------------------------------------------------------
Entity::Entity()
{
   // Flag this object so that it will always
   // be sent across the network to clients
   mNetFlags.set( Ghostable | ScopeAlways );

   mTypeMask |= EntityObjectType;
}

Entity::~Entity()
{
}

//-----------------------------------------------------------------------------
// Object Editing
//-----------------------------------------------------------------------------
void Entity::initPersistFields()
{
   // SceneObject already handles exposing the transform
   Parent::initPersistFields();
}

bool Entity::onAdd()
{
   if ( !Parent::onAdd() )
      return false;

   mObjBox = Box3F(Point3F(-0.5, -0.5, -0.5), Point3F(0.5, 0.5, 0.5));

   resetWorldBox();
   setObjectBox(mObjBox);

   addToScene();

   addComponents();

   //Make sure we get positioned
   if (isServerObject())
   {
      setMaskBits(TransformMask);
      //setMaskBits(NamespaceMask);
   }
   else
   {
      //We can shortcut the initialization here because stuff generally ghosts down in order, and onPostAdd isn't called on ghosts.
      onPostAdd();
   }

   return true;
}

void Entity::onRemove()
{
   clearComponents(true);

   // Remove this object from the scene
   removeFromScene();

   onDataSet.removeAll();

   Parent::onRemove();
}

void Entity::addComponents()
{
   const char* bField = "";
   const char* sField = "";

   // Check for data fields which contain packed behaviors, and instantiate them
   // As a side note, this is the most obfuscated conditional block I think I've ever written   
   for (int i = 0; dStrcmp(bField = getDataField(StringTable->insert(avar("_component%d", i)), NULL), "") != 0; i++)
   {
      AssertFatal((StringUnit::getUnitCount(bField, "\t") - 1) % 2 == 0, "Fields should always be in sets of two!");

      // Grab the template name, make sure the sim knows about it or we are hosed anyway
      StringTableEntry templateName = StringTable->insert(StringUnit::getUnit(bField, 0, "\t"));
      Component* tpl = dynamic_cast<Component*>(Sim::findObject(templateName));
      if (tpl == NULL)
      {
         // If anyone wants to know, let them.
         /*if (isMethod("onBehaviorMissing"))
            Con::executef(this, "onBehaviorMissing", templateName);
         else
            Con::warnf("ComponentObject::addBehaviors - Missing Behavior %s", templateName);*/

         // Skip it, it's invalid.
         setDataField(StringTable->insert(avar("_component%d", i)), NULL, "");

         continue;
      }

      // create instance
      if (!addComponent(tpl))
         continue;

      ComponentInstance* inst = mComponents[mComponents.size() - 1];

      // Sub loop to set up the fields with the values that got written out
      S32 index = 1;
      while (index < StringUnit::getUnitCount(bField, "\t"))
      {
         StringTableEntry slotName = StringTable->insert(StringUnit::getUnit(bField, index++, "\t"));
         const char* slotValue = StringUnit::getUnit(bField, index++, "\t");

         //check if it's a regular behavior field, or one of our special instanced fields
         if (!tpl->getComponentField(slotName))
            inst->addComponentField(slotName, slotValue);
         else
            inst->setDataField(slotName, NULL, slotValue);
      }

      //check for sub fields to this
      for (int sfi = 1; dStrcmp(sField = getDataField(StringTable->insert(avar("_component%d_%d", i, sfi)), NULL), "") != 0; sfi++)
      {
         S32 sindex = 0;
         while (sindex < StringUnit::getUnitCount(sField, "\t"))
         {
            StringTableEntry slotName = StringTable->insert(StringUnit::getUnit(sField, sindex++, "\t"));
            const char* slotValue = StringUnit::getUnit(sField, sindex++, "\t");

            //check if it's a regular behavior field, or one of our special instanced fields
            if (!tpl->getComponentField(slotName))
               inst->addComponentField(slotName, slotValue);
            else
               inst->setDataField(slotName, NULL, slotValue);
         }

         setDataField(StringTable->insert(avar("_component%d_%d", i, sfi)), NULL, "");
      }

      //clear the dynamic fields of the behaviors so they're not cluttering the insepctor
      setDataField(StringTable->insert(avar("_component%d", i)), NULL, "");
   }

   //Callback for letting scripts know we're done loading our behaviors
   //if (isServerObject())
   //   Con::executef(this, "onBehaviorsLoaded");

   //Now alert the behaviors they've been added for their callback
   /*for (U32 i = 0; i < mComponents.size(); i++)
   {
      if (isServerObject()) {
         if (mComponents[i]->isMethod("onAdd"))
            Con::executef(mComponents[i], "onAdd");
      }
   }*/
}

//
//
void Entity::onPostAdd()
{
   //everything's done and added. go ahead and initialize the components
   /*for (U32 i = 0; i < mComponents.size(); i++)
   {
      mComponents[i].onComponentAdd();
   }*/

   //Set up the networked components
   mNetworkedComponents.clear();
   for (U32 i = 0; i < mComponents.size(); i++)
   {
      if (mComponents[i]->getComponentData().isNetworked())
      {
         NetworkedComponent netComp;
         netComp.componentIndex = i;
         netComp.updateState = NetworkedComponent::Adding;
         netComp.updateMaskBits = -1;

         mNetworkedComponents.push_back(netComp);
      }
   }

   if (!mNetworkedComponents.empty())
   {
      setMaskBits(AddComponentsMask);
      setMaskBits(ComponentsUpdateMask);
   }

   if (isMethod("onAdd"))
      Con::executef(this, "onAdd");
}

void Entity::setDataField(StringTableEntry slotName, const char* array, const char* value)
{
   Parent::setDataField(slotName, array, value);

   onDataSet.trigger(this, slotName, value);
}

void Entity::onStaticModified(const char* slotName, const char* newValue)
{
   Parent::onStaticModified(slotName, newValue);

   onDataSet.trigger(this, slotName, newValue);
}

//
//
//
/*void Entity::setTransform(const MatrixF& mat)
{
   // Let SceneObject handle all of the matrix manipulation
   Parent::setTransform( mat );

   // Dirty our network mask so that the new transform gets
   // transmitted to the client object
   setMaskBits( TransformMask );
}*/

bool Entity::_setPosition(void* object, const char* index, const char* data)
{
   Entity* so = static_cast<Entity*>(object);
   if (so)
   {
      Point3F pos;

      if (!dStrcmp(data, ""))
         pos = Point3F(0, 0, 0);
      else
         Con::setData(TypePoint3F, &pos, 0, 1, &data);

      so->setTransform(pos, so->mRot);
   }
   return false;
}

const char* Entity::_getPosition(void* obj, const char* data)
{
   Entity* so = static_cast<Entity*>(obj);
   if (so)
   {
      Point3F pos = so->getPosition();

      static const U32 bufSize = 256;
      char* returnBuffer = Con::getReturnBuffer(bufSize);
      dSprintf(returnBuffer, bufSize, "%g %g %g", pos.x, pos.y, pos.z);
      return returnBuffer;
   }
   return "0 0 0";
}

bool Entity::_setRotation(void* object, const char* index, const char* data)
{
   Entity* so = static_cast<Entity*>(object);
   if (so)
   {
      RotationF rot;
      Con::setData(TypeRotationF, &rot, 0, 1, &data);

      //so->mRot = rot;
      //MatrixF mat = rot.asMatrixF();
      //mat.setPosition(so->getPosition());
      //so->setTransform(mat);
      so->setTransform(so->getPosition(), rot);
   }
   return false;
}

const char* Entity::_getRotation(void* obj, const char* data)
{
   Entity* so = static_cast<Entity*>(obj);
   if (so)
   {
      EulerF eulRot = so->mRot.asEulerF();

      static const U32 bufSize = 256;
      char* returnBuffer = Con::getReturnBuffer(bufSize);
      dSprintf(returnBuffer, bufSize, "%g %g %g", mRadToDeg(eulRot.x), mRadToDeg(eulRot.y), mRadToDeg(eulRot.z));
      return returnBuffer;
   }
   return "0 0 0";
}

void Entity::setTransform(const MatrixF& mat)
{
   MatrixF oldTransform = getTransform();

   if (isMounted())
   {
      // Use transform from mounted object
      Point3F newPos = mat.getPosition();
      Point3F parentPos = mMount.object->getTransform().getPosition();

      Point3F newOffset = newPos - parentPos;

      if (!newOffset.isZero())
      {
         mPos = newOffset;
      }

      Point3F matEul = mat.toEuler();

      if (matEul != Point3F(0, 0, 0))
      {
         Point3F mountEul = mMount.object->getTransform().toEuler();
         Point3F diff = matEul - mountEul;

         mRot = diff;
      }
      else
      {
         mRot = Point3F(0, 0, 0);
      }

      RotationF addRot = mRot + RotationF(mMount.object->getTransform());
      MatrixF transf = addRot.asMatrixF();
      transf.setPosition(mPos + mMount.object->getPosition());

      Parent::setTransform(transf);

      if (transf != oldTransform)
         setMaskBits(TransformMask);
   }
   else
   {
      //Are we part of a prefab?
      /*Prefab* p = Prefab::getPrefabByChild(this);
      if (p)
      {
         //just let our prefab know we moved
         p->childTransformUpdated(this, mat);
      }*/
      //else
      {
         //mRot.set(mat);
         //Parent::setTransform(mat);

         RotationF rot = RotationF(mat);

         EulerF tempRot = rot.asEulerF(RotationF::Degrees);

         Point3F pos;

         mat.getColumn(3, &pos);

         setTransform(pos, rot);
      }
   }
}

void Entity::setTransform(const Point3F& position, const RotationF& rotation)
{
   MatrixF oldTransform = getTransform();

   if (isMounted())
   {
      mPos = position;
      mRot = rotation;

      RotationF addRot = mRot + RotationF(mMount.object->getTransform());
      MatrixF transf = addRot.asMatrixF();
      transf.setPosition(mPos + mMount.object->getPosition());

      Parent::setTransform(transf);

      if (transf != oldTransform)
         setMaskBits(TransformMask);
   }
   else
   {
      /*MatrixF newMat, imat, xmat, ymat, zmat;
      Point3F radRot = Point3F(mDegToRad(rotation.x), mDegToRad(rotation.y), mDegToRad(rotation.z));
      xmat.set(EulerF(radRot.x, 0, 0));
      ymat.set(EulerF(0.0f, radRot.y, 0.0f));
      zmat.set(EulerF(0, 0, radRot.z));
      imat.mul(zmat, xmat);
      newMat.mul(imat, ymat);*/

      MatrixF newMat = rotation.asMatrixF();

      newMat.setColumn(3, position);

      mPos = position;
      mRot = rotation;

      //if (isServerObject())
      //   setMaskBits(TransformMask);

      //setTransform(temp);

      // This test is a bit expensive so turn it off in release.   
#ifdef TORQUE_DEBUG
      //AssertFatal( mat.isAffine(), "SceneObject::setTransform() - Bad transform (non affine)!" );
#endif

      //PROFILE_SCOPE(Entity_setTransform);

      // Update the transforms.
      Parent::setTransform(newMat);

      /*U32 compCount = mComponents.size();
      for (U32 i = 0; i < compCount; ++i)
      {
         mComponents[i]->ownerTransformSet(&newMat);
      }*/

      Point3F newPos = newMat.getPosition();
      RotationF newRot = newMat;

      Point3F oldPos = oldTransform.getPosition();
      RotationF oldRot = oldTransform;

      if (newPos != oldPos || newRot != oldRot)
         setMaskBits(TransformMask);
   }
}

void Entity::setRenderTransform(const MatrixF& mat)
{
   Parent::setRenderTransform(mat);
}

void Entity::setRenderTransform(const Point3F& position, const RotationF& rotation)
{
   if (isMounted())
   {
      mPos = position;
      mRot = rotation;

      RotationF addRot = mRot + RotationF(mMount.object->getTransform());
      MatrixF transf = addRot.asMatrixF();
      transf.setPosition(mPos + mMount.object->getPosition());

      Parent::setRenderTransform(transf);
   }
   else
   {
      MatrixF newMat = rotation.asMatrixF();

      newMat.setColumn(3, position);

      mPos = position;
      mRot = rotation;

      Parent::setRenderTransform(newMat);

      /*U32 compCount = mComponents.size();
      for (U32 i = 0; i < compCount; ++i)
      {
         mComponents[i]->ownerTransformSet(&newMat);
      }*/
   }
}

MatrixF Entity::getTransform()
{
   if (isMounted())
   {
      MatrixF mat;

      //Use transform from mount
      mMount.object->getMountTransform(mMount.node, mMount.xfm, &mat);

      Point3F transPos = mat.getPosition() + mPos;

      mat.mul(mRot.asMatrixF());

      mat.setPosition(transPos);

      return mat;
   }
   else
   {
      return Parent::getTransform();
   }
}

void Entity::setMountOffset(const Point3F& posOffset)
{
   if (isMounted())
   {
      mMount.xfm.setColumn(3, posOffset);
      //mPos = posOffset;
      setMaskBits(MountedMask);
   }
}

void Entity::setMountRotation(const EulerF& rotOffset)
{
   if (isMounted())
   {
      MatrixF temp, imat, xmat, ymat, zmat;

      Point3F radRot = Point3F(mDegToRad(rotOffset.x), mDegToRad(rotOffset.y), mDegToRad(rotOffset.z));
      xmat.set(EulerF(radRot.x, 0, 0));
      ymat.set(EulerF(0.0f, radRot.y, 0.0f));
      zmat.set(EulerF(0, 0, radRot.z));

      imat.mul(zmat, xmat);
      temp.mul(imat, ymat);

      temp.setColumn(3, mMount.xfm.getPosition());

      mMount.xfm = temp;

      setMaskBits(MountedMask);
   }
}
//
void Entity::getCameraTransform(F32* pos, MatrixF* mat)
{
   /*Component* foundComp = getComponent(sCameraComponentType);

   if (foundComp != nullptr)
   {
      CameraComponent* cameraComp = static_cast<CameraComponent*>(foundComp);
      cameraComp->getCameraTransform(pos, mat);
   }*/
}

void Entity::getMountTransform(S32 index, const MatrixF& xfm, MatrixF* outMat)
{
   /*renderComponent* renderComp = getComponent<renderComponent>(sRenderComponentType);

   if (renderComp)
   {
      renderComp->getShapeInstance()->animate();
      S32 nodeCount = renderComp->getShapeInstance()->getShape()->nodes.size();

      if (index >= 0 && index < nodeCount)
      {
         MatrixF mountTransform = renderComp->getShapeInstance()->mNodeTransforms[index];
         mountTransform.mul(xfm);
         const Point3F& scale = getScale();

         // The position of the mount point needs to be scaled.
         Point3F position = mountTransform.getPosition();
         position.convolve(scale);
         mountTransform.setPosition(position);

         // Also we would like the object to be scaled to the model.
         outMat->mul(mObjToWorld, mountTransform);
         return;
      }
   }*/

   // Then let SceneObject handle it.
   Parent::getMountTransform(index, xfm, outMat);
}

void Entity::getRenderMountTransform(F32 delta, S32 index, const MatrixF& xfm, MatrixF* outMat)
{
   /*renderComponent* renderComp = getComponent<renderComponent>(sRenderComponentType);

   if (renderComp && renderComp->getShapeInstance())
   {
      renderComp->getShapeInstance()->animate();
      S32 nodeCount = renderComp->getShape()->nodes.size();

      if (index >= 0 && index < nodeCount)
      {
         MatrixF mountTransform = renderComp->getShapeInstance()->mNodeTransforms[index];
         mountTransform.mul(xfm);
         const Point3F& scale = getScale();

         // The position of the mount point needs to be scaled.
         Point3F position = mountTransform.getPosition();
         position.convolve(scale);
         mountTransform.setPosition(position);

         // Also we would like the object to be scaled to the model.
         outMat->mul(getRenderTransform(), mountTransform);
         return;
      }
   }*/

   // Then let SceneObject handle it.
   Parent::getMountTransform(index, xfm, outMat);
}
//
// Updating
//
void Entity::processTick(const Move* move)
{
   //This would presumably be behaviors execution?
   /*for (U32 i = 0; i < mComponents.size(); i++)
   {
      mComponents[i]->processTick();
   }*/

   if (!isHidden())
   {
      if (mDelta.warpCount < mDelta.warpTicks)
      {
         mDelta.warpCount++;

         // Set new pos.
         mObjToWorld.getColumn(3, &mDelta.pos);
         mDelta.pos += mDelta.warpOffset;
         mDelta.rot[0] = mDelta.rot[1];
         mDelta.rot[1].interpolate(mDelta.warpRot[0], mDelta.warpRot[1], F32(mDelta.warpCount) / mDelta.warpTicks);
         setTransform(mDelta.pos, mDelta.rot[1]);

         // Pos backstepping
         mDelta.posVec.x = -mDelta.warpOffset.x;
         mDelta.posVec.y = -mDelta.warpOffset.y;
         mDelta.posVec.z = -mDelta.warpOffset.z;
      }
      else
      {
         if (isMounted())
         {
            MatrixF mat;
            mMount.object->getMountTransform(mMount.node, mMount.xfm, &mat);
            Parent::setTransform(mat);
            Parent::setRenderTransform(mat);
         }
         else
         {
            if (!move)
            {
               if (isGhost())
               {
                  // If we haven't run out of prediction time,
                  // predict using the last known move.
                  if (mPredictionCount-- <= 0)
                     return;

                  move = &mDelta.move;
               }
               else
               {
                  move = &NullMove;
               }
            }
         }
      }

      Move prevMove = mLastMove;

      if (move != NULL)
         mLastMove = *move;
      else
         mLastMove = NullMove;

      if (move && isServerObject())
      {
         if ((move->y != 0 || prevMove.y != 0)
            || (move->x != 0 || prevMove.x != 0)
            || (move->z != 0 || prevMove.x != 0))
         {
            if (isMethod("moveVectorEvent"))
               Con::executef(this, "moveVectorEvent", move->x, move->y, move->z);
         }

         if (move->yaw != 0)
         {
            if (isMethod("moveYawEvent"))
               Con::executef(this, "moveYawEvent", move->yaw);
         }

         if (move->pitch != 0)
         {
            if (isMethod("movePitchEvent"))
               Con::executef(this, "movePitchEvent", move->pitch);
         }

         if (move->roll != 0)
         {
            if (isMethod("moveRollEvent"))
               Con::executef(this, "moveRollEvent", move->roll);
         }

         for (U32 i = 0; i < MaxTriggerKeys; i++)
         {
            if (move->trigger[i] != prevMove.trigger[i])
            {
               if (isMethod("moveTriggerEvent"))
                  Con::executef(this, "moveTriggerEvent", i, move->trigger[i]);
            }
         }
      }

      // Save current rigid state interpolation
      mDelta.posVec = getPosition();
      mDelta.rot[0] = mRot.asQuatF();

      //Handle any script updates, which can include physics stuff
      if (isServerObject() && isMethod("processTick"))
         Con::executef(this, "processTick");

      // Wrap up interpolation info
      mDelta.pos = getPosition();
      mDelta.posVec -= getPosition();
      mDelta.rot[1] = mRot.asQuatF();

      setTransform(getPosition(), mRot);

      //Lifetime test
      /*if (mLifetimeMS != 0)
      {
         S32 currentTime = Platform::getRealMilliseconds();
         if (currentTime - mStartTimeMS >= mLifetimeMS)
            deleteObject();
      }*/
   }
}

void Entity::advanceTime(F32 dt)
{
}

void Entity::interpolateTick(F32 dt)
{
   if (dt == 0.0f)
   {
      setRenderTransform(mDelta.pos, mDelta.rot[1]);
   }
   else
   {
      QuatF rot;
      rot.interpolate(mDelta.rot[1], mDelta.rot[0], dt);
      Point3F pos = mDelta.pos + mDelta.posVec * dt;

      setRenderTransform(pos, rot);
   }

   mDelta.dt = dt;
}

//
// Networking
//
U32 Entity::packUpdate( NetConnection *conn, U32 mask, BitStream *stream )
{
   U32 retMask = Parent::packUpdate(conn, mask, stream);

   if (stream->writeFlag(mask & TransformMask))
   {
      stream->writeCompressedPoint(mPos);
      mathWrite(*stream, getRotation());

      mDelta.move.pack(stream);

      stream->writeFlag(!(mask & NoWarpMask));
   }

   if (stream->writeFlag(mask & BoundsMask))
   {
      mathWrite(*stream, mObjBox);
   }

   if (stream->writeFlag(mask & AddComponentsMask))
   {
      U32 toAddComponentCount = 0;

      for (U32 i = 0; i < mNetworkedComponents.size(); i++)
      {
         if (mNetworkedComponents[i].updateState == NetworkedComponent::Adding)
         {
            toAddComponentCount++;
         }
      }

      //you reaaaaally shouldn't have >255 networked components on a single entity
      stream->writeInt(toAddComponentCount, 8);

      for (U32 i = 0; i < mNetworkedComponents.size(); i++)
      {
         if (mNetworkedComponents[i].updateState == NetworkedComponent::Adding)
         {
            Component* comp = mComponents[mNetworkedComponents[i].componentIndex]->getComponentDataPtr();
            stream->writeRangedU32(comp->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

            mNetworkedComponents[i].updateState = NetworkedComponent::Updating;
         }
      }
   }

   if (stream->writeFlag(mask & RemoveComponentsMask))
   {
      /*U32 toRemoveComponentCount = 0;

      for (U32 i = 0; i < mNetworkedComponents.size(); i++)
      {
         if (mNetworkedComponents[i].updateState == NetworkedComponent::Adding)
         {
            toRemoveComponentCount++;
         }
      }

      //you reaaaaally shouldn't have >255 networked components on a single entity
      stream->writeInt(toRemoveComponentCount, 8);

      for (U32 i = 0; i < mNetworkedComponents.size(); i++)
      {
         if (mNetworkedComponents[i].updateState == NetworkedComponent::Removing)
         {
            stream->writeInt(i, 16);
         }
      }*/

      /*for (U32 i = 0; i < mNetworkedComponents.size(); i++)
      {
         if (mNetworkedComponents[i].updateState == NetworkedComponent::UpdateState::Removing)
         {
            removeComponent(mComponents[mNetworkedComponents[i].componentIndex], true);
            mNetworkedComponents.erase(i);
            i--;

         }
      }*/
   }

   //Update our components
   if (stream->writeFlag(mask & ComponentsUpdateMask))
   {
      U32 toUpdateComponentCount = 0;

      for (U32 i = 0; i < mNetworkedComponents.size(); i++)
      {
         if (mNetworkedComponents[i].updateState == NetworkedComponent::Updating)
         {
            toUpdateComponentCount++;
         }
      }

      //you reaaaaally shouldn't have >255 networked components on a single entity
      stream->writeInt(toUpdateComponentCount, 8);

      bool forceUpdate = false;

      for (U32 i = 0; i < mNetworkedComponents.size(); i++)
      {
         if (mNetworkedComponents[i].updateState == NetworkedComponent::Updating)
         {
            stream->writeInt(i, 8);

            mNetworkedComponents[i].updateMaskBits = mComponents[mNetworkedComponents[i].componentIndex]->packUpdate(conn, mNetworkedComponents[i].updateMaskBits, stream);

            if (mNetworkedComponents[i].updateMaskBits != 0)
               forceUpdate = true;
            else
               mNetworkedComponents[i].updateState = NetworkedComponent::None;
         }
      }

      //If we have leftover, we need to re-iterate our packing
      if (forceUpdate)
         setMaskBits(ComponentsUpdateMask);
   }

   /*if (stream->writeFlag(mask & NamespaceMask))
   {
      const char* name = getName();
      if (stream->writeFlag(name && name[0]))
         stream->writeString(String(name));

      if (stream->writeFlag(mSuperClassName && mSuperClassName[0]))
         stream->writeString(String(mSuperClassName));

      if (stream->writeFlag(mClassName && mClassName[0]))
         stream->writeString(String(mClassName));
   }*/

   return retMask;
}

void Entity::unpackUpdate(NetConnection *conn, BitStream *stream)
{
   Parent::unpackUpdate(conn, stream);

   if (stream->readFlag())
   {
      Point3F pos;
      stream->readCompressedPoint(&pos);

      RotationF rot;
      mathRead(*stream, &rot);

      mDelta.move.unpack(stream);

      if (stream->readFlag() && isProperlyAdded())
      {
         // Determine number of ticks to warp based on the average
         // of the client and server velocities.
         Point3F cp = mDelta.pos + mDelta.posVec * mDelta.dt;
         mDelta.warpOffset = pos - cp;

         // Calc the distance covered in one tick as the average of
         // the old speed and the new speed from the server.
         VectorF vel = pos - mDelta.pos;
         F32 dt, as = vel.len() * 0.5 * TickSec;

         // Cal how many ticks it will take to cover the warp offset.
         // If it's less than what's left in the current tick, we'll just
         // warp in the remaining time.
         if (!as || (dt = mDelta.warpOffset.len() / as) > sMaxWarpTicks)
            dt = mDelta.dt + sMaxWarpTicks;
         else
            dt = (dt <= mDelta.dt) ? mDelta.dt : mCeil(dt - mDelta.dt) + mDelta.dt;

         // Adjust current frame interpolation
         if (mDelta.dt)
         {
            mDelta.pos = cp + (mDelta.warpOffset * (mDelta.dt / dt));
            mDelta.posVec = (cp - mDelta.pos) / mDelta.dt;
            QuatF cr;
            cr.interpolate(mDelta.rot[1], mDelta.rot[0], mDelta.dt);

            mDelta.rot[1].interpolate(cr, rot.asQuatF(), mDelta.dt / dt);
            mDelta.rot[0].extrapolate(mDelta.rot[1], cr, mDelta.dt);
         }

         // Calculated multi-tick warp
         mDelta.warpCount = 0;
         mDelta.warpTicks = (S32)(mFloor(dt));
         if (mDelta.warpTicks)
         {
            mDelta.warpOffset = pos - mDelta.pos;
            mDelta.warpOffset /= mDelta.warpTicks;
            mDelta.warpRot[0] = mDelta.rot[1];
            mDelta.warpRot[1] = rot.asQuatF();
         }
      }
      else
      {
         // Set the entity to the server position
         mDelta.dt = 0;
         mDelta.pos = pos;
         mDelta.posVec.set(0, 0, 0);
         mDelta.rot[1] = mDelta.rot[0] = rot.asQuatF();
         mDelta.warpCount = mDelta.warpTicks = 0;
         setTransform(pos, rot);
      }
   }

   if (stream->readFlag())
   {
      mathRead(*stream, &mObjBox);
      resetWorldBox();
   }

   //AddComponentMask
   if (stream->readFlag())
   {
      U32 addedComponentCount = stream->readInt(8);

      for (U32 i = 0; i < addedComponentCount; i++)
      {
         S32 compId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);

         Component* comp;
         if (Sim::findObject(compId, comp))
         {
            addComponent(comp);
         }
      }
   }

   //RemoveComponentMask
   if (stream->readFlag())
   {

   }

   //ComponentUpdateMask
   if (stream->readFlag())
   {
      U32 updatingComponents = stream->readInt(8);

      for (U32 i = 0; i < updatingComponents; i++)
      {
         U32 updateComponentIndex = stream->readInt(8);

         mComponents[updateComponentIndex]->unpackUpdate(conn, stream);
      }
   }

   /*if (stream->readFlag())
   {
      if (stream->readFlag())
      {
         char name[256];
         stream->readString(name);
         assignName(name);
      }

      if (stream->readFlag())
      {
         char superClassname[256];
         stream->readString(superClassname);
         mSuperClassName = superClassname;
      }

      if (stream->readFlag())
      {
         char classname[256];
         stream->readString(classname);
         mClassName = classname;
      }

      linkNamespaces();
   }*/
}

//
// Mounting and heirarchy manipulation
//
void Entity::mountObject(SceneObject* objB, const MatrixF& txfm)
{
   Parent::mountObject(objB, -1, txfm);
   Parent::addObject(objB);
}

void Entity::mountObject(SceneObject* obj, S32 node, const MatrixF& xfm)
{
   Parent::mountObject(obj, node, xfm);
}

void Entity::onMount(SceneObject* obj, S32 node)
{
   deleteNotify(obj);

   // Are we mounting to a GameBase object?
   Entity* entityObj = dynamic_cast<Entity*>(obj);

   //if (entityObj && entityObj->getControlObject() != this)
   //   processAfter(entityObj);

   if (!isGhost()) {
      setMaskBits(MountedMask);

      //TODO implement this callback
      //onMount_callback( this, obj, node );
   }
}

void Entity::onUnmount(SceneObject* obj, S32 node)
{
   clearNotify(obj);

   Entity* entityObj = dynamic_cast<Entity*>(obj);

   //if (entityObj && entityObj->getControlObject() != this)
   //   clearProcessAfter();

   if (!isGhost()) {
      setMaskBits(MountedMask);

      //TODO implement this callback
      //onUnmount_callback( this, obj, node );
   }
}
void Entity::addObject(SimObject* object)
{
   Component* component = dynamic_cast<Component*>(object);
   if (component)
   {
      addComponent(component);
      return;
   }

   Entity* e = dynamic_cast<Entity*>(object);
   if (e)
   {
      MatrixF offset;

      //offset.mul(getWorldTransform(), e->getWorldTransform());

      //check if we're mounting to a node on a shape we have
      /*String node = e->getDataField("mountNode", NULL);
      if (!node.isEmpty())
      {
         renderComponent* renderComp = getComponent<renderComponent>(sRenderComponentType);
         if (renderComp)
         {
            TSShape* shape = renderComp->getShape();
            S32 nodeIdx = shape->findNode(node);

            mountObject(e, nodeIdx, MatrixF::Identity);
         }
         else
         {
            mountObject(e, MatrixF::Identity);
         }
      }
      else
      {*/
         /*Point3F posOffset = mPos - e->getPosition();
         mPos = posOffset;

         RotationF rotOffset = mRot - e->getRotation();
         mRot = rotOffset;
         setMaskBits(TransformMask);
         mountObject(e, MatrixF::Identity);*/

         mountObject(e, MatrixF::Identity);
      //}

      //e->setMountOffset(e->getPosition() - getPosition());

      //Point3F diff = getWorldTransform().toEuler() - e->getWorldTransform().toEuler();

      //e->setMountRotation(Point3F(mRadToDeg(diff.x),mRadToDeg(diff.y),mRadToDeg(diff.z)));

      //mountObject(e, offset);
   }
   else
   {
      SceneObject* so = dynamic_cast<SceneObject*>(object);
      if (so)
      {
         //get the difference and build it as our offset!
         Point3F posOffset = so->getPosition() - mPos;
         RotationF rotOffset = RotationF(so->getTransform()) - mRot;

         MatrixF offset = rotOffset.asMatrixF();
         offset.setPosition(posOffset);

         mountObject(so, offset);
         return;
      }
   }

   Parent::addObject(object);
}

void Entity::removeObject(SimObject* object)
{
   Entity* e = dynamic_cast<Entity*>(object);
   if (e)
   {
      mPos = mPos + e->getPosition();
      mRot = mRot + e->getRotation();
      unmountObject(e);
      setMaskBits(TransformMask);
   }
   else
   {
      SceneObject* so = dynamic_cast<SceneObject*>(object);
      if (so)
         unmountObject(so);
   }

   Parent::removeObject(object);
}

bool Entity::addComponent(Component* comp)
{
   if (comp == NULL)
      return false;

   ComponentInstance* compInst = comp->createInstance(this);
   compInst->setIsServerObject(isServerObject());

   mComponents.push_back(compInst);

   if (comp->isNetworked())
   {
      NetworkedComponent netComp;
      netComp.componentIndex = mComponents.size() - 1;
      netComp.updateState = NetworkedComponent::Adding;
      netComp.updateMaskBits = -1;

      mNetworkedComponents.push_back(netComp);

      setMaskBits(AddComponentsMask);
      setMaskBits(ComponentsUpdateMask);
   }

   comp->addComponent(this); //trips the notify system

   return true;
}

bool Entity::removeComponent(Component* comp)
{
   if (comp == NULL)
      return false;

   ComponentInstance* compInst = getComponentInstanceByData(comp);

   if (compInst == nullptr)
      return false;

   if (mComponents.remove(compInst))
   {
      //AssertFatal(comp->isProperlyAdded(), "Don't know how but a component is not registered w/ the sim");

      //setComponentsDirty();

      comp->removeComponent(this);

      compInst->destroyInstance();

      return true;
   }

   return false;
}

SimObject* Entity::findObjectByInternalName(StringTableEntry internalName, bool searchChildren)
{
   for (U32 i = 0; i < mComponents.size(); i++)
   {
      if (mComponents[i]->getComponentData().getInternalName() == internalName)
      {
         return mComponents[i]->getComponentDataPtr();
      }
   }

   return Parent::findObjectByInternalName(internalName, searchChildren);
}

//
// IO
//
static void writeTabs(Stream& stream, U32 count)
{
   char tab[] = "   ";
   while (count--)
      stream.write(3, (void*)tab);
}

void Entity::write(Stream& stream, U32 tabStop, U32 flags)
{
   writeTabs(stream, tabStop);
   char buffer[1024];
   dSprintf(buffer, sizeof(buffer), "new %s(%s) {\r\n", getClassName(), getName() ? getName() : "");
   stream.write(dStrlen(buffer), buffer);
   writeFields(stream, tabStop + 1);

   //stream.write(1, "\n");
   ////first, write out our behavior objects

   // NOW we write the behavior fields proper
   if (mComponents.size() > 0)
   {
      // Pack out the behaviors into fields
      U32 i = 0;
      for (U32 i=0; i < mComponents.size(); i++)
      {
         ComponentInstance* bi = mComponents[i];

         writeTabs(stream, tabStop + 1);

         StringTableEntry compFieldData = bi->writeComponentFields();

         char buffer[1024];
         if(compFieldData != StringTable->EmptyString())
            dSprintf(buffer, sizeof(buffer), "_component%d = \"%s\t%s", i, bi->getComponentData().getName(), compFieldData);
         else
            dSprintf(buffer, sizeof(buffer), "_component%d = \"%s", i, bi->getComponentData().getName());

         stream.write(dStrlen(buffer), buffer);

         stream.write(4, "\";\r\n");
      }
   }

   //
   stream.write(2, "\r\n");
   for (U32 i = 0; i < size(); i++)
   {
      SimObject* child = (*this)[i];
      if (child->getCanSave())
         child->write(stream, tabStop + 1, flags);
   }

   writeTabs(stream, tabStop);
   stream.write(4, "};\r\n");
}

SimObject* Entity::getTamlChild(const U32 childIndex) const
{
   // Sanity!
   AssertFatal(childIndex < getTamlChildCount(), "SimSet::getTamlChild() - Child index is out of range.");

   // For when the assert is not used.
   if (childIndex >= getTamlChildCount())
      return NULL;

   //we always order components first, child objects second
   if (childIndex >= getComponentCount())
      return at(childIndex - getComponentCount());
   else
      return getComponent(childIndex);
}
//
void Entity::onCameraScopeQuery(NetConnection* connection, CameraScopeQuery* query)
{
   // Object itself is in scope.
   Parent::onCameraScopeQuery(connection, query);

   /*CameraComponent* cameraComp = getComponent<CameraComponent>(sCameraComponentType);
   if (cameraComp != nullptr)
   {
      cameraComp->onCameraScopeQuery(connection, query);
   }*/
}
//
void Entity::setObjectBox(const Box3F& objBox)
{
   mObjBox = objBox;
   resetWorldBox();

   if (isServerObject())
      setMaskBits(BoundsMask);
}

/*void Entity::updateContainer()
{
   PROFILE_SCOPE(Entity_updateContainer);

   // Update container drag and buoyancy properties
   containerInfo.box = getWorldBox();
   //containerInfo.mass = mMass;

   getContainer()->findObjects(containerInfo.box, WaterObjectType | PhysicalZoneObjectType, findRouter, &containerInfo);

   //mWaterCoverage = info.waterCoverage;
   //mLiquidType    = info.liquidType;
   //mLiquidHeight  = info.waterHeight;   
   //setCurrentWaterObject( info.waterObject );

   // This value might be useful as a datablock value,
   // This is what allows the player to stand in shallow water (below this coverage)
   // without jiggling from buoyancy
   /*if (info.waterCoverage >= 0.25f)
   {
      // water viscosity is used as drag for in water.
      // ShapeBaseData drag is used for drag outside of water.
      // Combine these two components to calculate this ShapeBase object's
      // current drag.
      mDrag = (info.waterCoverage * info.waterViscosity) +
         (1.0f - info.waterCoverage) * mDrag;
      //mBuoyancy = (info.waterDensity / mDataBlock->density) * info.waterCoverage;
   }

   //mAppliedForce = info.appliedForce;
   mGravityMod = info.gravityScale;*/
//}

//
//
//
#ifdef TORQUE_TOOLS
void Entity::onInspect(GuiInspector* inspector)
{
   /*S32 groupIdx = inspector->findExistentGroupIndex(StringTable->insert("GameObject"));
   GuiInspectorGroup* editingGroup = inspector->findExistentGroup(StringTable->insert("Editing"));
   //GuiControl* stack = dynamic_cast<GuiControl*>(materialGroup->findObjectByInternalName(StringTable->insert("Stack")));

   GuiInspectorEntityGroup* components = new GuiInspectorEntityGroup("Components", inspector);
   if (components != NULL)
   {
      components->registerObject();

      inspector->insertInspectorGroup(groupIdx, components);
      inspector->addObject(components);
      inspector->reOrder(components, editingGroup);
   }

   groupIdx++;

   U32 compCount = getComponentCount();
   //Now, add the component groups
   for (U32 c = 0; c < compCount; ++c)
   {
      Component* comp = getComponent(c);

      String compName;
      if (comp->getFriendlyName() != StringTable->EmptyString())
         compName = comp->getFriendlyName();
      else
         compName = comp->getComponentName();

      StringBuilder captionString;
      captionString.format("%s [%i]", compName.c_str(), comp->getId());

      GuiInspectorGroup* compGroup = new GuiInspectorComponentGroup(captionString.data(), inspector, comp);
      if (compGroup != NULL)
      {
         compGroup->registerObject();
         inspector->insertInspectorGroup(groupIdx, compGroup);
         inspector->addObject(compGroup);
         inspector->reOrder(compGroup, editingGroup);

         groupIdx++;
      }
   }

   for (U32 i = 0; i < mComponents.size(); i++)
   {
      Component* comp = mComponents[i];
      comp->onInspect();
   }*/
}

void Entity::onEndInspect()
{
   /*for (U32 i = 0; i < mComponents.size(); i++)
   {
      Component* comp = mComponents[i];
      comp->onEndInspect();
   }

   GuiTreeViewCtrl* editorTree = dynamic_cast<GuiTreeViewCtrl*>(Sim::findObject("EditorTree"));
   if (!editorTree)
      return;

   S32 componentItemIdx = editorTree->findItemByName("Components");

   editorTree->removeItem(componentItemIdx, false);*/
}
#endif

DefineEngineMethod(Entity, addComponent, bool, (Component* toAddComponent), (nullAsType<Component*>()),
   "@brief Add a component to the entity\n\n")
{
   return object->addComponent(toAddComponent);
}

DefineEngineMethod(Entity, removeComponent, bool, (Component* toRemoveComponent), (nullAsType<Component*>()),
   "@brief Remove a component from the entity\n")
{
   return object->removeComponent(toRemoveComponent);
}

