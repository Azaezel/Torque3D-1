#include "renderMeshComponent.h"
#include "gfx/gfxDrawUtil.h"

IMPLEMENT_CO_DATABLOCK_V1(RenderMeshComponent);

RenderMeshComponent::RenderMeshComponent()
{

}

bool RenderMeshComponent::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

void RenderMeshComponent::consoleInit()
{
   Parent::consoleInit();

   DirectorManager::get()->mDirectors.push_back(RenderMeshDirector());
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

ComponentInstance RenderMeshComponent::createInstance(Entity* owner) const
{
   RenderMeshComponentInstance compInst = RenderMeshComponentInstance(*this, *owner);
   RenderMeshComponentInstance::sComponentInstanceList.push_back(compInst);
   return compInst;
}

bool RenderMeshComponent::addComponent(Entity* ent)
{
   RenderMeshComponent::getAddedSignal().trigger(ent, *this);
}

bool RenderMeshComponent::removeComponent(Entity* ent)
{
   RenderMeshComponent::getAddedSignal().trigger(ent, *this);
}

//
//
Vector< RenderMeshComponentInstance> RenderMeshComponentInstance::sComponentInstanceList;

RenderMeshComponentInstance::RenderMeshComponentInstance(const RenderMeshComponent& componentData, const Entity& ownerEntity)
{
   mComponentData = &componentData;
   mOwner = &ownerEntity;
}

RenderMeshComponentInstance::~RenderMeshComponentInstance()
{
}

void RenderMeshComponentInstance::destroyInstance()
{
   //RenderMeshComponentInstance::sComponentInstanceList.remove(*this);
}

void RenderMeshComponentInstance::update(const MatrixF& transform)
{
   GFXStateBlockDesc desc;
   desc.setZReadWrite(true, false);
   desc.setBlend(true);
   desc.fillMode = GFXFillWireframe;
   Box3F bounds = Box3F(1);

   bounds.setCenter(transform.getPosition());

   //do the work
   GFX->getDrawUtil()->drawCube(desc, bounds, ColorI(255, 0, 0, 255));
}


//
//
RenderMeshDirector::RenderMeshDirector()
{
   mTimingGroup = DirectorManager::Rendering;
   RenderMeshComponent::getAddedSignal().notify(this, registerComponent);
   RenderMeshComponent::getRemovedSignal().notify(this, unregisterComponent);
}

RenderMeshDirector::~RenderMeshDirector()
{
   RenderMeshComponentInstance::sComponentInstanceList.clear();
}

void RenderMeshDirector::registerComponent(Entity* entity, const Component& comp)
{
   RenderMeshEntityRef ref;
   ref.ownerEntity = entity;
   ref.mesh = entity->getComponentInstance<RenderMeshComponentInstance>();
   ref.transform = entity->getComponentInstance<Transform3DComponentInstance>();

   if(ref.isValid())
      mValidEntriesList.push_back(ref);
}

void RenderMeshDirector::unregisterComponent(Entity* entity, const Component& comp)
{
   for (U32 i = 0; i < mValidEntriesList.size(); i++)
   {
      if (mValidEntriesList[i].ownerEntity == entity)
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

      bool isClient = ref.ownerEntity->isClientObject();
      if (!isClient)
         continue;

      ref.mesh->update(ref.transform->getWorldTransform());
   }
}
