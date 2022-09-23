#pragma once

#include "game/components/component.h"
#include "game/directors/directorManager.h"

class RenderMeshDirector;

class RenderMeshComponent : public Component
{
   typedef Component Parent;

public:
   RenderMeshComponent();

   DECLARE_CONOBJECT(RenderMeshComponent);

   bool onAdd();
   static void initPersistFields();
   static void consoleInit();
   virtual void packData(BitStream* stream);
   virtual void unpackData(BitStream* stream);


   //
   //
   virtual ComponentInstance createInstance(Entity* owner) const;
};

//
class RenderMeshComponentInstance : public ComponentInstance
{
   friend RenderMeshComponent;
   friend RenderMeshDirector;

private:
   static Vector<RenderMeshComponentInstance> sComponentInstanceList;

public:
   RenderMeshComponentInstance() { mComponentData = nullptr; mOwner = nullptr; }
   RenderMeshComponentInstance(const RenderMeshComponent& componentData, const Entity& ownerEntity);
   ~RenderMeshComponentInstance();

   virtual void destroyInstance();

   virtual void update();
};

//
class RenderMeshDirector : public Director
{
friend DirectorManager;
typedef Director Parent;

public:
   RenderMeshDirector();
   ~RenderMeshDirector();

   void update();
};
