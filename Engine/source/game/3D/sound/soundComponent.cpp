#include "soundComponent.h"
#include "soundComponent_scriptBinding.h"
#include "gfx/gfxDrawUtil.h"
#include "game/Entity.h"
#include <gfx/gfxTransformSaver.h>
#include "scene/sceneRenderState.h"
#include "renderInstance/renderPassManager.h"
#include "materials/baseMatInstance.h"

// Timeout for non-looping sounds on a channel
static SimTime sAudioTimeout = 500;

IMPLEMENT_CO_DATABLOCK_V1(SoundComponent);

IMPL_COMP_REGISTER_SIGNALS(SoundComponent);

SoundComponent::SoundComponent() : Component()
{
   mNetworked = true;
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

   for (U32 slotNum = 0; slotNum < MaxSoundThreads; slotNum++) {
      mSoundThread[slotNum].play = false;
      mSoundThread[slotNum].profile = 0;
      mSoundThread[slotNum].sound = 0;

      INIT_ASSET_ARRAY(Sound, slotNum);

      mPlay[slotNum] = false;
   }
}

SoundComponentInstance::~SoundComponentInstance()
{
}

void SoundComponentInstance::initPersistFields()
{
   Parent::initPersistFields();

   INITPERSISTFIELD_SOUNDASSET_ARRAY(Sound, MaxSoundThreads, SoundComponentInstance, "The source shape asset.");

   addGroup("ComponentFields");
   addProtectedField("play", TypeBool, Offset(mPlay, SoundComponentInstance),
      &_autoplay, &defaultProtectedGetFn, MaxSoundThreads, "Whether playback of the emitter's sound should start as soon as the emitter object is added to the level.\n"
      "If this is true, the emitter will immediately start to play when the level is loaded.");
   endGroup("ComponentFields");
}

bool SoundComponentInstance::_autoplay(void* object, const char* index, const char* data)
{
   U32 slotNum = (index != NULL) ? dAtoui(index) : 0;
   SoundComponentInstance* component = reinterpret_cast<SoundComponentInstance*>(object);
   component->mPlay[slotNum] = dAtoui(data);
   if (component->mPlay[slotNum] && component->mSoundAsset[slotNum].notNull())
      component->playAudio(slotNum, component->mSoundAsset[slotNum]->getSfxProfile());
   else
      component->stopAudio(slotNum);

   return false;
}

void SoundComponentInstance::destroyInstance()
{
   SoundComponentInstance::sComponentInstanceList.remove(this);

   delete this;
}

void SoundComponentInstance::update(StrongRefPtr<Transform3DComponentInstance> transformComp)
{
   if (transformComp.isValid())
   {
      //if we have a transform component, we become a 3d sound emitter
      for (S32 slotNum = 0; slotNum < MaxSoundThreads; slotNum++)
      {
         SFXSource* source = mSoundThread[slotNum].sound;
         if (source)
            source->setTransform(transformComp->getTransform());
      }
   }
}

//
U32 SoundComponentInstance::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
   U32 retMask = Parent::packUpdate(con, mask, stream);

   if (mask & InitialUpdateMask)
   {
      // mask off sounds that aren't playing
      S32 slotNum;
      for (slotNum = 0; slotNum < MaxSoundThreads; slotNum++)
         if (!mSoundThread[slotNum].play)
            mask &= ~(SoundMaskN << slotNum);
   }

   if (stream->writeFlag(mask & SoundMask))
   {
      for (S32 slotNum = 0; slotNum < MaxSoundThreads; slotNum++)
      {
         Sound& st = mSoundThread[slotNum];

         if (stream->writeFlag(mask & (SoundMaskN << slotNum)))
         {
            if (stream->writeFlag(st.play))
               PACKDATA_SOUNDASSET_ARRAY(Sound, slotNum);
         }
      }
   }

   return retMask;
}

