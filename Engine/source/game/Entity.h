#pragma once

#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif

#include "components/component.h"

class Component;
class ComponentInstance;

class Entity : public SceneObject
{
   typedef SceneObject Parent;

   // Networking masks
   // We need to implement at least one of these to allow
   // the client version of the object to receive updates
   // from the server version (like if it has been moved
   // or edited)
   enum MaskBits 
   {
      TransformMask = Parent::NextFreeMask << 0,
      NextFreeMask  = Parent::NextFreeMask << 1
   };

protected:
   Vector<ComponentInstance> mComponents;

public:
   Entity();
   virtual ~Entity();

   // Declare this object as a ConsoleObject so that we can
   // instantiate it into the world and network it
   DECLARE_CONOBJECT(Entity);

   //--------------------------------------------------------------------------
   // Object Editing
   // Since there is always a server and a client object in Torque and we
   // actually edit the server object we need to implement some basic
   // networking functions
   //--------------------------------------------------------------------------
   // Set up any fields that we want to be editable (like position)
   static void initPersistFields();

   // Handle when we are added to the scene and removed from the scene
   bool onAdd();
   void onRemove();

   // Override this so that we can dirty the network flag when it is called
   void setTransform( const MatrixF &mat );

   // This function handles sending the relevant data from the server
   // object to the client object
   U32 packUpdate( NetConnection *conn, U32 mask, BitStream *stream );
   // This function handles receiving relevant data from the server
   // object and applying it to the client object
   void unpackUpdate( NetConnection *conn, BitStream *stream );

   //
   // Editing
   //
   void onInspect(GuiInspector* inspector);
   void onEndInspect();

   //
   // Components
   //
   ComponentInstance* getComponentInstance(const U32& index) {
      if (index >= mComponents.size())
         return nullptr;

      return &mComponents[index];
   }

   U32 getComponentCount() const
   {
      return mComponents.size();
   }

   bool addComponent(const Component& component);
};
