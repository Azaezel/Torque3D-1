#pragma once

#ifndef _SIMDATABLOCK_H_
#include "console/simDatablock.h"
#endif
#ifndef _MBOX_H_
#include "math/mBox.h"
#endif
#ifndef _EARLYOUTPOLYLIST_H_
#include "collision/earlyOutPolyList.h"
#endif
#ifndef _MPOLYHEDRON_H_
#include "math/mPolyhedron.h"
#endif

#include "game/Entity.h"

class Entity;
class ComponentInstance;

class Component: public SimDataBlock {
   typedef SimDataBlock Parent;

  public:

   Component();
   
   DECLARE_CONOBJECT(Component);

   bool onAdd();
   static void initPersistFields();
   static void consoleInit();
   virtual void packData  (BitStream* stream);
   virtual void unpackData(BitStream* stream);

   virtual bool addComponent(Entity* ent);
   virtual bool removeComponent(Entity* ent);

   //
   //
   virtual ComponentInstance createInstance(Entity* owner) const;

   typedef Signal <void(Entity* ent, const Component& comp)> AddComponentSignal;
   static AddComponentSignal smAddedComponentSignal;
   virtual AddComponentSignal& getAddedSignal() { return smAddedComponentSignal; }

   typedef Signal <void(Entity* ent, const Component& comp)> RemoveComponentSignal;
   static RemoveComponentSignal smRemovedComponentSignal;
   virtual RemoveComponentSignal& getRemovedSignal() { return smRemovedComponentSignal; }
};

class ComponentInstance
{
   friend Component;

private:
   static Vector<ComponentInstance> sComponentInstanceList;

protected:
   const Component* mComponentData;
   const Entity* mOwner;
   
public:
   ComponentInstance() { mComponentData = nullptr; mOwner = nullptr; }
   ComponentInstance(const Component& componentData, const Entity& ownerEntity);
   ~ComponentInstance();

   virtual void destroyInstance();

   virtual void update();

   const Component& getComponentData() { return *mComponentData; }
   const Entity& getOwnerEntity() { return *mOwner; }
};
