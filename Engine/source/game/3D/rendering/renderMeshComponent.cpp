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

void RenderMeshComponentInstance::update()
{
   GFXStateBlockDesc desc;
   desc.setZReadWrite(true, false);
   desc.setBlend(true);
   desc.fillMode = GFXFillWireframe;
   Box3F bounds = Box3F(1);

   //do the work
   GFX->getDrawUtil()->drawCube(desc, bounds, ColorI(255, 0, 0, 255));
}


//
//
RenderMeshDirector::RenderMeshDirector()
{
   mTimingGroup = DirectorManager::Rendering;
}

RenderMeshDirector::~RenderMeshDirector()
{
   RenderMeshComponentInstance::sComponentInstanceList.clear();
}

void RenderMeshDirector::update()
{
   for (U32 i = 0; i < RenderMeshComponentInstance::sComponentInstanceList.size(); i++)
   {
      RenderMeshComponentInstance* compInst = &RenderMeshComponentInstance::sComponentInstanceList[i];

      bool isClient = compInst->getOwnerEntity().isClientObject();

      compInst->update();
   }
}
