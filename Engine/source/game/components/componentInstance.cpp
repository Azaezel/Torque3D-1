#include "componentInstance.h"

Vector<ComponentInstance> ComponentInstance::sComponentInstanceList;

IMPLEMENT_CONOBJECT(ComponentInstance);

ComponentInstance::ComponentInstance(const Component& componentData, const ComponentObject& owner)
{
   mComponentData = &componentData;
   mOwner = &owner;

   mIsServerObject = true;
   mDirtyMaskBits = 0;
}

ComponentInstance::~ComponentInstance()
{
   //ComponentInstance::sComponentInstanceList.clear();
}

void ComponentInstance::destroyInstance()
{
   //ComponentInstance::sComponentInstanceList.remove(*this);
}

void ComponentInstance::update()
{
}

void ComponentInstance::updateDelta(F32 dt)
{
}

void ComponentInstance::setMaskBits(U32 orMask)
{
   AssertFatal(orMask != 0, "Invalid net mask bits set.");

   if (mOwner)
   {
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

void ComponentInstance::addComponentField(const char* fieldName, const char* value)
{
   //if this field already exists, just update it.
   /*for(U32 i=0; i < mComponentFields.size(); i++)
   {
      if(!dStrcmp(mComponentFields[i].mFieldName, fieldName)){
         mComponentFields[i].mDefaultValue = StringTable->insert(value);
         setDataField( mComponentFields[i].mFieldName, NULL, mComponentFields[i].mDefaultValue );
         return;
      }
   }

   //Otherwise, set the field up, and store it
   behaviorFields field;
   field.mFieldName = StringTable->insert(fieldName);
   field.mDefaultValue = StringTable->insert(value);

   mComponentFields.push_back(field);

   setDataField( field.mFieldName, NULL, field.mDefaultValue );*/
}

void ComponentInstance::addComponentField(const char* fieldName, const char* desc, const char* type, const char* defaultValue /* = NULL */, const char* userData /* = NULL */, /*const char* dependency /* = NULL *//*,*/ bool hidden /* = false */)
{
   StringTableEntry stFieldName = StringTable->insert(fieldName);

   for (S32 i = 0; i < mComponentFields.size(); ++i)
   {
      if (mComponentFields[i].mFieldName == stFieldName)
         return;
   }

   ComponentField field;
   field.mFieldName = stFieldName;
   field.mFieldType = StringTable->insert(type ? type : "");
   field.mUserData = StringTable->insert(userData ? userData : "");
   field.mDefaultValue = StringTable->insert(defaultValue ? defaultValue : "");
   //field.mFieldDescription = getDescriptionText(desc);

   //field.mDependency = StringTable->insert(dependency ? dependency : "");

   field.mGroup = mComponentGroup;

   field.mHidden = hidden;

   mComponentFields.push_back(field);
}

void ComponentInstance::addComponentField(ComponentField newField)
{
   for (U32 i = 0; i < mComponentFields.size(); i++)
   {
      //if we have a match on an existing component field, we don't need to add another one.
      if (newField.mFieldName == mComponentFields[i].mFieldName)
         return;
   }

   mComponentFields.push_back(newField);
}

void ComponentInstance::removeBehaviorField(const char* fieldName)
{
   for (U32 i = 0; i < mComponentFields.size(); i++)
   {
      if (!dStrcmp(mComponentFields[i].mFieldName, fieldName)) {
         mComponentFields.erase(i);
         return;
      }
   }

   setDataField(fieldName, NULL, "");
}

void ComponentInstance::packToStream(Stream& stream, U32 tabStop, S32 behaviorID, U32 flags /* = 0  */)
{
   char buffer[1024];

   writeFields(stream, tabStop);
}
