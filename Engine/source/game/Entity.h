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
      BoundsMask = Parent::NextFreeMask << 0,
      ComponentsUpdateMask = Parent::NextFreeMask << 1,
      AddComponentsMask = Parent::NextFreeMask << 2,
      RemoveComponentsMask = Parent::NextFreeMask << 3,
      NamespaceMask = Parent::NextFreeMask << 4,
      NextFreeMask = Parent::NextFreeMask << 5
   };

protected:
   Vector<ComponentInstance> mComponents;

   //Bit of helper data to let us track and manage the adding, removal and updating of networked components
   struct NetworkedComponent
   {
      U32 componentIndex;

      enum UpdateState
      {
         None,
         Adding,
         Removing,
         Updating
      };

      UpdateState updateState;

      U32 updateMaskBits;
   };

   Vector<NetworkedComponent> mNetworkedComponents;

   U32                        mComponentNetMask;

   bool                       mStartComponentUpdate;

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
   
   //
   virtual void onPostAdd();

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

   void setComponentsDirty();
   void setComponentDirty(Component* comp, bool forceUpdate = false);

   void setComponentNetMask(Component* comp, U32 mask);

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

   template <class T>
   T* getComponentInstance() {

      for (U32 i = 0; i < mComponents.size(); i++)
      {
         T* compInst = dynamic_cast<T*>(mComponents[i]);
         if (compInst != nullptr)
         {
            return T;
         }
      }
      
      return nullptr;
   }

   U32 getComponentCount() const
   {
      return mComponents.size();
   }

   bool addComponent(Component* component);
   bool removeComponent(Component* component);
};
