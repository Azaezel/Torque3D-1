#include "renderMeshComponent.h"
#include "gfx/gfxDrawUtil.h"
#include "game/Entity.h"
#include <gfx/gfxTransformSaver.h>
#include "scene/sceneRenderState.h"
#include "renderInstance/renderPassManager.h"
#include "materials/baseMatInstance.h"

IMPLEMENT_CO_DATABLOCK_V1(RenderMeshComponent);

IMPL_COMP_REGISTER_SIGNALS(RenderMeshComponent);

RenderMeshComponent::RenderMeshComponent() : Component()
{
}

bool RenderMeshComponent::onAdd()
{
   if (!Parent::onAdd())
      return false;

   //Registers a component field to the field name 'shapeAsset', which via the init persist fields in the Instance, hooks back into the instance's Shape asset stuff
   addComponentField("ShapeAsset", "The Shape Asset to be rendered by this component", "TypeShapeAssetId");

   return true;
}

void RenderMeshComponent::consoleInit()
{
   Parent::consoleInit();

   //We'll register the RenderMeshDirector to the DirectorManager, so it's ready to go at runtime
   DirectorManager::get()->mDirectors.push_back(new RenderMeshDirector());
}

void RenderMeshComponent::initPersistFields()
{
   Parent::initPersistFields();
}

void RenderMeshComponent::packData(BitStream* stream)
{
   Parent::packData(stream);
}

void RenderMeshComponent::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);
}

ComponentInstance* RenderMeshComponent::createInstance(ComponentObject* owner)
{
   RenderMeshComponentInstance* compInst = new RenderMeshComponentInstance(*this, *owner);

   setupFields(compInst, true);
   RenderMeshComponentInstance::sComponentInstanceList.push_back(compInst);
   
   return compInst;
}

//==================================================================================================
//
//==================================================================================================
IMPLEMENT_CONOBJECT(RenderMeshComponentInstance);

Vector< RenderMeshComponentInstance*> RenderMeshComponentInstance::sComponentInstanceList;

RenderMeshComponentInstance::RenderMeshComponentInstance(const RenderMeshComponent& componentData, const ComponentObject& owner)
{
   mComponentData = &componentData;
   mOwner = &owner;

   //Init the shape asset
   INIT_ASSET(Shape);

   mShapeInstance = nullptr;
}

RenderMeshComponentInstance::~RenderMeshComponentInstance()
{
}

void RenderMeshComponentInstance::initPersistFields()
{
   Parent::initPersistFields();

   //As noted above, we set up the init persist fields here for the shape asset field, which shares a name back with the componentField. This creates a nice truncated list
   //of component fields that still tie back against the main instance fields and thus the internal vars. This lets us interop with the inspector even though the instances
   //are not registered/script-aware objects
   INITPERSISTFIELD_SHAPEASSET(Shape, RenderMeshComponentInstance, "");
}

void RenderMeshComponentInstance::updateShape()
{
   //If no asset, bail
   if (mShapeAsset.isNull())
      return;

   //Clear any existing shape instances
   if (mShapeInstance)
      SAFE_DELETE(mShapeInstance);
   mShape = NULL;

   // Attempt to get the resource from the ResourceManager
   mShape = mShapeAsset->getShapeResource();

   if (!mShape)
   {
      //Something went wrong. Bail.
      Con::errorf("RenderMeshComponentInstance::updateShape() - Unable to load shape: %s", mShapeAsset.getAssetId());
      return;
   }

   //set up the shape instance
   setupShape();

   //Do this on both the server and client
   S32 materialCount = mShapeAsset->getShape()->materialList->getMaterialNameList().size(); //mMeshAsset->getMaterialCount();

   if (isServerObject())
   {

   }

   /*if (mOwner != NULL)
   {
      Point3F min, max, pos;
      pos = mOwner->getPosition();

      mOwner->getWorldToObj().mulP(pos);

      min = mMeshAsset->getShape()->mBounds.minExtents;
      max = mMeshAsset->getShape()->mBounds.maxExtents;

      mBounds.set(min, max);
      mScale = mOwner->getScale();
      mTransform = mOwner->getRenderTransform();

      mOwner->setObjectBox(Box3F(min, max));

      mOwner->resetWorldBox();

      if (mOwner->getSceneManager() != NULL)
         mOwner->getSceneManager()->notifyObjectDirty(mOwner);
   }*/

   //finally, notify that our shape was changed
   //onShapeInstanceChanged.trigger(this);
}

void RenderMeshComponentInstance::setupShape()
{
   mShapeInstance = new TSShapeInstance(mShapeAsset->getShape(), true);
}

void RenderMeshComponentInstance::destroyInstance()
{
   RenderMeshComponentInstance::sComponentInstanceList.remove(this);

   delete this;
}

