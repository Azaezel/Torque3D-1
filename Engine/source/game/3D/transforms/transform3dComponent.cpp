#include "transform3DComponent.h"
#include "transform3DComponent_scriptBinding.h"
#include "gfx/gfxDrawUtil.h"
#include "game/Entity.h"
#include <gfx/gfxTransformSaver.h>
#include "scene/sceneRenderState.h"
#include "renderInstance/renderPassManager.h"
#include "materials/baseMatInstance.h"
#include <math/mathIO.h>

IMPLEMENT_CO_DATABLOCK_V1(Transform3DComponent);

IMPL_COMP_REGISTER_SIGNALS(Transform3DComponent);

Transform3DComponent::Transform3DComponent() : Component()
{
   mNetworked = true;
}

bool Transform3DComponent::onAdd()
{
   if (!Parent::onAdd())
      return false;

   addComponentField("position", "Object world position.", "TypeMatrixPosition", "0 0 0");
   addComponentField("rotation", "Object world orientation.", "TypeMatrixRotation", "0 0 0 1");
   addComponentField("scale", "Object world scale.", "TypePoint3F", "1 1 1");

   return true;
}

void Transform3DComponent::consoleInit()
{
   Parent::consoleInit();

   //We'll register the Transform3DComponentDirector to the DirectorManager, so it's ready to go at runtime
   //DirectorManager::get()->mDirectors.push_back(new Transform3DComponentDirector());
}

void Transform3DComponent::initPersistFields()
{
   Parent::initPersistFields();
}

void Transform3DComponent::packData(BitStream* stream)
{
   Parent::packData(stream);
}

void Transform3DComponent::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);
}

ComponentInstance* Transform3DComponent::createInstance(ComponentObject* owner)
{
   Transform3DComponentInstance* compInst = new Transform3DComponentInstance(*this, *owner);

   if (!compInst->registerObject())
   {
      Con::errorf("Transform3DComponent::createInstance() - failed to create instance");
      return nullptr;
   }

   setupFields(compInst, true);
   Transform3DComponentInstance::sComponentInstanceList.push_back(compInst);

   return compInst;
}

//==================================================================================================
//
//==================================================================================================
IMPLEMENT_CONOBJECT(Transform3DComponentInstance);

Vector< Transform3DComponentInstance*> Transform3DComponentInstance::sComponentInstanceList;

Transform3DComponentInstance::Transform3DComponentInstance(const Transform3DComponent& componentData, const ComponentObject& owner)
{
   mComponentData = &componentData;
   mOwner = &owner;

   mGlobalBounds = false;

   mObjScale.set(1, 1, 1);
   mObjToWorld.identity();
   mWorldToObj.identity();

   mObjBox = Box3F(Point3F(0, 0, 0), Point3F(0, 0, 0));
   mWorldBox = Box3F(Point3F(0, 0, 0), Point3F(0, 0, 0));
   mWorldSphere = SphereF(Point3F(0, 0, 0), 0);

   mRenderObjToWorld.identity();
   mRenderWorldToObj.identity();
   mRenderWorldBox = Box3F(Point3F(0, 0, 0), Point3F(0, 0, 0));
   mRenderWorldSphere = SphereF(Point3F(0, 0, 0), 0);
}

Transform3DComponentInstance::~Transform3DComponentInstance()
{
}

void Transform3DComponentInstance::initPersistFields()
{
   Parent::initPersistFields();

   addProtectedField("position", TypeMatrixPosition, Offset(mObjToWorld, Transform3DComponentInstance),
      &_setFieldPosition, &defaultProtectedGetFn,
      "Object world position.");
   addProtectedField("rotation", TypeMatrixRotation, Offset(mObjToWorld, Transform3DComponentInstance),
      &_setFieldRotation, &defaultProtectedGetFn,
      "Object world orientation.");
   addProtectedField("scale", TypePoint3F, Offset(mObjScale, Transform3DComponentInstance),
      &_setFieldScale, &defaultProtectedGetFn,
      "Object world scale.");
}

