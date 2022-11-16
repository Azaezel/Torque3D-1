#pragma once

#include "game/components/component.h"
#include "game/components/componentInstance.h"

#include "game/directors/directorManager.h"

class ControlObjectDirector;

class ControlObjectComponent : public Component
{
   typedef Component Parent;

private:
   void onShapeChange() {}

public:
   ControlObjectComponent();

   DECLARE_CONOBJECT(ControlObjectComponent);

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
   COMP_REGISTER_SIGNALS(ControlObjectComponent);

   /// <summary>
   /// Creates a ControlObjectComponentInstance based on this template ControlObjectComponent.
   /// Also invokes setupFields to ensure that the ControlObjectComponentInstance is fully templated from this ControlObjectComponent
   /// Once created, the ControlObjectComponent is added to it's own static master list for self-management and retention
   /// </summary>
   /// <param name="owner">Owner ComponentObject to associate to the ControlObjectComponentInstance</param>
   /// <returns>The created ComponentInstance</returns>
   virtual ComponentInstance* createInstance(ComponentObject* owner);
};

class Move;

//
class ControlObjectComponentInstance : public ComponentInstance
{
   typedef ComponentInstance Parent;

   friend ControlObjectComponent;
   friend ControlObjectDirector;

public:

private:
   static Vector<ControlObjectComponentInstance*> sComponentInstanceList;

   //Connection* mConnection;

public:
   DECLARE_CONOBJECT(ControlObjectComponentInstance);

   /// <summary>
   /// Obligatory default constructor
   /// </summary>
   ControlObjectComponentInstance() { mComponentData = nullptr; mOwner = nullptr; }
   /// <summary>
   /// The main constructor actually utilized by DOCs
   /// This will assign the template componentData and owner ComponentObject for this componentInstance
   /// </summary>
   /// <param name="componentData">Template Component</param>
   /// <param name="owner">Owner ComopnentObject</param>
   ControlObjectComponentInstance(const ControlObjectComponent& componentData, const ComponentObject& owner);
   ~ControlObjectComponentInstance();

   static void initPersistFields();

   /// <summary>
   /// Destroys this ComponentInstance, removing it from the static list
   /// </summary>
   virtual void destroyInstance();

   /// <summary>
   /// Called by ControlObjectDirector when it runs, this will render our shapeInstance(if we have one)
   /// </summary>
   /// <param name="transform">The transform to render at</param>
   virtual void update();

   /// <summary>
   /// See ComponentInstance::packUpdate();
   /// </summary>
   virtual U32 packUpdate(NetConnection* con, U32 mask, BitStream* stream);
   /// <summary>
   /// See ComponentInstance::unpackUpdate();
   /// </summary>
   virtual void unpackUpdate(NetConnection* con, BitStream* stream);
};

//
class ControlObjectDirector : public Director
{
   typedef Director Parent;
   friend DirectorManager;

   /// <summary>
   /// A struct containing an owner ComponentObject and its relevent components
   /// If this Director tracks if it's a valid ref or not, so if any of the required components
   /// are missing, invalid, or disabled, we can easily skip it and move on without needing to
   /// re-juggle lists or dependency tracking
   /// </summary>
   struct ControlObjectEntityRef
   {
      ComponentObject* owner;
      StrongRefPtr<ControlObjectComponentInstance> controlObj;

      bool isValid()
      {
         if (owner != nullptr && !controlObj.isNull())
            return true;

         return false;
      }
   };

private:
   /// <summary>
   /// A list of validated ControlObjectEntityRef entries.
   /// </summary>
   Vector<ControlObjectEntityRef> mValidEntriesList;

public:
   ControlObjectDirector();
   ~ControlObjectDirector();

   void registerComponent(ComponentObject* owner, const Component& comp);
   void unregisterComponent(ComponentObject* owner, const Component& comp);

   /// <summary>
   /// The main update function. Will iterate over valid ControlObjectEntityRef's and invoke them to update
   /// In our case, this means we take the mesh ref to a ControlObjectComponentInstance and have it render the shape
   /// </summary>
   virtual void update();
};
