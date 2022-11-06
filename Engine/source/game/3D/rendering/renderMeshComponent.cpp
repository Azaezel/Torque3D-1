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

   addComponentField("ShapeAsset", "The Shape Asset to be rendered by this component", "TypeShapeAssetId");

   return true;
}

void RenderMeshComponent::consoleInit()
{
   Parent::consoleInit();

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

   INIT_ASSET(Shape);

   mShapeInstance = nullptr;
}

RenderMeshComponentInstance::~RenderMeshComponentInstance()
{
}

void RenderMeshComponentInstance::initPersistFields()
{
   Parent::initPersistFields();

   INITPERSISTFIELD_SHAPEASSET(Shape, RenderMeshComponentInstance, "");
}

void RenderMeshComponentInstance::updateShape()
{
   if (mShapeAsset.isNull())
      return;

   if (mShapeInstance)
      SAFE_DELETE(mShapeInstance);
   mShape = NULL;

   // Attempt to get the resource from the ResourceManager
   mShape = mShapeAsset->getShapeResource();

   if (!mShape)
   {
      Con::errorf("RenderMeshComponentInstance::updateShape() - Unable to load shape: %s", mShapeAsset.getAssetId());
      return;
   }

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
   mTimingGroup = DirectorManager::TimingGroup::Rendering;
   RenderMeshComponent::getAddedComponentSignal().notify(this, &RenderMeshDirector::registerComponent);
   RenderMeshComponent::getRemovedComponentSignal().notify(this, &RenderMeshDirector::unregisterComponent);
}

RenderMeshDirector::~RenderMeshDirector()
{
   mValidEntriesList.clear();
}

void RenderMeshDirector::registerComponent(ComponentObject* owner, const Component& comp)
{
   RenderMeshEntityRef ref;
   ref.owner = owner;
   ref.mesh = owner->getComponentInstance<RenderMeshComponentInstance>();
   //ref.transform = owner->getComponentInstance<Transform3DComponentInstance>();

   if(ref.isValid())
      mValidEntriesList.push_back(ref);
}

void RenderMeshDirector::unregisterComponent(ComponentObject* owner, const Component& comp)
{
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
   for (U32 i = 0; i < mValidEntriesList.size(); i++)
   {
      RenderMeshEntityRef& ref = mValidEntriesList[i];

      Entity* ownerEntity = static_cast<Entity*>(ref.owner);
      bool isClient = ownerEntity->isClientObject();
      if (!isClient)
         continue;

      ref.mesh->update(/*ref.transform->getWorldTransform()*/ownerEntity->getTransform());
   }
}