void Transform3DComponentInstance::destroyInstance()
{
   Transform3DComponentInstance::sComponentInstanceList.remove(this);

   delete this;
}

void Transform3DComponentInstance::update()
{
}

//
U32 Transform3DComponentInstance::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   if (stream->writeFlag(mask & TransformMask))
   {
      mathWrite(*stream, getTransform());
      mathWrite(*stream, getScale());
   }

   return retMask;
}

void Transform3DComponentInstance::unpackUpdate(NetConnection* con, BitStream* stream)
{
   Parent::unpackUpdate(con, stream);

   if (stream->readFlag())  // TransformMask
   {
      mathRead(*stream, &mObjToWorld);
      mathRead(*stream, &mObjScale);

      setTransform(mObjToWorld);
   }
}

#pragma region World/Transform
bool Transform3DComponentInstance::_setFieldPosition(void* object, const char* index, const char* data)
{
   Transform3DComponentInstance* so = static_cast<Transform3DComponentInstance*>(object);
   if (so)
   {
      MatrixF txfm(so->getTransform());
      Con::setData(TypeMatrixPosition, &txfm, 0, 1, &data);
      so->setTransform(txfm);
   }
   return false;
}

//-----------------------------------------------------------------------------

bool Transform3DComponentInstance::_setFieldRotation(void* object, const char* index, const char* data)
{
   Transform3DComponentInstance* so = static_cast<Transform3DComponentInstance*>(object);
   if (so)
   {
      MatrixF txfm(so->getTransform());
      Con::setData(TypeMatrixRotation, &txfm, 0, 1, &data);
      so->setTransform(txfm);
   }
   return false;
}

//-----------------------------------------------------------------------------

bool Transform3DComponentInstance::_setFieldScale(void* object, const char* index, const char* data)
{
   Transform3DComponentInstance* so = static_cast<Transform3DComponentInstance*>(object);
   if (so)
   {
      Point3F scale;
      Con::setData(TypePoint3F, &scale, 0, 1, &data);
      so->setScale(scale);
   }
   return false;
}

void Transform3DComponentInstance::setTransform(const MatrixF& mat)
{
   // This test is a bit expensive so turn it off in release.   
#ifdef TORQUE_DEBUG
   //AssertFatal( mat.isAffine(), "Transform3DComponentInstance::setTransform() - Bad transform (non affine)!" );
#endif

   PROFILE_SCOPE(Transform3DComponentInstance_setTransform);
   // PATHSHAPE
   //UpdateXformChange(mat);
   //PerformUpdatesForChildren(mat);
   // PATHSHAPE END

      // Update the transforms.

   mObjToWorld = mWorldToObj = mat;
   mWorldToObj.affineInverse();

   // Update the world-space AABB.

   resetWorldBox();

   // If we're in a SceneManager, sync our scene state.

   //if (mSceneManager != NULL)
   //   mSceneManager->notifyObjectDirty(this);

   setRenderTransform(mat);

   setMaskBits(TransformMask);

   //TODO: Manage this better with the graph/scene integration so we don't need to call back up to the owner.
   //this is principly a no-no in DOCs as the Entity shouldn't specifically care about anything the components do
   //but for editor integration atm, we gotta hack it
   auto obj = dynamic_cast<const Entity*>(mOwner);
   (const_cast<Entity*>(obj))->setTransform(mat);
}

//-----------------------------------------------------------------------------

void Transform3DComponentInstance::setScale(const VectorF& scale)
{
   AssertFatal(!mIsNaN(scale), "Transform3DComponentInstance::setScale() - The scale is NaN!");

   // Avoid unnecessary scaling operations.
   if (mObjScale.equal(scale))
      return;

   mObjScale = scale;
   setTransform(MatrixF(mObjToWorld));

   // Make sure that any subclasses of me get a chance to react to the
   // scale being changed.
   onScaleChanged();

   setMaskBits(ScaleMask);
}

