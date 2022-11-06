#include "componentInstance.h"
#include "console/engineAPI.h"
#include "T3D/assets/ImageAsset.h"
#include "T3D/assets/ShapeAsset.h"
#include "T3D/assets/MaterialAsset.h"

Vector<ComponentInstance*> ComponentInstance::sComponentInstanceList;

IMPLEMENT_CONOBJECT(ComponentInstance);

ComponentInstance::ComponentInstance(const Component& componentData, const ComponentObject& owner) :
   mIsServerObject(true),
   mDirtyMaskBits(0),
   mEnabled(true)
{
   mComponentData = &componentData;
   mOwner = &owner;
}

ComponentInstance::~ComponentInstance()
{
}

void ComponentInstance::destroyInstance()
{
   ComponentInstance::sComponentInstanceList.remove(this);
   delete this;
}

void ComponentInstance::initPersistFields()
{
   Parent::initPersistFields();
}

void ComponentInstance::update()
{
}

void ComponentInstance::setMaskBits(U32 orMask)
{
   AssertFatal(orMask != 0, "Invalid net mask bits set.");

   if (mOwner)
   {
      //We have a valid owner, so tell it that it needs to be marked dirty for network updates for components
      (const_cast<ComponentObject*>(mOwner))->setComponentNetMask(this, orMask);
   }
}

U32 ComponentInstance::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
   U32 retMask = 0;

   /*if (mask & OwnerMask)
   {
      if (mOwner != NULL)
      {
         S32 ghostIndex = con->getGhostIndex(mOwner);

         if (ghostIndex == -1)
         {
            stream->writeFlag(false);
            retMask |= OwnerMask;
         }
         else
         {
            stream->writeFlag(true);
            stream->writeFlag(true);
            stream->writeInt(ghostIndex, NetConnection::GhostIdBitSize);
         }
      }
      else
      {
         stream->writeFlag(true);
         stream->writeFlag(false);
      }
   }
   else
      stream->writeFlag(false);*/

   if (stream->writeFlag(mask & EnableMask))
   {
      stream->writeFlag(mEnabled);
   }

   /*if (stream->writeFlag(mask & NamespaceMask))
   {
      const char* name = getName();
      if (stream->writeFlag(name && name[0]))
         stream->writeString(String(name));

      if (stream->writeFlag(mSuperClassName && mSuperClassName[0]))
         stream->writeString(String(mSuperClassName));

      if (stream->writeFlag(mClassName && mClassName[0]))
         stream->writeString(String(mClassName));
   }*/

   return retMask;
}

void ComponentInstance::unpackUpdate(NetConnection* con, BitStream* stream)
{
   /*if (stream->readFlag())
   {
      if (stream->readFlag())
      {
         //we have an owner object, so fetch it
         S32 gIndex = stream->readInt(NetConnection::GhostIdBitSize);

         Entity *e = dynamic_cast<Entity*>(con->resolveGhost(gIndex));
         if (e)
            e->addComponent(this);
      }
      else
      {
         //it's being nulled out
         setOwner(NULL);
      }
   }*/

   if (stream->readFlag())
   {
      mEnabled = stream->readFlag();
   }

   /*if (stream->readFlag())
   {
      if (stream->readFlag())
      {
         char name[256];
         stream->readString(name);
         assignName(name);
      }

      if (stream->readFlag())
      {
         char superClassname[256];
         stream->readString(superClassname);
       mSuperClassName = superClassname;
      }

      if (stream->readFlag())
      {
         char classname[256];
         stream->readString(classname);
         mClassName = classname;
      }

      linkNamespaces();
   }*/
}

