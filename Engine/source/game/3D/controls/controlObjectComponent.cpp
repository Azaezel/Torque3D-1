#include "controlObjectComponent.h"
#include "gfx/gfxDrawUtil.h"
#include "game/Entity.h"
#include <gfx/gfxTransformSaver.h>
#include "scene/sceneRenderState.h"
#include "renderInstance/renderPassManager.h"
#include "materials/baseMatInstance.h"

IMPLEMENT_CO_DATABLOCK_V1(ControlObjectComponent);

IMPL_COMP_REGISTER_SIGNALS(ControlObjectComponent);

ControlObjectComponent::ControlObjectComponent() : Component()
{
}

bool ControlObjectComponent::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

void ControlObjectComponent::consoleInit()
{
   Parent::consoleInit();

   //We'll register the ControlObjectDirector to the DirectorManager, so it's ready to go at runtime
   DirectorManager::get()->mDirectors.push_back(new ControlObjectDirector());
}

void ControlObjectComponent::initPersistFields()
{
   Parent::initPersistFields();
}

void ControlObjectComponent::packData(BitStream* stream)
{
   Parent::packData(stream);
}

void ControlObjectComponent::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);
}

ComponentInstance* ControlObjectComponent::createInstance(ComponentObject* owner)
{
   ControlObjectComponentInstance* compInst = new ControlObjectComponentInstance(*this, *owner);

   if (!compInst->registerObject())
   {
      Con::errorf("ControlObjectComponent::createInstance() - failed to create instance");
      return nullptr;
   }

   setupFields(compInst, true);
   ControlObjectComponentInstance::sComponentInstanceList.push_back(compInst);

   return compInst;
}

//==================================================================================================
//
//==================================================================================================
IMPLEMENT_CONOBJECT(ControlObjectComponentInstance);

Vector< ControlObjectComponentInstance*> ControlObjectComponentInstance::sComponentInstanceList;

ControlObjectComponentInstance::ControlObjectComponentInstance(const ControlObjectComponent& componentData, const ComponentObject& owner)
{
   mComponentData = &componentData;
   mOwner = &owner;
}

ControlObjectComponentInstance::~ControlObjectComponentInstance()
{
}

void ControlObjectComponentInstance::initPersistFields()
{
   Parent::initPersistFields();
}

void ControlObjectComponentInstance::destroyInstance()
{
   ControlObjectComponentInstance::sComponentInstanceList.remove(this);

   delete this;
}

void ControlObjectComponentInstance::update()
{
}

//
U32 ControlObjectComponentInstance::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   return retMask;
}

void ControlObjectComponentInstance::unpackUpdate(NetConnection* con, BitStream* stream)
{
   Parent::unpackUpdate(con, stream);
}

//==================================================================================================
//
//==================================================================================================
ControlObjectDirector::ControlObjectDirector() : Director()
{
   //Establish the timing we'll need
   mTimingGroup = DirectorManager::TimingGroup::PreSim;

   //Here, we listen to the ControlObjectComponent's add and remove signaling.
   //If a ControlObjectComponent(or in other directors, any other components we care about) are added/removed
   //we can process the component and it's owner to track valid entries the director actually cares about
   ControlObjectComponent::getAddedComponentSignal().notify(this, &ControlObjectDirector::registerComponent);
   ControlObjectComponent::getRemovedComponentSignal().notify(this, &ControlObjectDirector::unregisterComponent);
}

ControlObjectDirector::~ControlObjectDirector()
{
   mValidEntriesList.clear();
}

void ControlObjectDirector::registerComponent(ComponentObject* owner, const Component& comp)
{
   //We have a valid component we care about added to a ComponentObject
   //So lets create a ref and add it to the list if it's valid
   //Because this is called whenever a component this director cares about is added
   //We can only worry about Objects that match to ALL requirements. Otherwise, we can
   //completely ignore it for this director's purposes
   ControlObjectEntityRef ref;
   ref.owner = owner;
   ref.controlObj = owner->getComponentInstance<ControlObjectComponentInstance>();
   //ref.transform = owner->getComponentInstance<Transform3DComponentInstance>();

   //If all valid, we finally add it
   if (ref.isValid())
      mValidEntriesList.push_back(ref);
}

void ControlObjectDirector::unregisterComponent(ComponentObject* owner, const Component& comp)
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

void ControlObjectDirector::update()
{
   //Now we loop over all the valid entries we've got and go to work
   for (U32 i = 0; i < mValidEntriesList.size(); i++)
   {
      ControlObjectEntityRef& ref = mValidEntriesList[i];

      //All good, so we'll pass in the stuff the component needs to do it's work, and let it crunch.
      //In other directors, we may have structs to pack complex data for the components to work off of.
      //The reson we do this is to keep the work the components do compartmentalized.
      //This keeps it more cache friendly, and also threadsafe when we don't have to worry about the components
      //needing to reach out to any other objects while they work.
      ref.controlObj->update();
   }
}
