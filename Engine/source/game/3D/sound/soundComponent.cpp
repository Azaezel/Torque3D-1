#include "soundComponent.h"
#include "gfx/gfxDrawUtil.h"
#include "game/Entity.h"
#include <gfx/gfxTransformSaver.h>
#include "scene/sceneRenderState.h"
#include "renderInstance/renderPassManager.h"
#include "materials/baseMatInstance.h"

IMPLEMENT_CO_DATABLOCK_V1(SoundComponent);

IMPL_COMP_REGISTER_SIGNALS(SoundComponent);

SoundComponent::SoundComponent() : Component()
{
}

bool SoundComponent::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

void SoundComponent::consoleInit()
{
   Parent::consoleInit();

   //We'll register the SoundDirector to the DirectorManager, so it's ready to go at runtime
   DirectorManager::get()->mDirectors.push_back(new SoundDirector());
}

void SoundComponent::initPersistFields()
{
   Parent::initPersistFields();
}

void SoundComponent::packData(BitStream* stream)
{
   Parent::packData(stream);
}

void SoundComponent::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);
}

ComponentInstance* SoundComponent::createInstance(ComponentObject* owner)
{
   SoundComponentInstance* compInst = new SoundComponentInstance(*this, *owner);

   if (!compInst->registerObject())
   {
      Con::errorf("SoundComponent::createInstance() - failed to create instance");
      return nullptr;
   }

   setupFields(compInst, true);
   SoundComponentInstance::sComponentInstanceList.push_back(compInst);

   return compInst;
}

//==================================================================================================
//
//==================================================================================================
IMPLEMENT_CONOBJECT(SoundComponentInstance);

Vector< SoundComponentInstance*> SoundComponentInstance::sComponentInstanceList;

SoundComponentInstance::SoundComponentInstance(const SoundComponent& componentData, const ComponentObject& owner)
{
   mComponentData = &componentData;
   mOwner = &owner;
}

SoundComponentInstance::~SoundComponentInstance()
{
}

void SoundComponentInstance::initPersistFields()
{
   Parent::initPersistFields();
}

void SoundComponentInstance::destroyInstance()
{
   SoundComponentInstance::sComponentInstanceList.remove(this);

   delete this;
}

void SoundComponentInstance::update()
{
}

//
U32 SoundComponentInstance::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   return retMask;
}

void SoundComponentInstance::unpackUpdate(NetConnection* con, BitStream* stream)
{
   Parent::unpackUpdate(con, stream);
}

//==================================================================================================
//
//==================================================================================================
SoundDirector::SoundDirector() : Director()
{
   //Establish the timing we'll need
   mTimingGroup = DirectorManager::TimingGroup::PreSim;

   //Here, we listen to the SoundComponent's add and remove signaling.
   //If a SoundComponent(or in other directors, any other components we care about) are added/removed
   //we can process the component and it's owner to track valid entries the director actually cares about
   SoundComponent::getAddedComponentSignal().notify(this, &SoundDirector::registerComponent);
   SoundComponent::getRemovedComponentSignal().notify(this, &SoundDirector::unregisterComponent);
}

SoundDirector::~SoundDirector()
{
   mValidEntriesList.clear();
}

void SoundDirector::registerComponent(ComponentObject* owner, const Component& comp)
{
   //We have a valid component we care about added to a ComponentObject
   //So lets create a ref and add it to the list if it's valid
   //Because this is called whenever a component this director cares about is added
   //We can only worry about Objects that match to ALL requirements. Otherwise, we can
   //completely ignore it for this director's purposes
   SoundEntityRef ref;
   ref.owner = owner;
   ref.controlObj = owner->getComponentInstance<SoundComponentInstance>();
   //ref.transform = owner->getComponentInstance<Transform3DComponentInstance>();

   //If all valid, we finally add it
   if (ref.isValid())
      mValidEntriesList.push_back(ref);
}

void SoundDirector::unregisterComponent(ComponentObject* owner, const Component& comp)
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

void SoundDirector::update()
{
   //Now we loop over all the valid entries we've got and go to work
   for (U32 i = 0; i < mValidEntriesList.size(); i++)
   {
      SoundEntityRef& ref = mValidEntriesList[i];

      //All good, so we'll pass in the stuff the component needs to do it's work, and let it crunch.
      //In other directors, we may have structs to pack complex data for the components to work off of.
      //The reson we do this is to keep the work the components do compartmentalized.
      //This keeps it more cache friendly, and also threadsafe when we don't have to worry about the components
      //needing to reach out to any other objects while they work.
      ref.controlObj->update();
   }
}
