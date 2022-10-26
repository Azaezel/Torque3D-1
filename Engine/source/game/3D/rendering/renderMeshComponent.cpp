#include "renderMeshComponent.h"
#include "gfx/gfxDrawUtil.h"
#include "game/Entity.h"
#include <gfx/gfxTransformSaver.h>
#include "scene/sceneRenderState.h"
#include "renderInstance/renderPassManager.h"

IMPLEMENT_CO_DATABLOCK_V1(RenderMeshComponent);

IMPL_COMP_REGISTER_SIGNALS(RenderMeshComponent);

RenderMeshComponent::RenderMeshComponent() : Component()
{

}

bool RenderMeshComponent::onAdd()
{
   if (!Parent::onAdd())
      return false;

   addComponentField("Shape", "The Shape Asset to be rendered by this component", "TypeShapeAssetId");

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
Vector< RenderMeshComponentInstance*> RenderMeshComponentInstance::sComponentInstanceList;

RenderMeshComponentInstance::RenderMeshComponentInstance(const RenderMeshComponent& componentData, const ComponentObject& owner)
{
   mComponentData = &componentData;
   mOwner = &owner;
}

RenderMeshComponentInstance::~RenderMeshComponentInstance()
{
}

void RenderMeshComponentInstance::destroyInstance()
{
   RenderMeshComponentInstance::sComponentInstanceList.remove(this);

   delete this;
}

void RenderMeshComponentInstance::update(const MatrixF& transform)
{
   SceneRenderState* state = DirectorManager::sceneRenderState;
   if (!state)
      return;

   ObjectRenderInst* ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->type = RenderPassManager::RIT_Editor;

   GFXTransformSaver saver;

   GFXStateBlockDesc desc;
   desc.setZReadWrite(true, false);
   desc.setBlend(true);
   desc.fillMode = GFXFillWireframe;
   Box3F bounds = Box3F(1);

   bounds.setCenter(transform.getPosition());

   //do the work
   GFX->getDrawUtil()->drawCube(desc, bounds, ColorI(255, 0, 0, 255));

   state->getRenderPass()->addInst(ri);
}

//==================================================================================================
//
//==================================================================================================
RenderMeshDirector::RenderMeshDirector() : Director()
{
   mTimingGroup = DirectorManager::Rendering;
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