void Transform3DComponentInstance::setForwardVector(VectorF newForward, VectorF upVector)
{
   MatrixF mat = getTransform();

   VectorF up(0.0f, 0.0f, 1.0f);
   VectorF axisX;
   VectorF axisY = newForward;
   VectorF axisZ;

   if (upVector != VectorF::Zero)
      up = upVector;

   // Validate and normalize input:  
   F32 lenSq;
   lenSq = axisY.lenSquared();
   if (lenSq < 0.000001f)
   {
      axisY.set(0.0f, 1.0f, 0.0f);
      Con::errorf("Transform3DComponentInstance::setForwardVector() - degenerate forward vector");
   }
   else
   {
      axisY /= mSqrt(lenSq);
   }


   lenSq = up.lenSquared();
   if (lenSq < 0.000001f)
   {
      up.set(0.0f, 0.0f, 1.0f);
      Con::errorf("Transform3DComponentInstance::setForwardVector() - degenerate up vector - too small");
   }
   else
   {
      up /= mSqrt(lenSq);
   }

   if (fabsf(mDot(up, axisY)) > 0.9999f)
   {
      Con::errorf("Transform3DComponentInstance::setForwardVector() - degenerate up vector - same as forward");
      // I haven't really tested this, but i think it generates something which should be not parallel to the previous vector:  
      F32 tmp = up.x;
      up.x = -up.y;
      up.y = up.z;
      up.z = tmp;
   }

   // construct the remaining axes:  
   mCross(axisY, up, &axisX);
   mCross(axisX, axisY, &axisZ);

   mat.setColumn(0, axisX);
   mat.setColumn(1, axisY);
   mat.setColumn(2, axisZ);

   setTransform(mat);
}

//-----------------------------------------------------------------------------

void Transform3DComponentInstance::resetWorldBox()
{
   AssertFatal(mObjBox.isValidBox(), "Transform3DComponentInstance::resetWorldBox - Bad object box!");

   mWorldBox = mObjBox;
   mWorldBox.minExtents.convolve(mObjScale);
   mWorldBox.maxExtents.convolve(mObjScale);
   mObjToWorld.mul(mWorldBox);

   AssertFatal(mWorldBox.isValidBox(), "Transform3DComponentInstance::resetWorldBox - Bad world box!");

   // Create mWorldSphere from mWorldBox
   mWorldBox.getCenter(&mWorldSphere.center);
   mWorldSphere.radius = (mWorldBox.maxExtents - mWorldSphere.center).len();

   // Update tracker links.

   //for (SceneObjectLink* link = mSceneObjectLinks; link != NULL;
   //   link = link->getNextLink())
   //   link->update();
}

//-----------------------------------------------------------------------------

void Transform3DComponentInstance::resetObjectBox()
{
   AssertFatal(mWorldBox.isValidBox(), "Transform3DComponentInstance::resetObjectBox - Bad world box!");

   mObjBox = mWorldBox;
   mWorldToObj.mul(mObjBox);

   Point3F objScale(mObjScale);
   objScale.setMax(Point3F((F32)POINT_EPSILON, (F32)POINT_EPSILON, (F32)POINT_EPSILON));
   mObjBox.minExtents.convolveInverse(objScale);
   mObjBox.maxExtents.convolveInverse(objScale);

   AssertFatal(mObjBox.isValidBox(), "Transform3DComponentInstance::resetObjectBox - Bad object box!");

   // Update the mWorldSphere from mWorldBox
   mWorldBox.getCenter(&mWorldSphere.center);
   mWorldSphere.radius = (mWorldBox.maxExtents - mWorldSphere.center).len();

   // Update scene managers.

   //for (SceneObjectLink* link = mSceneObjectLinks; link != NULL;
   //   link = link->getNextLink())
   //   link->update();
}

//-----------------------------------------------------------------------------

void Transform3DComponentInstance::setRenderTransform(const MatrixF& mat)
{
   PROFILE_START(Transform3DComponentInstance_setRenderTransform);
   mRenderObjToWorld = mRenderWorldToObj = mat;
   mRenderWorldToObj.affineInverse();

   AssertFatal(mObjBox.isValidBox(), "Bad object box!");
   resetRenderWorldBox();
   PROFILE_END();
}

