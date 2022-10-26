#include "platform/platform.h"
#include "component.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stream/bitStream.h"
#include "math/mathIO.h"
#include "core/stream/fileStream.h"

//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(Component);

IMPL_COMP_REGISTER_SIGNALS(Component);

ConsoleDocClass( Component,
   "@brief \n"
   "@ingroup Datablocks\n"
);

Component::Component() :
   mFriendlyName(StringTable->EmptyString()),
   mDescription(StringTable->EmptyString()),
   mFromResource(StringTable->EmptyString()),
   mComponentGroup(StringTable->EmptyString()),
   mComponentType(StringTable->EmptyString()),
   mNetworkType(StringTable->EmptyString()),
   mTemplateName(StringTable->EmptyString()),
   mNetworked(false)
{
}

bool Component::onAdd()
{
   if (!Parent::onAdd())
      return false;

   addComponentField("templateName", "Name of the Component this ComponentInstance is templated from", "String", getName());

   return true;
}

void Component::consoleInit()
{
   Parent::consoleInit();

   //DirectorManager::get()->addDirector(ComponentDirector());
}

void Component::initPersistFields()
{
   Parent::initPersistFields();

   addField("friendlyName", TypeString, Offset(mFriendlyName, Component), "");
   addField("description", TypeString, Offset(mDescription, Component), "");
   addField("componentGroup", TypeString, Offset(mComponentGroup, Component), "");
   addField("componentType", TypeString, Offset(mComponentType, Component), "");
   
   addField("networkType", TypeString, Offset(mNetworkType, Component), "");
   addField("templateName", TypeString, Offset(mTemplateName, Component), "");

   addField("isNetworked", TypeBool, Offset(mNetworked, Component), "");
}

//--------------------------------------------------------------------------
void Component::packData(BitStream* stream)
{
   Parent::packData(stream);
}

void Component::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);
}

ComponentInstance* Component::createInstance(ComponentObject* owner)
{
   ComponentInstance* compInst = new ComponentInstance(*this, *owner);

   setupFields(compInst, true);
   ComponentInstance::getComponentInstList()->push_back(compInst);

   //It's important to note we never actually register the ComponentInstance created.
   //This gives us access to the normal SimObject boilerplate, like being able to keep tabs on fields for the object
   //but never directly integrates the object against the console.
   //This keeps it sufficiently functional, but detached, and thread-safe as we don't have to
   //worry about the console touching the Instance while something else is working it in a thread
   //It's safely held in our static list above, so we never have to worry about it going out of scope or cleaned up
   //when we don't want it to be
   return compInst;
}

bool Component::setupFields(ComponentInstance* bi, bool forceSetup) const
{
   for (S32 i = 0; i < mFields.size(); ++i)
   {
      ComponentField& field = const_cast<ComponentField&>(mFields[i]);

      bi->addComponentField(field);

      //check if this field already has data or not
      //if it's blank, we're good to continue setting it.
      //bi->getClassRep()->findField(
      const char* data = bi->getDataField(StringTable->insert(field.mFieldName), NULL);

      if (forceSetup || !dStrcmp(data, ""))
         bi->setDataField(field.mFieldName, NULL, field.mDefaultValue);
   }

   return true;
}

//////////////////////////////////////////////////////////////////////////

void Component::addComponentField(const char* fieldName, const char* desc, const char* type, const char* defaultValue /* = NULL */, const char* userData /* = NULL */, /*const char* dependency /* = NULL *//*,*/ bool hidden /* = false */)
{
   StringTableEntry stFieldName = StringTable->insert(fieldName);

   for (S32 i = 0; i < mFields.size(); ++i)
   {
      if (mFields[i].mFieldName == stFieldName)
         return;
   }

   ComponentField field;
   field.mFieldName = stFieldName;
   field.mFieldType = StringTable->insert(type ? type : "");
   field.mUserData = StringTable->insert(userData ? userData : "");
   field.mDefaultValue = StringTable->insert(defaultValue ? defaultValue : "");
   field.mFieldDescription = getDescriptionText(desc);

   field.mGroup = mComponentGroup;

   field.mHidden = hidden;

   mFields.push_back(field);
}

ComponentField* Component::getComponentField(const char* fieldName)
{
   StringTableEntry stFieldName = StringTable->insert(fieldName);

   for (S32 i = 0; i < mFields.size(); ++i)
   {
      if (mFields[i].mFieldName == stFieldName)
         return &mFields[i];
   }

   return NULL;
}
//////////////////////////////////////////////////////////////////////////

const char* Component::getDescriptionText(const char* desc)
{
   if (desc == NULL)
      return NULL;

   char* newDesc;

   // [tom, 1/12/2007] If it isn't a file, just do it the easy way
   if (!Platform::isFile(desc))
   {
      S32 descLen = dStrlen(desc) + 1;
      newDesc = new char[descLen];
      dStrcpy(newDesc, desc, descLen);

      return newDesc;
   }

   FileStream str;
   str.open(desc, Torque::FS::File::Read);

   Stream* stream = &str;
   if (stream == NULL) {
      str.close();
      return NULL;
   }

   U32 size = stream->getStreamSize();
   if (size > 0)
   {
      newDesc = new char[size + 1];
      if (stream->read(size, (void*)newDesc))
         newDesc[size] = 0;
      else
      {
         SAFE_DELETE_ARRAY(newDesc);
      }
   }

   str.close();
   delete stream;
   //ResourceManager->closeStream(stream);

   return newDesc;
}

void Component::beginFieldGroup(const char* groupName)
{
   if (dStrcmp(mComponentGroup, ""))
   {
      Con::errorf("Component: attempting to begin new field group with a group already begun!");
      return;
   }

   mComponentGroup = StringTable->insert(groupName);
}

void Component::endFieldGroup()
{
   mComponentGroup = StringTable->insert("");
}

void Component::addDependency(StringTableEntry name)
{
   mDependencies.push_back_unique(name);
}
