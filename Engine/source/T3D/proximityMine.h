//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef _PROXIMITYMINE_H_
#define _PROXIMITYMINE_H_

#ifndef _ITEM_H_
   #include "T3D/item.h"
#endif

#include "T3D/assets/SoundAsset.h"

class ExplosionData;
class SFXTrack;
class ProximityMine;

//----------------------------------------------------------------------------

struct ProximityMineData: public ItemData
{
   typedef ItemData Parent;

   DECLARE_CALLBACK( void, onTriggered, ( ProximityMine* obj, SceneObject* target ) );
   DECLARE_CALLBACK( void, onExplode, ( ProximityMine* obj, Point3F pos ) );

public:
   F32               armingDelay;
   S32               armingSequence;
   DECLARE_SOUNDASSET(ProximityMineData, ArmSound);
   DECLARE_ASSET_SETGET(ProximityMineData, ArmSound);

   F32               autoTriggerDelay;
   bool              triggerOnOwner;
   F32               triggerRadius;
   F32               triggerSpeed;
   F32               triggerDelay;
   S32               triggerSequence;
   DECLARE_SOUNDASSET(ProximityMineData, TriggerSound);
   DECLARE_ASSET_SETGET(ProximityMineData, TriggerSound);

   F32               explosionOffset;

   ProximityMineData();
   DECLARE_CONOBJECT( ProximityMineData );
   static void initPersistFields();
   bool preload( bool server, String& errorStr ) override;
   void packData( BitStream* stream ) override;
   void unpackData( BitStream* stream ) override;
};

//----------------------------------------------------------------------------

class ProximityMine: public Item
{
   typedef Item Parent;

protected:
   enum MaskBits {
      DeployedMask   = Parent::NextFreeMask,
      ExplosionMask  = Parent::NextFreeMask << 1
   };

   enum State
   {
      Thrown = 0,       //!< Mine has been thrown, but not yet attached to a surface
      Deployed,         //!< Mine has attached to a surface but is not yet armed
      Armed,            //!< Mine is armed and will trigger if any object enters the trigger area
      Triggered,        //!< Mine has been triggered and will explode soon
      Exploded,         //!< Mine has exploded and will be deleted on the server shortly
      NumStates
   };

   ProximityMineData*   mDataBlock;

   TSThread*            mAnimThread;
   SceneObject*         mOwner;

   State                mState;
   F32                  mStateTimeout;

   void setDeployedPos( const Point3F& pos, const Point3F& normal );

   void prepRenderImage( SceneRenderState* state ) override;
   void renderObject( ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat );

public:
   DECLARE_CONOBJECT( ProximityMine );

   ProximityMine();
   ~ProximityMine();

   static void consoleInit();

   bool onAdd() override;
   void onRemove() override;
   bool onNewDataBlock( GameBaseData* dptr, bool reload ) override;

   void setTransform( const MatrixF& mat ) override;
   void processTick( const Move* move ) override;
   void explode();

   void advanceTime( F32 dt ) override;

   U32  packUpdate  ( NetConnection* conn, U32 mask, BitStream* stream ) override;
   void unpackUpdate( NetConnection* conn, BitStream* stream ) override;
};

#endif // _PROXIMITYMINE_H_
