#pragma once

#include "soundComponent.h"
#include "console/engineAPI.h"

#include "sfx/sfxSystem.h"
#include "sfx/sfxSource.h"
#include "sfx/sfxTrack.h"
#include "sfx/sfxDescription.h"
#include "T3D/sfx/sfx3DWorld.h"

#include "sfx/sfxTrack.h"
#include "sfx/sfxTypes.h"

//----------------------------------------------------------------------------
DefineEngineMethod(SoundComponentInstance, playAudio, bool, (S32 slot, SFXTrack* track), (0, nullAsType<SFXTrack*>()),
   "@brief Attach a sound to this shape and start playing it.\n\n"

   "@param slot Audio slot index for the sound (valid range is 0 - 3)\n" // 3 = ShapeBase::MaxSoundThreads-1
   "@param track SFXTrack to play\n"
   "@return true if the sound was attached successfully, false if failed\n\n"

   "@see stopAudio()\n")
{
   if (track && slot >= 0 && slot < SoundComponentInstance::MaxSoundThreads) {
      object->playAudio(slot, track);
      return true;
   }
   return false;
}

DefineEngineMethod(SoundComponentInstance, stopAudio, bool, (S32 slot), ,
   "@brief Stop a sound started with playAudio.\n\n"

   "@param slot audio slot index (started with playAudio)\n"
   "@return true if the sound was stopped successfully, false if failed\n\n"

   "@see playAudio()\n")
{
   if (slot >= 0 && slot < SoundComponentInstance::MaxSoundThreads) {
      object->stopAudio(slot);
      return true;
   }
   return false;
}
