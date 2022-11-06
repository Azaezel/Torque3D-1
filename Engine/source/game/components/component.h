#pragma once

#ifndef _SIMDATABLOCK_H_
#include "console/simDatablock.h"
#endif

#include "componentMacroHelpers.h"
#include "game/components/componentObject.h"

class ComponentObject;
class ComponentInstance;

/// <summary>
/// ComponentField acts as a component-specific alternative or supplement to the normal fields.
/// Specifically, these are fields that are FOR components, and can act as a targeted list for tooling
/// or the template-instance copy over to process as opposed to the entire mass of fields on any given object
/// </summary>
struct ComponentField
{
   /// <summary>
   /// Name of the field
   /// </summary>
   StringTableEntry mFieldName;
   /// <summary>
   /// A text description of the field. Most useful in the edtior for toolips
   /// </summary>
   StringTableEntry mFieldDescription;

   /// <summary>
   /// The name of the field type. This is most useful for behaviors, where we don't directly interface
   /// with the engine-side Types, but it's used to look up the fieldType id for field generation in the editor
   /// </summary>
   StringTableEntry mFieldTypeName;
   /// <summary>
   /// The specific type id for the given field. Used by the editor to generate the correct type of editor field.
   /// </summary>
   S32 mFieldType;

   /// <summary>
   /// This is special-case data the field can utilize. Things like specialized lists or the like can be packed in here
   /// and processed by the tools frontend for editing purposes
   /// </summary>
   StringTableEntry mUserData;

   /// <summary>
   /// The default value of the field. Can be left blank. The base-level data for a field, that is overriden by values on the component
   /// template and instance
   /// </summary>
   StringTableEntry mDefaultValue;

   /// <summary>
   /// Is this field hidden in the editor?
   /// </summary>
   bool mHidden;
};

/// <summary>
/// Component is a datablock-based class of object. It's used as a template for associated ComponentInstances
/// </summary>
class Component: public SimDataBlock {
   typedef SimDataBlock Parent;

protected:
   /// <summary>
   /// This is the human-friendly name. Mainly just used for editor interface stuff
   /// </summary>
   StringTableEntry mFriendlyName;
   /// <summary>
   /// Description of the component
   /// </summary>
   StringTableEntry mDescription;

   /// <summary>
   /// The category group this component fits into. For organizational purposes in the editor, ie "Render" or "Physics"
   /// </summary>
   StringTableEntry mComponentType;

   /// <summary>
   /// Indicates if this component should be networked down to the client or not
   /// </summary>
   bool mNetworked;

   /// <summary>
   /// List of ComponentFields defined for this component
   /// These are used to set up and apply to the instances created from this Component
   /// </summary>
   Vector<ComponentField> mFields;

public:
   Component();
   
   DECLARE_CONOBJECT(Component);

   //Standard boilerplate functions for setup and field init'ing
   bool onAdd();
   static void initPersistFields();

   //We lean into this to register associated directors at startup time so they're always ready and registered to the DirectorManager
   static void consoleInit();

   /// <summary>
   /// This sets up some common, boilerplate signal calls that hook into the Director notifications
   /// Namely, when a component is added or removed, the signals will inform the associated director
   /// And the event can be handled
   /// </summary>
   COMP_REGISTER_SIGNALS(Component);

   /// <summary>
   /// Creates a ComponentInstance based on this template Component.
   /// Also invokes setupFields to ensure that the componentInstance is fully templated from this component
   /// Once created, the ComponentInstance is added to it's own static master list for self-management and retention
   /// </summary>
   /// <param name="owner">Owner ComponentObject to associate to the ComponentInstance</param>
   /// <returns>The created ComponentInstance</returns>
   virtual ComponentInstance* createInstance(ComponentObject* owner);

#pragma region Field Management
   /// <summary>
   /// This function sets up the component fields on a componentInstance
   /// When called, it will iterate over the componentFields and duplicate
   /// them onto the target componentInstance, ensuring that the instance
   /// has been fully templated from this template component
   /// </summary>
   /// <param name="componentInstance">The target ComponentInstance to be setup</param>
   /// <param name="forceSetup">If on, it will forcefully set up the field, even if the componentInstance already has data for it</param>
   /// <returns>A boolean indicating if the setup was successful</returns>
   bool setupFields(ComponentInstance* componentInstance, bool forceSetup = false);

   /// <summary>
   /// Adds a named field to a Component that can specify a description, data type, default value and userData
   /// </summary>
   /// <param name="fieldName">The name of the Field</param>
   /// <param name="desc">The Description of the Field</param>
   /// <param name="type">The Type of field that this is, example 'String' or 'Bool'</param>
   /// <param name="defaultValue">The Default value of this field</param>
   /// <param name="userData">An extra optional field that can be used for user data, such as lists</param>
   void addComponentField(const char* fieldName, const char* desc, const char* type, const char* defaultValue = NULL, const char* userData = NULL, bool hidden = false);

   /// <summary>
   /// Returns the number of ComponentFields on this template
   /// </summary>
   /// <returns>The number of component fields</returns>
   inline S32 getComponentFieldCount() { return mFields.size(); };

   /// <summary>
   /// Gets a ComponentField by its index in the mFields vector 
   /// </summary>
   /// <param name="idx">The index of the field in the mField vector</param>
   /// <returns>Pointer to the ComponentField found at the index</returns>
   inline ComponentField* getComponentField(S32 idx)
   {
      if (idx < 0 || idx >= mFields.size())
         return NULL;

      return &mFields[idx];
   }

   /// <summary>
   /// Gets a ComponentField by its name in the mFields vector 
   /// </summary>
   /// <param name="fieldName">The name of the field in the mField vector we're looking for</param>
   /// <returns>Pointer to the ComponentField found at the index</returns>
   ComponentField* getComponentField(const char* fieldName);
#pragma endregion

#pragma region Networking
   virtual void packData(BitStream* stream);
   virtual void unpackData(BitStream* stream);

   /// <summary>
   /// Returns if the component is marked to be networked or not.
   /// Specifically, this is used to indicate to owner ComponentObjects to handle the networking process
   /// for the component and its derived instances
   /// </summary>
   /// <returns>A boolean via the var mNetworked</returns>
   bool isNetworked() const { return mNetworked; }
#pragma endregion

   /// <summary>
   /// Gets the type of the component. Used as a group or categorization of the type, ie "Render" or "Physics"
   /// See @mComponentType
   /// </summary>
   /// <returns>The string value of mComponentType</returns>
   const char* getComponentType() const { return mComponentType; }

   /// <summary>
   /// A utility function that processes a string to be formatted in a way that can be easily packed into the mDescription field for the component
   /// </summary>
   /// <param name="desc">The string description to be processed</param>
   /// <returns>The final, processed string description</returns>
   const char* getDescriptionText(const char* desc);
};

