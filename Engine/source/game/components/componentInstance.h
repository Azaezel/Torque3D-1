#pragma once

#include "component.h"
#ifndef _NETCONNECTION_H_
#include "sim/netConnection.h"
#endif
#ifndef _BITSTREAM_H_
#include "core/stream/bitStream.h"
#endif

class Component;
class ComponentObject;
struct ComponentField;

class ComponentInstance : public SimObject
{
   friend Component;

protected:
   static Vector<ComponentInstance> sComponentInstanceList;

   U32  mDirtyMaskBits;
   bool mIsServerObject;

   bool	mEnabled;

   const Component* mComponentData;
   const ComponentObject* mOwner;

   Vector<ComponentField> mComponentFields;
   StringTableEntry mComponentGroup;

public:
   DECLARE_CONOBJECT(ComponentInstance);

   ComponentInstance() { mComponentData = nullptr; mOwner = nullptr; }
   ComponentInstance(const Component& componentData, const ComponentObject& ownerEntity);
   ~ComponentInstance();

   virtual void packToStream(Stream& stream, U32 tabStop, S32 behaviorID, U32 flags = 0);

   virtual void destroyInstance();

   virtual void update();
   virtual void updateDelta(F32 dt);

   const Component& getComponentData() { return *mComponentData; }
   Component* getComponentDataPtr() { return const_cast<Component*>(mComponentData); }
   const ComponentObject& getOwnerObject() { return *mOwner; }

   //
   bool	isEnabled() const { return mEnabled; }
   void  setEnabled(bool toggle) { mEnabled = toggle; setMaskBits(EnableMask); }

   //Networking stuff
   enum NetMaskBits
   {
      InitialUpdateMask = BIT(0),
      OwnerMask = BIT(1),
      UpdateMask = BIT(2),
      EnableMask = BIT(3),
      NamespaceMask = BIT(4),
      NextFreeMask = BIT(5)
   };

   virtual void setMaskBits(U32 orMask);
   virtual void clearMaskBits() {
      mDirtyMaskBits = 0;
   }

   bool isServerObject() const { return mIsServerObject; }
   bool isClientObject() const { return !mIsServerObject; }

   void setIsServerObject(bool isServerObj) { mIsServerObject = isServerObj; }

   virtual U32 packUpdate(NetConnection* con, U32 mask, BitStream* stream);
   virtual void unpackUpdate(NetConnection* con, BitStream* stream);

   //Component Fields
   void addComponentField(const char* fieldName, const char* value);
   void removeBehaviorField(const char* fieldName);

   void addComponentField(const char* fieldName, const char* desc, const char* type, const char* defaultValue = NULL, const char* userData = NULL, bool hidden = false);
   void addComponentField(ComponentField newField);

   inline S32 getComponentFieldCount() { return mComponentFields.size(); };

   /// Gets a ComponentField by its index in the mFields vector 
   /// @param idx  The index of the field in the mField vector
   inline ComponentField* getComponentField(S32 idx)
   {
      if (idx < 0 || idx >= mComponentFields.size())
         return NULL;

      return &mComponentFields[idx];
   }

   ComponentField* getComponentField(const char* fieldName);

   //
   static Vector<ComponentInstance>* getComponentInstList() { return &sComponentInstanceList; };
};
