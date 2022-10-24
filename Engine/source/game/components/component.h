#pragma once

#ifndef _SIMDATABLOCK_H_
#include "console/simDatablock.h"
#endif

#include "componentMacroHelpers.h"
#include "game/Entity.h"

class Entity;
class ComponentInstance;

struct ComponentField
{
   StringTableEntry mFieldName;
   StringTableEntry mFieldDescription;

   StringTableEntry mFieldType;
   StringTableEntry mUserData;

   StringTableEntry mDefaultValue;

   StringTableEntry mGroup;

   bool mHidden;
};


class Component: public SimDataBlock {
   typedef SimDataBlock Parent;

protected:
   StringTableEntry mFriendlyName;
   StringTableEntry mDescription;

   StringTableEntry mFromResource;
   StringTableEntry mComponentGroup;
   StringTableEntry mComponentType;
   StringTableEntry mNetworkType;
   StringTableEntry mTemplateName;

   Vector<StringTableEntry> mDependencies;
   Vector<ComponentField> mFields;

   bool mNetworked;

public:

   Component();
   
   DECLARE_CONOBJECT(Component);

   bool onAdd();
   static void initPersistFields();
   static void consoleInit();
   virtual void packData  (BitStream* stream);
   virtual void unpackData(BitStream* stream);

   virtual bool addComponent(ComponentObject* owner);
   virtual bool removeComponent(ComponentObject* owner);

   //
   //
   virtual ComponentInstance createInstance(ComponentObject* owner) const;

   bool setupFields(ComponentInstance* bi, bool forceSetup = false) const;

   COMP_REGISTER_SIGNALS();

   //
   /// @name Adding Named Fields
   /// @{

   /// Adds a named field to a Component that can specify a description, data type, default value and userData
   ///
   /// @param   fieldName    The name of the Field
   /// @param   desc         The Description of the Field
   /// @param   type         The Type of field that this is, example 'Text' or 'Bool'
   /// @param   defaultValue The Default value of this field
   /// @param   userData     An extra optional field that can be used for user data
   void addComponentField(const char* fieldName, const char* desc, const char* type, const char* defaultValue = NULL, const char* userData = NULL, bool hidden = false);

   /// Returns the number of ComponentField's on this template
   inline S32 getComponentFieldCount() { return mFields.size(); };

   /// Gets a ComponentField by its index in the mFields vector 
   /// @param idx  The index of the field in the mField vector
   inline ComponentField* getComponentField(S32 idx)
   {
      if (idx < 0 || idx >= mFields.size())
         return NULL;

      return &mFields[idx];
   }

   ComponentField* getComponentField(const char* fieldName);

   const char* getComponentType() const { return mComponentType; }

   const char* getDescriptionText(const char* desc);

   const char* getName() const { return mTemplateName; }

   bool isNetworked() const { return mNetworked; }

   void beginFieldGroup(const char* groupName);
   void endFieldGroup();

   void addDependency(StringTableEntry name);
   /// @}

   /// @name Description
   /// @{
   static bool setDescription(void* object, const char* index, const char* data);
   static const char* getDescription(void* obj, const char* data);
};