//-----------------------------------------------------------------------------

void Transform3DComponentInstance::resetRenderWorldBox()
{
   AssertFatal(mObjBox.isValidBox(), "Bad object box!");

   mRenderWorldBox = mObjBox;
   mRenderWorldBox.minExtents.convolve(mObjScale);
   mRenderWorldBox.maxExtents.convolve(mObjScale);
   mRenderObjToWorld.mul(mRenderWorldBox);

   AssertFatal(mRenderWorldBox.isValidBox(), "Bad world box!");

   // Create mRenderWorldSphere from mRenderWorldBox.

   mRenderWorldBox.getCenter(&mRenderWorldSphere.center);
   mRenderWorldSphere.radius = (mRenderWorldBox.maxExtents - mRenderWorldSphere.center).len();
}

Point3F Transform3DComponentInstance::getPosition() const
{
   Point3F pos;
   mObjToWorld.getColumn(3, &pos);
   return pos;
}

//-----------------------------------------------------------------------------

Point3F Transform3DComponentInstance::getRenderPosition() const
{
   Point3F pos;
   mRenderObjToWorld.getColumn(3, &pos);
   return pos;
}

//-----------------------------------------------------------------------------

void Transform3DComponentInstance::setPosition(const Point3F& pos)
{
   AssertFatal(!mIsNaN(pos), "Transform3DComponentInstance::setPosition() - The position is NaN!");

   MatrixF xform = mObjToWorld;
   xform.setColumn(3, pos);
   setTransform(xform);
}
#pragma endregion

//==================================================================================================
//
//==================================================================================================
/*Transform3DComponentDirector::Transform3DComponentDirector() : Director()
{
   //Establish the timing we'll need
   mTimingGroup = DirectorManager::TimingGroup::PreSim;

   //Here, we listen to the Transform3DComponent's add and remove signaling.
   //If a Transform3DComponent(or in other directors, any other components we care about) are added/removed
   //we can process the component and it's owner to track valid entries the director actually cares about
   Transform3DComponent::getAddedComponentSignal().notify(this, &Transform3DComponentDirector::registerComponent);
   Transform3DComponent::getRemovedComponentSignal().notify(this, &Transform3DComponentDirector::unregisterComponent);
}

Transform3DComponentDirector::~Transform3DComponentDirector()
{
   mValidEntriesList.clear();
}

void Transform3DComponentDirector::registerComponent(ComponentObject* owner, const Component& comp)
{
   //We have a valid component we care about added to a ComponentObject
   //So lets create a ref and add it to the list if it's valid
   //Because this is called whenever a component this director cares about is added
   //We can only worry about Objects that match to ALL requirements. Otherwise, we can
   //completely ignore it for this director's purposes
   Transform3DComponentEntityRef ref;
   ref.owner = owner;
   ref.controlObj = owner->getComponentInstance<Transform3DComponentInstance>();
   //ref.transform = owner->getComponentInstance<Transform3DComponentInstance>();

   //If all valid, we finally add it
   if (ref.isValid())
      mValidEntriesList.push_back(ref);
}

void Transform3DComponentDirector::unregisterComponent(ComponentObject* owner, const Component& comp)
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

void Transform3DComponentDirector::update()
{
   //Now we loop over all the valid entries we've got and go to work
   for (U32 i = 0; i < mValidEntriesList.size(); i++)
   {
      Transform3DComponentEntityRef& ref = mValidEntriesList[i];

      //All good, so we'll pass in the stuff the component needs to do it's work, and let it crunch.
      //In other directors, we may have structs to pack complex data for the components to work off of.
      //The reson we do this is to keep the work the components do compartmentalized.
      //This keeps it more cache friendly, and also threadsafe when we don't have to worry about the components
      //needing to reach out to any other objects while they work.
      //ref.controlObj->update(move);
      ref.controlObj->update(nullptr);
   }
}*/
