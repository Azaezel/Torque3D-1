#pragma once

#include "component.h"
#ifndef _NETCONNECTION_H_
#include "sim/netConnection.h"
#endif

class Component;
class ComponentObject;
struct ComponentField;

/// <summary>
/// A ComponentInstance is a thin, instanced object based off a Component as a template
/// These objects contain the "live" data that is implemented based on what the component does
/// Ie, a render componentInstance, when invoked, renders whatever data it has
/// While a child of SimObject, most components do not register to the console system via registerObject
/// This gives us all the logic for doing stuff with field dictionaries and other torque bits,
/// but doesn't assign a SimObjectId alleviating the burden of going through it in the object map in the console,
/// reducing burden/overhead. It also means the console doesn't directly interact with the instance, keeping it threadsafe
/// </summary>
class ComponentInstance : public SimObject
{
   typedef SimObject Parent;

   friend Component;

protected:
   /// <summary>
   /// A static list of all ComponentInstances. Each class derived from ComponentInstance has it's own that overrides the sComponentInstanceList name
   /// This allows us to, when doing work in a Director, have only the lists of components it needs to work with, reducing the memory footprint
   /// and keeping things more cache friendly. In theory, all the objects a Director needs to work can be packed into the cache, yielding large performance
   /// gains
   /// This list also is then thread friendly because we can index out chunks from it to work on sets of components and not risk threads erroneously
   /// tapping the same component in a different place.
   /// </summary>
   static Vector<ComponentInstance*> sComponentInstanceList;

   /// <summary>
   /// Mask bits for doing network updates.
   /// </summary>
   U32  mDirtyMaskBits;
   /// <summary>
   /// Indicates if this component is a server or a client object
   /// </summary>
   bool mIsServerObject;

   /// <summary>
   /// Is this component currently enabled?
   /// </summary>
   bool	mEnabled;

   /// <summary>
   /// A const handle back to the original template Component
   /// </summary>
   const Component* mComponentData;
   /// <summary>
   /// A const handle to the owner ComponentObject this Instance was created for
   /// </summary>
   const ComponentObject* mOwner;

   /// <summary>
   /// A list of this ComponentInstance's componentFields. These are setup by the template Component,
   /// but after that the data is unique to the Instance, and can be overridden as needed.
   /// </summary>
   Vector<ComponentField> mComponentFields;

public:
   DECLARE_CONOBJECT(ComponentInstance);

   ComponentInstance() { mComponentData = nullptr; mOwner = nullptr; }
   ComponentInstance(const Component& componentData, const ComponentObject& ownerEntity);
   ~ComponentInstance();

   static void initPersistFields();

   /// <summary>
   /// Destroys this ComponentInstance, removing it from the static list
   /// </summary>
   virtual void destroyInstance();

   /// <summary>
   /// The primary update function. When called, this makes the componentInstance do its work
   /// Ie, a render component renders something, a physics component updates the physics sim, etc
   /// </summary>
   virtual void update();

   /// <summary>
   /// Gets the const reference to the template Component.
   /// </summary>
   /// <returns>A constant reference to the template Component</returns>
   const Component& getComponentData() { return *mComponentData; }

   /// <summary>
   /// Gets a pointer to the template Component. This is not constant.
   /// </summary>
   /// <returns>A pointer to the template Component</returns>
   Component* getComponentDataPtr() { return const_cast<Component*>(mComponentData); }

   /// <summary>
   /// Gets a const reference to the owner ComponentObject
   /// </summary>
   /// <returns>A constant reference to the owner ComponentObject</returns>
   const ComponentObject& getOwnerObject() { return *mOwner; }

   /// <summary>
   /// Gets a const reference to the owner ComponentObject
   /// </summary>
   /// <returns>A constant reference to the owner ComponentObject</returns>
   ComponentObject* getOwnerObjectPtr() { return const_cast<ComponentObject*>(mOwner); }

   /// <summary>
   /// Is this componentInstance currently enabled?
   /// </summary>
   /// <returns>Boolean of is enabled</returns>
   bool	isEnabled() const { return mEnabled; }

   /// <summary>
   /// Sets the enabled status for the ComponentInstance, and flags the EnableMask to ensure a networked component is
   /// marked to be updated
   /// </summary>
   /// <param name="toggle">The enable status we're updating the instance to be</param>
   void setEnabled(bool toggle) { mEnabled = toggle; setMaskBits(EnableMask); }

   /// @name Networking
   /// @{

   /// <summary>
   /// The enum of mask bits used to granularize the network updates so we only traffic network data for things that
   /// have actually changed
   /// </summary>
   enum NetMaskBits
   {
      InitialUpdateMask = BIT(0),
      OwnerMask = BIT(1),
      UpdateMask = BIT(2),
      EnableMask = BIT(3),
      NamespaceMask = BIT(4),
      NextFreeMask = BIT(5)
   };

   /// <summary>
   /// Marks the componentInstance to be network updated, and marks the maskbits dirty
   /// Then sets the owner ComponentObject's ghost as dirty, ensuring that the network update
   /// will activate.
   /// </summary>
   /// <param name="orMask"></param>
   virtual void setMaskBits(U32 orMask);

   /// <summary>
   /// Clears any in-effect dirty mask bits
   /// </summary>
   virtual void clearMaskBits() {
      mDirtyMaskBits = 0;
   }

