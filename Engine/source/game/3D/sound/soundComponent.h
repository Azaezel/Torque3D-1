#pragma once

#include "game/components/component.h"
#include "game/components/componentInstance.h"

#include "game/directors/directorManager.h"
#include <game/3D/transforms/transform3DComponent.h>

#include "sfx/sfxSystem.h"
#include "sfx/sfxSource.h"
#include "sfx/sfxTrack.h"
#include "sfx/sfxDescription.h"
#include "T3D/sfx/sfx3DWorld.h"

#include "sfx/sfxTrack.h"
#include "sfx/sfxTypes.h"

class SoundDirector;

class SoundComponent : public Component
{
   typedef Component Parent;

private:
   void onShapeChange() {}

public:
   SoundComponent();

   DECLARE_CONOBJECT(SoundComponent);

   bool onAdd();
   static void initPersistFields();
   static void consoleInit();

   /// <summary>
   /// See Component::packData()
   /// </summary>
   virtual void packData(BitStream* stream);
   /// <summary>
   /// See Component::unpackData()
   /// </summary>
   virtual void unpackData(BitStream* stream);

   /// <summary>
   /// This sets up some common, boilerplate signal calls that hook into the Director notifications
   /// Namely, when a component is added or removed, the signals will inform the associated director
   /// And the event can be handled
   /// </summary>
   COMP_REGISTER_SIGNALS(SoundComponent);

   /// <summary>
   /// Creates a SoundComponentInstance based on this template SoundComponent.
   /// Also invokes setupFields to ensure that the SoundComponentInstance is fully templated from this SoundComponent
   /// Once created, the SoundComponent is added to it's own static master list for self-management and retention
   /// </summary>
   /// <param name="owner">Owner ComponentObject to associate to the SoundComponentInstance</param>
   /// <returns>The created ComponentInstance</returns>
   virtual ComponentInstance* createInstance(ComponentObject* owner);
};

class Move;

//
class SoundComponentInstance : public ComponentInstance
{
   typedef ComponentInstance Parent;

   friend SoundComponent;
   friend SoundDirector;

public:
   enum PublicConstants
   {
      MaxSoundThreads = 4,            ///< Should be a power of 2
   };

private:
   static Vector<SoundComponentInstance*> sComponentInstanceList;


   /// @name Network state masks
   /// @{

   ///
   enum SoundComponentMasks
   {
      SoundMaskN = Parent::NextFreeMask << 6,       ///< Extends + MaxSoundThreads bits
   };

   enum BaseMaskConstants
   {
      SoundMask = (SoundMaskN << MaxSoundThreads) - SoundMaskN,
   };
   /// @name Scripted Sound
   /// @{
   struct Sound {
      bool play;                    ///< Are we playing this sound?
      SimTime timeout;              ///< Time until we stop playing this sound.
      SFXTrack* profile;            ///< Profile on server
      SFXSource* sound;             ///< Sound on client
      Sound()
      {
         play = false;
         timeout = 0;
         profile = NULL;
         sound = NULL;
      }
   };

   DECLARE_SOUNDASSET_ARRAY(SoundComponentInstance, Sound, MaxSoundThreads);
   DECLARE_ASSET_ARRAY_SETGET(SoundComponentInstance, Sound);

   Sound mSoundThread[MaxSoundThreads];

   bool mPlay[MaxSoundThreads];

public:
   DECLARE_CONOBJECT(SoundComponentInstance);

   /// <summary>
   /// Obligatory default constructor
   /// </summary>
   SoundComponentInstance() { mComponentData = nullptr; mOwner = nullptr; }
   /// <summary>
   /// The main constructor actually utilized by DOCs
   /// This will assign the template componentData and owner ComponentObject for this componentInstance
   /// </summary>
   /// <param name="componentData">Template Component</param>
   /// <param name="owner">Owner ComopnentObject</param>
   SoundComponentInstance(const SoundComponent& componentData, const ComponentObject& owner);
   ~SoundComponentInstance();

   static void initPersistFields();

   /// <summary>
   /// Destroys this ComponentInstance, removing it from the static list
   /// </summary>
   virtual void destroyInstance();

   /// <summary>
   /// Called by SoundDirector when it runs, this will render our shapeInstance(if we have one)
   /// </summary>
   /// <param name="transform">The transform to render at</param>
   virtual void update(StrongRefPtr<Transform3DComponentInstance> transformComp);

   /// <summary>
   /// See ComponentInstance::packUpdate();
   /// </summary>
   virtual U32 packUpdate(NetConnection* con, U32 mask, BitStream* stream);
   /// <summary>
   /// See ComponentInstance::unpackUpdate();
   /// </summary>
   virtual void unpackUpdate(NetConnection* con, BitStream* stream);

   static bool _previewSound(void* object, const char* index, const char* data);
   static bool _autoplay(void* object, const char* index, const char* data);

   virtual void playAudio(U32 slotNum, SFXTrack* profile = NULL);
   virtual void stopAudio(U32 slot);
   virtual void updateServerAudio();
   virtual void updateAudioState(Sound& st);
};

//
class SoundDirector : public Director
{
   typedef Director Parent;
   friend DirectorManager;

   /// <summary>
   /// A struct containing an owner ComponentObject and its relevent components
   /// If this Director tracks if it's a valid ref or not, so if any of the required components
   /// are missing, invalid, or disabled, we can easily skip it and move on without needing to
   /// re-juggle lists or dependency tracking
   /// </summary>
   struct SoundEntityRef
   {
      ComponentObject* owner;
      StrongRefPtr<SoundComponentInstance> soundComp;
      StrongRefPtr<Transform3DComponentInstance> transformComp;

      bool isValid()
      {
         if (owner != nullptr && !soundComp.isNull())
            return true;

         return false;
      }

      bool operator==(const SoundEntityRef& o) const {
         if (this->soundComp.isNull() || o.soundComp.isNull())
            return false;
         return o.soundComp.getPointer()->getId() == this->soundComp.getPointer()->getId();
      }
   };

private:
   /// <summary>
   /// A list of validated SoundEntityRef entries.
   /// </summary>
   Vector<SoundEntityRef> mValidEntriesList;

public:
   SoundDirector();
   ~SoundDirector();

   void registerComponent(ComponentObject* owner, const Component& comp);
   void unregisterComponent(ComponentObject* owner, const Component& comp);

   /// <summary>
   /// The main update function. Will iterate over valid SoundEntityRef's and invoke them to update
   /// In our case, this means we take the mesh ref to a SoundComponentInstance and have it render the shape
   /// </summary>
   virtual void update();
};