void ComponentInstance::addComponentField(const char* fieldName, const char* desc, const char* type, const char* defaultValue /* = NULL */, const char* userData /* = NULL */, /*const char* dependency /* = NULL *//*,*/ bool hidden /* = false */)
{
   StringTableEntry stFieldName = StringTable->insert(fieldName);

   for (S32 i = 0; i < mComponentFields.size(); ++i)
   {
      if (mComponentFields[i].mFieldName == stFieldName)
         return;
   }

   //find the field type
   S32 fieldTypeMask = -1;
   StringTableEntry fieldType = StringTable->insert(type);

   if (fieldType == StringTable->insert("int"))
      fieldTypeMask = TypeS32;
   else if (fieldType == StringTable->insert("float"))
      fieldTypeMask = TypeF32;
   else if (fieldType == StringTable->insert("vector"))
      fieldTypeMask = TypePoint3F;
   else if (fieldType == StringTable->insert("material") || fieldType == StringTable->insert("TypeMaterialAssetId"))
      fieldTypeMask = TypeMaterialAssetId;
   else if (fieldType == StringTable->insert("image") || fieldType == StringTable->insert("TypeImageAssetId"))
      fieldTypeMask = TypeImageAssetId;
   else if (fieldType == StringTable->insert("shape") || fieldType == StringTable->insert("TypeShapeAssetId"))
      fieldTypeMask = TypeShapeAssetId;
   else if (fieldType == StringTable->insert("bool"))
      fieldTypeMask = TypeBool;
   else if (fieldType == StringTable->insert("object"))
      fieldTypeMask = TypeSimObjectPtr;
   else if (fieldType == StringTable->insert("string"))
      fieldTypeMask = TypeString;
   else if (fieldType == StringTable->insert("colorI"))
      fieldTypeMask = TypeColorI;
   else if (fieldType == StringTable->insert("colorF"))
      fieldTypeMask = TypeColorF;
   else if (fieldType == StringTable->insert("ease"))
      fieldTypeMask = TypeEaseF;
   else
      fieldTypeMask = -1;

   //Setup the actual field
   ComponentField field;
   field.mFieldName = stFieldName;

   field.mFieldTypeName = fieldType;
   field.mFieldType = fieldTypeMask;

   field.mUserData = StringTable->insert(userData ? userData : "");
   field.mDefaultValue = StringTable->insert(defaultValue ? defaultValue : "");

   field.mHidden = hidden;

   //Save it off
   mComponentFields.push_back(field);
}

void ComponentInstance::addComponentField(ComponentField newField)
{
   //Check if we have an existing field with the same name
   for (U32 i = 0; i < mComponentFields.size(); i++)
   {
      //if we have a match on an existing component field, we don't need to add another one.
      if (newField.mFieldName == mComponentFields[i].mFieldName)
         return;
   }

   //Save it off
   mComponentFields.push_back(newField);
}

StringTableEntry ComponentInstance::writeComponentFields()
{
   String output;

   for (U32 i = 0; i < mComponentFields.size(); i++)
   {
      //we don't need to write the templateName field
      if (mComponentFields[i].mFieldName == StringTable->insert("templateName"))
         continue;

      StringTableEntry fieldData = StringTable->insert(getDataField(mComponentFields[i].mFieldName, NULL));

      //If the data we have for that field is blank, use the default value
      if (fieldData == StringTable->EmptyString())
         fieldData = mComponentFields[i].mDefaultValue;

      //if we have literally no data at all on this field, skip writing it
      if (fieldData == StringTable->EmptyString())
         continue;

      //If the data we have is identical to the component template data, no point in writing it out
      StringTableEntry templateData = StringTable->insert(getComponentDataPtr()->getDataField(mComponentFields[i].mFieldName, NULL));
      if (templateData == fieldData)
         continue;

      //Append the string
      output += mComponentFields[i].mFieldName + String("\t") + fieldData;

      if (i + 1 < mComponentFields.size())
         output += "\t";
   }

   return StringTable->insert(output.c_str());
}

void ComponentInstance::onStaticModified(const char* slotName, const char* newValue)
{
   Parent::onStaticModified(slotName, newValue);

   //If we don't have an owner yet, then this is probably the initial setup, so we don't need the console callbacks yet.
   if (!mOwner)
      return;

   checkComponentFieldModified(slotName, newValue);
}

void ComponentInstance::onDynamicModified(const char* slotName, const char* newValue)
{
   Parent::onDynamicModified(slotName, newValue);

   //If we don't have an owner yet, then this is probably the initial setup, so we don't need the console callbacks yet.
   if (!mOwner)
      return;

   checkComponentFieldModified(slotName, newValue);
}

void ComponentInstance::checkComponentFieldModified(const char* slotName, const char* newValue)
{
   StringTableEntry slotNameEntry = StringTable->insert(slotName);
   //find if it's a behavior field
   for (int i = 0; i < getComponentFieldCount(); i++)
   {
      ComponentField* field = getComponentField(i);
      if (field->mFieldName == slotNameEntry)
      {
         //ensure the update bumps through
         setMaskBits(-1); 

         //we have a match, do the script callback that we updated a field
         if (isMethod("onInspectorUpdate"))
            Con::executef(this, "onInspectorUpdate", slotName);

         /*BehaviorFieldInterface *bInterface = mOwner->getInterface<BehaviorFieldInterface)();

         BehaviorInterface *bInterface = dynamic_cast<BehaviorFieldInterface*>(mOwner->getInterface(NULL, "behaviorFieldUpdate", NULL));

         if(bInterface)
         {
         BehaviorFieldInterface *bInterface = dynamic_cast<BehaviorFieldInterface*>(bInterface)
         bInterface->onFieldChange(slotName, newValue);
         }*/

         //Lastly, notify up to our owner's parent(s). If one is a prefab, we inform it it's now dirty
         /*Prefab* p = Prefab::getPrefabByChild(mOwner);
         if (p)
            p->setDirty();
         return;*/
      }
   }
}
