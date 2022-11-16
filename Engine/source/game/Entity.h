#pragma once

#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif
#ifndef _MOVEMANAGER_H_
#include "T3D/gameBase/moveManager.h"
#endif

#include "components/component.h"
#include "components/componentObject.h"

class ComponentObject;
class Component;
class ComponentInstance;

class Entity : public SceneObject, public ComponentObject
{
   typedef SceneObject Parent;
   typedef ComponentObject CompObjParent;

   // Networking masks
   // We need to implement at least one of these to allow
   // the client version of the object to receive updates
   // from the server version (like if it has been moved
   // or edited)
public:
   enum MaskBits
   {
      TransformMask = Parent::NextFreeMask << 0,
      BoundsMask = Parent::NextFreeMask << 1,
      ComponentsUpdateMask = Parent::NextFreeMask << 2,
      AddComponentsMask = Parent::NextFreeMask << 3,
      RemoveComponentsMask = Parent::NextFreeMask << 4,
      NoWarpMask = Parent::NextFreeMask << 5,
      NamespaceMask = Parent::NextFreeMask << 6,
      NextFreeMask = Parent::NextFreeMask << 7
   };

protected:
   Move mLastMove;

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

   virtual void onPostAdd();

#pragma region Component Handling
   /// <summary>
   /// Creates a ComponentInstance from the Component passed in, and adds it to this Object
   /// </summary>
   /// <param name="component">The Component to use as a template to create an instance of</param>
   /// <returns>A bool indicating if creation and addition of an instance was successful</returns>
   virtual bool addComponent(Component* component);

   /// <summary>
   /// Removes a ComponentInstance from the Object based on the Component passed in.
   /// Looks up the Instance that utilizes the Component as it's template, and removes it.
   /// </summary>
   /// <param name="component">The Component that was used as a template to create an instance</param>
   /// <returns>A bool indicating if removal and deletion of an instance was successful</returns>
   virtual bool removeComponent(Component* component);

   /// <summary>
   /// Iterates over the _component fields for this entity and creates and loads up the ComponentInstances
   /// Used mainly when loading an Entity from a file, exec'ing the object block
   /// </summary>
   void loadComponents();
#pragma endregion

#pragma region Heirarchy/Mounting
   virtual void mountObject(SceneObject *obj, S32 node, const MatrixF &xfm = MatrixF::Identity);
   void mountObject(SceneObject* objB, const MatrixF& txfm);
   void onMount(SceneObject *obj, S32 node);
   void onUnmount(SceneObject *obj, S32 node);

   virtual void addObject(SimObject* object);
   virtual void removeObject(SimObject* object);

   virtual SimObject* findObjectByInternalName(StringTableEntry internalName, bool searchChildren);
#pragma endregion

#pragma region Sim/Updates
   virtual void   processTick(const Move* move);
   virtual void   advanceTime(F32 dt);
   virtual void   interpolateTick(F32 delta);
#pragma endregion

#pragma region Networking
   // This function handles sending the relevant data from the server
   // object to the client object
   U32 packUpdate( NetConnection *conn, U32 mask, BitStream *stream );
   // This function handles receiving relevant data from the server
   // object and applying it to the client object
   void unpackUpdate( NetConnection *conn, BitStream *stream );

   /// <summary>
   /// Sets a specific componentInstance's netmask and ensures the NetworkedComponent listing is marked as to-be-updated
   /// </summary>
   /// <param name="compInst">The componentInstance to have its mask set</param>
   /// <param name="mask">The netmask bits to be set on the instance</param>
   virtual void setComponentNetMask(ComponentInstance* comp, const U32& mask) {
      setMaskBits(Entity::ComponentsUpdateMask);
      CompObjParent::setComponentNetMask(comp, mask);
   }

   Move& getLastMove() { return mLastMove; }
#pragma endregion

#pragma region Console/Fields
   Signal< void(SimObject*, String, String) > onDataSet;
   virtual void setDataField(StringTableEntry slotName, const char *array, const char *value);
   virtual void onStaticModified(const char* slotName, const char* newValue);
#pragma endregion

#pragma region IO
   virtual void write(Stream &stream, U32 tabStop, U32 flags);
#pragma endregion

   //
   // Editing
   //
   void onInspect(GuiInspector* inspector);
   void onEndInspect();
};