   /// <summary>
   /// Is this componentInstance a server object?
   /// </summary>
   /// <returns>True if is an object on the server</returns>
   bool isServerObject() const { return mIsServerObject; }
   /// <summary>
   /// Is this componentInstance a client object?
   /// </summary>
   /// <returns>True if is an object on the client</returns>
   bool isClientObject() const { return !mIsServerObject; }

   /// <summary>
   /// Sets if this componentInstance is a server object. If set as false, will indicate the instance is a client object
   /// </summary>
   /// <param name="isServerObj">Is this instance a server object</param>
   void setIsServerObject(bool isServerObj) { mIsServerObject = isServerObj; }

   /// <summary>
   /// Packs the network update with data from this component to update down to clients.
   /// Important to note, this is not invoked directly by the networking layer, as componentInstances do not generate
   /// network ghosts.
   /// Instead, this is invoked by the owner ComponentObject, and any network update data is packed into the owner's
   /// network update packets. This keeps the ghost count down allowing lean network footprints
   /// But each componentInstance has the dirty mask bit system so only the parts that need updating are trafficed
   /// This keeps the overall network burden as lean as possible without compromising functionality
   /// </summary>
   /// <param name="con">NetConnection</param>
   /// <param name="mask">Pass-through NetMaskBit</param>
   /// <param name="stream">BitStream that update data is written into</param>
   /// <returns>The pass-through NetMaskBit</returns>
   virtual U32 packUpdate(NetConnection* con, U32 mask, BitStream* stream);
   /// <summary>
   /// Unpacks the network update with data from this component from the server onto this client.
   /// Important to note, this is not invoked directly by the networking layer, as componentInstances do not generate
   /// network ghosts.
   /// Instead, this is invoked by the owner ComponentObject, and any network update data is unpacked from the owner's
   /// network update packets. This keeps the ghost count down allowing lean network footprints
   /// But each componentInstance has the dirty mask bit system so only the parts that need updating are trafficed
   /// This keeps the overall network burden as lean as possible without compromising functionality
   /// </summary>
   /// <param name="con">NetConnection</param>
   /// <param name="stream">Bitstream that the update data is read from</param>
   virtual void unpackUpdate(NetConnection* con, BitStream* stream);

   /// @}

   /// @name Component Fields
   /// @{

   /// <summary>
   /// Adds a ComponentField to the componentInstance.
   /// </summary>
   /// <param name="fieldName">Name of the componentField</param>
   /// <param name="desc">Text description of the component</param>
   /// <param name="type">The type of field</param>
   /// <param name="defaultValue">The default value for this field</param>
   /// <param name="userData">Special-purpose user data. This would be a list of options or other similar special data</param>
   /// <param name="hidden">Is this field hidden?</param>
   void addComponentField(const char* fieldName, const char* desc, const char* type, const char* defaultValue = NULL, const char* userData = NULL, bool hidden = false);
   /// <summary>
   /// Adds a ComponentField to the componentInstance
   /// </summary>
   /// <param name="newField">The new ComponentField to be added to the instance</param>
   void addComponentField(ComponentField newField);

   /// <summary>
   /// Builds a string of the ComponentField names and values for writing out when saving an owner ComponentObject
   /// </summary>
   /// <returns>A string of the packed ComponentField data for this instance</returns>
   StringTableEntry writeComponentFields();

   /// <summary>
   /// Gets the number of ComponentFields on this instance
   /// </summary>
   /// <returns>Number of ComponentFields</returns>
   inline S32 getComponentFieldCount() { return mComponentFields.size(); };

   /// <summary>
   /// Gets a ComponentField by its index in the mFields vector 
   /// </summary>
   /// <param name="idx">The index of the field in the mField vector</param>
   /// <returns></returns>
   inline ComponentField* getComponentField(S32 idx)
   {
      if (idx < 0 || idx >= mComponentFields.size())
         return NULL;

      return &mComponentFields[idx];
   }

   /// <summary>
   /// This function is called via the setDataField function. Called when a valid static field is modified on this object.
   /// We call this to catch any case where a field is modified on this componentInstance so we can check if there's an associated
   /// ComponentField so we can handle the field being updated this way
   /// </summary>
   /// <param name="fieldName">Name of the static field that was modified</param>
   /// <param name="newValue">The new value that was set for the field</param>
   virtual void onStaticModified(const char* fieldName, const char* newValue);
   /// <summary>
   /// This function is called via the setDataField function. Called when a valid dynamic field is modified on this object.
   /// We call this to catch any case where a field is modified on this componentInstance so we can check if there's an associated
   /// ComponentField so we can handle the field being updated this way
   /// </summary>
   /// <param name="fieldName">Name of the dynamic field that was modified</param>
   /// <param name="newValue">The new value that was set for the field</param>
   virtual void onDynamicModified(const char* fieldName, const char* newValue = NULL);

   /// <summary>
   /// This is what we actually use to check if the modified field is one of our behavior fields. If it is, we update and make the correct callbacks
   /// </summary>
   /// <param name="fieldName">Name of the dynamic field that was modified</param>
   /// <param name="newValue">The new value that was set for the field</param>
   void checkComponentFieldModified(const char* fieldName, const char* newValue);

   /// @}

   /// <summary>
   /// A function to get the static list of ComponentInstances of this specific class
   /// </summary>
   /// <returns>Returns the ComponentInstance list</returns>
   static Vector<ComponentInstance*>* getComponentInstList() { return &sComponentInstanceList; };
};