void SoundComponentInstance::unpackUpdate(NetConnection* con, BitStream* stream)
{
   Parent::unpackUpdate(con, stream);

   if (stream->readFlag())
   {
      for (S32 slotNum = 0; slotNum < MaxSoundThreads; slotNum++)
      {
         if (stream->readFlag())
         {
            Sound& st = mSoundThread[slotNum];
            st.play = stream->readFlag();
            if (st.play)
            {
               UNPACKDATA_SOUNDASSET_ARRAY(Sound, slotNum);

               if (mSoundAsset[slotNum].notNull())
                  st.profile = mSoundAsset[slotNum]->getSfxProfile();
               else
                  st.profile = NULL;

            }

            //if (isProperlyAdded())
            updateAudioState(st);
         }
      }
   }
}

void SoundComponentInstance::playAudio(U32 slotNum, SFXTrack* _profile)
{
   AssertFatal(slotNum < MaxSoundThreads, "ShapeBase::playAudio() bad slot index");
   SFXTrack* profile;

   if (_profile != NULL)
   {
      profile = _profile;
   }
   else
   {
      if (mSoundAsset[slotNum].notNull())
         profile = mSoundAsset[slotNum]->getSfxProfile();
   }

   Sound& st = mSoundThread[slotNum];
   if (profile && (!st.play || st.profile != profile))
   {
      setMaskBits(SoundMaskN << slotNum);
      st.play = true;
      st.profile = profile;
      updateAudioState(st);
   }
}

void SoundComponentInstance::stopAudio(U32 slotNum)
{
   AssertFatal(slotNum < MaxSoundThreads, "ShapeBase::stopAudio() bad slot index");

   Sound& st = mSoundThread[slotNum];
   if (st.play)
   {
      st.play = false;
      setMaskBits(SoundMaskN << slotNum);
      updateAudioState(st);
   }
}

void SoundComponentInstance::updateServerAudio()
{
   // Timeout non-looping sounds
   for (S32 slotNum = 0; slotNum < MaxSoundThreads; slotNum++)
   {
      Sound& st = mSoundThread[slotNum];
      if (st.play && st.timeout && st.timeout < Sim::getCurrentTime())
      {
         //clearMaskBits(SoundMaskN << slotNum);
         st.play = false;
      }
   }
}

void SoundComponentInstance::updateAudioState(Sound& st)
{
   SFX_DELETE(st.sound);

   if (st.play && st.profile)
   {
      if (isClientObject())
      {
         //if (Sim::findObject(SimObjectId((uintptr_t)st.profile), st.profile))
        // {
         st.sound = SFX->createSource(st.profile);
         if (st.sound)
            st.sound->play();
         //}
         else
            st.play = false;
      }
      else
      {
         // Non-looping sounds timeout on the server
         st.timeout = 0;
         if (!st.profile->getDescription()->mIsLooping)
            st.timeout = Sim::getCurrentTime() + sAudioTimeout;
      }
   }
   else
      st.play = false;
}

//==================================================================================================
//
//==================================================================================================
SoundDirector::SoundDirector() : Director()
{
   //Establish the timing we'll need
   mTimingGroup = DirectorManager::TimingGroup::PostSim;

   DIRECTOR_SUBSCRIBE_SIGNALS(SoundDirector, SoundComponent);
   DIRECTOR_SUBSCRIBE_SIGNALS(SoundDirector, Transform3DComponent);
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
   ref.soundComp = owner->getComponentInstance<SoundComponentInstance>();
   ref.transformComp = owner->getComponentInstance<Transform3DComponentInstance>(); //this isn't required, but if we have a transform defined, it becomes a 3d sound, rather than a 2d sound

   //see if we have an existing
   S32 existingId = mValidEntriesList.find_next(ref, 0);

   //If all valid, we finally add it
   if (ref.isValid())
      mValidEntriesList.push_back_unique(ref);
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
      ref.soundComp->update(ref.transformComp);
   }
}