void RenderMeshComponentInstance::update(const MatrixF& _transform)
{
   SceneRenderState* state = DirectorManager::sceneRenderState;
   if (!state)
      return;

   ObjectRenderInst* ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->type = RenderPassManager::RIT_Editor;

   transform = _transform;

   //do the work
   ri->renderDelegate.bind(this, &RenderMeshComponentInstance::drawDebug);
   state->getRenderPass()->addInst(ri);

   if (!mEnabled || !mOwner || !mShapeInstance)
      return;

   Point3F cameraOffset;
   transform.getColumn(3, &cameraOffset);
   cameraOffset -= state->getDiffuseCameraPosition();
   F32 dist = cameraOffset.len();
   if (dist < 0.01f)
      dist = 0.01f;

   Point3F objScale = transform.getScale();
   F32 invScale = (1.0f / getMax(getMax(objScale.x, objScale.y), objScale.z));

   mShapeInstance->setDetailFromDistance(state, dist * invScale);

   if (mShapeInstance->getCurrentDetail() < 0)
      return;

   GFXTransformSaver saver;

   // Set up our TS render state.
   TSRenderState rdata;
   rdata.setSceneState(state);
   rdata.setFadeOverride(1.0f);
   rdata.setOriginSort(false);

   // We might have some forward lit materials
   // so pass down a query to gather lights.
   //LightQuery query;
  // query.init(mOwner->getWorldSphere());
   //rdata.setLightQuery(&query);

  /* if (mOwner->isMounted())
   {
      MatrixF wrldPos = mOwner->getWorldTransform();
      Point3F wrldPosPos = wrldPos.getPosition();

      Point3F mntPs = mat.getPosition();
      EulerF mntRt = RotationF(mat).asEulerF();

      bool tr = true;
   }*/

   //mat.scale(objScale);
   GFX->setWorldMatrix(transform);

   mShapeInstance->render(rdata);
}

void RenderMeshComponentInstance::drawDebug(ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance*)
{
   GFXTransformSaver saver;

   GFXStateBlockDesc desc;
   desc.setZReadWrite(true, false);
   desc.setBlend(true);
   desc.fillMode = GFXFillWireframe;
   Box3F bounds = Box3F(1);

   bounds.setCenter(transform.getPosition());

   GFX->getDrawUtil()->drawCube(desc, bounds, ColorI(255, 0, 0, 255));
}

//
U32 RenderMeshComponentInstance::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   PACKDATA_ASSET(Shape);

   return retMask;
}

void RenderMeshComponentInstance::unpackUpdate(NetConnection* con, BitStream* stream)
{
   Parent::unpackUpdate(con, stream);

   UNPACKDATA_ASSET(Shape);

   updateShape();
}

//==================================================================================================
//
//==================================================================================================
RenderMeshDirector::RenderMeshDirector() : Director()
{
   //Establish the timing we'll need
   mTimingGroup = DirectorManager::TimingGroup::Rendering;

   //Here, we listen to the RenderMeshComponent's add and remove signaling.
   //If a RenderMeshComponent(or in other directors, any other components we care about) are added/removed
   //we can process the component and it's owner to track valid entries the director actually cares about
   RenderMeshComponent::getAddedComponentSignal().notify(this, &RenderMeshDirector::registerComponent);
   RenderMeshComponent::getRemovedComponentSignal().notify(this, &RenderMeshDirector::unregisterComponent);
}

RenderMeshDirector::~RenderMeshDirector()
{
   mValidEntriesList.clear();
}

void RenderMeshDirector::registerComponent(ComponentObject* owner, const Component& comp)
{
   //We have a valid component we care about added to a ComponentObject
   //So lets create a ref and add it to the list if it's valid
   //Because this is called whenever a component this director cares about is added
   //We can only worry about Objects that match to ALL requirements. Otherwise, we can
   //completely ignore it for this director's purposes
   RenderMeshEntityRef ref;
   ref.owner = owner;
   ref.mesh = owner->getComponentInstance<RenderMeshComponentInstance>();
   //ref.transform = owner->getComponentInstance<Transform3DComponentInstance>();

   //If all valid, we finally add it
   if(ref.isValid())
      mValidEntriesList.push_back(ref);
}

void RenderMeshDirector::unregisterComponent(ComponentObject* owner, const Component& comp)
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

void RenderMeshDirector::update()
{
   //Now we loop over all the valid entries we've got and go to work
   for (U32 i = 0; i < mValidEntriesList.size(); i++)
   {
      RenderMeshEntityRef& ref = mValidEntriesList[i];

      //This needs to be done more cleanly, but for rendering, we need to know if we're
      //on the client or not. If we aren't, no point in continuing
      Entity* ownerEntity = static_cast<Entity*>(ref.owner);
      bool isClient = ownerEntity->isClientObject();
      if (!isClient)
         continue;

      //All good, so we'll pass in the stuff the component needs to do it's work, and let it crunch.
      //In other directors, we may have structs to pack complex data for the components to work off of.
      //The reson we do this is to keep the work the components do compartmentalized.
      //This keeps it more cache friendly, and also threadsafe when we don't have to worry about the components
      //needing to reach out to any other objects while they work.
      ref.mesh->update(/*ref.transform->getWorldTransform()*/ownerEntity->getTransform());
   }
}
