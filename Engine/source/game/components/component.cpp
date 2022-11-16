#include "component.h"

#include "platform/platform.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stream/bitStream.h"

#include "core/stream/fileStream.h"
#include "T3D/assets/ImageAsset.h"
#include "T3D/assets/ShapeAsset.h"
#include "T3D/assets/MaterialAsset.h"
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(Component);

IMPL_COMP_REGISTER_SIGNALS(Component);

ConsoleDocClass( Component,
   "@brief Component is a datablock-based class of object. It's used as a template for associated ComponentInstances\n"
   "@ingroup Datablocks\n"
);

Component::Component() :
   mFriendlyName(StringTable->EmptyString()),
   mDescription(StringTable->EmptyString()),
   mComponentType(StringTable->EmptyString()),
   mNetworked(false)
{
}

bool Component::onAdd()
{
   if (!Parent::onAdd())
      return false;

   //All componentInstances need the template name, so it's done as a componentField here. This way it's always
   //set up on the created instances, and it's derived from the component's name as per the default field being the getName()
   //addComponentField("templateName", "Name of the Component this ComponentInstance is templated from", "String", getName());

   return true;
}

void Component::consoleInit()
{
   Parent::consoleInit();
}

void Component::initPersistFields()
{
   Parent::initPersistFields();

   addField("friendlyName", TypeString, Offset(mFriendlyName, Component), "This is the human-friendly name. Mainly just used for editor interface stuff");
   addField("description", TypeString, Offset(mDescription, Component), "Description of the component");
   addField("componentType", TypeString, Offset(mComponentType, Component), "The category group this component fits into. For organizational purposes in the editor, ie \"Render\" or \"Physics\"");
   addField("isNetworked", TypeBool, Offset(mNetworked, Component), "Indicates if this component should be networked down to the client or not");
}

ComponentInstance* Component::createInstance(ComponentObject* owner)
{
   ComponentInstance* compInst = new ComponentInstance(*this, *owner);

   if (!compInst->registerObject())
   {
      Con::errorf("Component::createInstance() - failed to create instance");
      return nullptr;
   }

   setupFields(compInst, true);

   //Now we register the component to the static list that contains all the ComponentInstances(of the explicit class)
   //This gives us a memory-coherent list to iterate over when doing work on components, improving cache coherency
   ComponentInstance::getComponentInstList()->push_back(compInst);

   return compInst;
}

#pragma region Field Management
bool Component::setupFields(ComponentInstance* componentInstance, bool forceSetup)
{
   for (S32 i = 0; i < mFields.size(); ++i)
   {
      ComponentField& field = const_cast<ComponentField&>(mFields[i]);

      //Add the field from us to the compInstance
      componentInstance->addComponentField(field);

      //check if this field already has data or not
      //if it's blank or we're going to force it, we're good to continue setting it.
      const char* data = componentInstance->getDataField(StringTable->insert(field.mFieldName), NULL);

      if (forceSetup || !dStrcmp(data, ""))
      {
         //Now we check to see if the component has existing data
         //If so, this indicates the field was overridden and we want to use that data
         //If not, then we'll use the default value from the componentField define
         const char* newData = getDataField(StringTable->insert(field.mFieldName), NULL);
         if (!newData || !newData[0])
            newData = field.mDefaultValue;

         //Now set the instance's field data
         componentInstance->setDataField(field.mFieldName, NULL, newData);
      }
   }

   return true;
}

void Component::addComponentField(const char* fieldName, const char* desc, const char* type, const char* defaultValue /* = NULL */, const char* userData /* = NULL */, /*const char* dependency /* = NULL *//*,*/ bool hidden /* = false */)
{
   StringTableEntry stFieldName = StringTable->insert(fieldName);

   for (S32 i = 0; i < mFields.size(); ++i)
   {
      if (mFields[i].mFieldName == stFieldName)
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
   else if (fieldType == StringTable->insert("position") || fieldType == StringTable->insert("TypeMatrixPosition"))
      fieldTypeMask = TypeMatrixPosition;
   else if (fieldType == StringTable->insert("rotation") || fieldType == StringTable->insert("TypeMatrixRotation"))
      fieldTypeMask = TypeMatrixRotation;
   else if(fieldType == StringTable->insert("Point3F") || fieldType == StringTable->insert("TypePoint3F"))
      fieldTypeMask = TypePoint3F;
   else
      fieldTypeMask = -1;

   ComponentField field;
   field.mFieldName = stFieldName;

   field.mFieldTypeName = fieldType;
   field.mFieldType = fieldTypeMask;

   field.mUserData = StringTable->insert(userData ? userData : "");
   field.mDefaultValue = StringTable->insert(defaultValue ? defaultValue : "");
   field.mFieldDescription = getDescriptionText(desc);

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
#pragma endregion


#pragma region Networking
void Component::packData(BitStream* stream)
{
   Parent::packData(stream);
}

void Component::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);
}
#pragma endregion


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


#pragma region CallMethod Passthrough
bool Component::handlesConsoleMethod(const char* fname, S32* routingId)
{
   // CodeReview: Host object is now given priority over components for method
   // redirection. [6/23/2007 Pat]

   // On this object?
   if (isMethod(fname))
   {
      *routingId = -1; // -1 denotes method on object
      return true;
   }

   // on this objects components?
   /*for (U32 i = 0; i < mComponents.size(); i++)
   {
      ComponentInstance* pComponent = mComponents[i];
      if (pComponent != NULL && pComponent->isMethod(fname))
      {
         *routingId = -2; // -2 denotes method on component
         return true;
      }
   }*/

   return false;
}

const char* Component::callMethod(S32 argc, const char* methodName, ...)
{
   ConsoleValue argv[128];
   methodName = StringTable->insert(methodName);

   argc++;

   va_list args;
   va_start(args, methodName);
   for (S32 i = 0; i < argc; i++)
      argv[i + 2].setString(va_arg(args, const char*));
   va_end(args);

   // FIXME: the following seems a little excessive. I wonder why it's needed?
   argv[0].setString(methodName);
   argv[1].setString(methodName);
   argv[2].setString(methodName);

   return callMethodArgList(argc, argv);
}

const char* Component::callMethodArgList(U32 argc, ConsoleValue argv[], bool callThis /* = true  */)
{
   return _callMethod(argc, argv, callThis);
}

// Call all components that implement methodName giving them a chance to operate
// Components are called in reverse order of addition
const char* Component::_callMethod(U32 argc, ConsoleValue argv[], bool callThis /* = true  */)
{
   // Set Owner
   SimObject* pThis = dynamic_cast<SimObject*>(this);
   AssertFatal(pThis, "DynamicConsoleMethodComponent::callMethod : this should always exist!");

   if (pThis == NULL)
   {
      char* empty = Con::getReturnBuffer(4);
      empty[0] = 0;

      return empty;
   }

   const char* cbName = StringTable->insert(argv[0]);

   /*if (getComponentCount() > 0)
   {
      for (U32 i = 0; i < mComponents.size(); i++)
      {
         ComponentInstance* pComponent = mComponents[i];

         if (pComponent->isEnabled())
            Con::execute(pComponent, argc, argv);
      }
   }*/

   // Set Owner Field
   const char* result = "";
   if (callThis)
      result = Con::execute(pThis, argc, argv, true); // true - exec method onThisOnly, not on DCMCs

   return result;
}
#pragma endregion CallMethod Passthrough
