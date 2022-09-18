#pragma once

#include "game/components/component.h"

class RenderMeshComponent : public Component
{
   typedef Component Parent;

public:

   RenderMeshComponent();

   DECLARE_CONOBJECT(RenderMeshComponent);

   bool onAdd();
   static void initPersistFields();
   virtual void packData(BitStream* stream);
   virtual void unpackData(BitStream* stream);
};

class RenderMeshComponentInstance : public ComponentInstance
{

};
