#pragma once
#include "Entity.h"
#include "math/mTransform.h"

DefineEngineMethod(Entity, mountObject, bool,
   (SceneObject* objB, TransformF txfm), (MatrixF::Identity),
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   if (objB)
   {
      //BUG: Unsure how it broke, but atm the default transform passed in here is rotated 180 degrees. This doesn't happen
      //for the SceneObject mountobject method. Hackish, but for now, just default to a clean MatrixF::Identity
      object->mountObject(objB, /*MatrixF::Identity*/txfm.getMatrix());
      return true;
   }
   return false;
}

DefineEngineMethod(Entity, setMountOffset, void,
   (Point3F posOffset), (Point3F(0, 0, 0)),
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   object->setMountOffset(posOffset);
}

DefineEngineMethod(Entity, setMountRotation, void,
   (EulerF rotOffset), (EulerF(0, 0, 0)),
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   object->setMountRotation(rotOffset);
}

DefineEngineMethod(Entity, getMountTransform, TransformF, (), ,
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   MatrixF mat;
   object->getMountTransform(0, MatrixF::Identity, &mat);
   return mat;
}

DefineEngineMethod(Entity, setBox, void,
   (Point3F box), (Point3F(1, 1, 1)),
   "@brief Mount objB to this object at the desired slot with optional transform.\n\n"

   "@param objB  Object to mount onto us\n"
   "@param slot  Mount slot ID\n"
   "@param txfm (optional) mount offset transform\n"
   "@return true if successful, false if failed (objB is not valid)")
{
   object->setObjectBox(Box3F(-box, box));
}


/*DefineEngineMethod(Entity, callOnComponents, void, (const char* functionName), ,
   "Get the number of static fields on the object.\n"
   "@return The number of static fields defined on the object.")
{
   object->callOnComponents(functionName);
}

ConsoleMethod(Entity, callMethod, void, 3, 64, "(methodName, argi) Calls script defined method\n"
   "@param methodName The method's name as a string\n"
   "@param argi Any arguments to pass to the method\n"
   "@return No return value"
   "@note %obj.callMethod( %methodName, %arg1, %arg2, ... );\n")

{
   object->callMethodArgList(argc - 1, argv + 2);
}

ConsoleMethod(Entity, addComponents, void, 2, 2, "() - Add all fielded behaviors\n"
   "@return No return value")
{
   object->addComponents();
}*/

DefineEngineMethod(Entity, addComponent, bool, (Component* comp), ,
   "@brief Add a behavior to the object\n"
   "@param bi The behavior instance to add"
   "@return (bool success) Whether or not the behavior was successfully added")
{
   if (comp != NULL)
   {
      bool success = object->addComponent(comp);

      if (success)
      {
         //Placed here so we can differentiate against adding a new behavior during runtime, or when we load all
         //fielded behaviors on mission load. This way, we can ensure that we only call the callback
         //once everything is loaded. This avoids any problems with looking for behaviors that haven't been added yet, etc.
         if (comp->isMethod("onBehaviorAdd"))
            Con::executef(comp, "onBehaviorAdd");

         return true;
      }
   }

   return false;
}

DefineEngineMethod(Entity, removeComponent, bool, (Component* comp, bool deleteComponent), (true),
   "@param bi The behavior instance to remove\n"
   "@param deleteBehavior Whether or not to delete the behavior\n"
   "@return (bool success) Whether the behavior was successfully removed")
{
   return object->removeComponent(comp, deleteComponent);
}

DefineEngineMethod(Entity, clearComponents, void, (), , "Clear all behavior instances\n"
   "@return No return value")
{
   object->clearComponents();
}

DefineEngineMethod(Entity, getComponentByIndex, Component*, (S32 index), ,
   "@brief Gets a particular behavior\n"
   "@param index The index of the behavior to get\n"
   "@return (ComponentInstance bi) The behavior instance you requested")
{
   return object->getComponent(index);
}

DefineEngineMethod(Entity, getComponent, Component*, (String componentName), (""),
   "Get the number of static fields on the object.\n"
   "@return The number of static fields defined on the object.")
{
   return object->getComponent(componentName);
}

/*ConsoleMethod(Entity, getBehaviorByType, S32, 3, 3, "(string BehaviorTemplateName) - gets a behavior\n"
   "@param BehaviorTemplateName The name of the template of the behavior instance you want\n"
   "@return (ComponentInstance bi) The behavior instance you requested")
{
   ComponentInstance *bInstance = object->getComponentByType(StringTable->insert(argv[2]));

   return (bInstance != NULL) ? bInstance->getId() : 0;
}*/

/*ConsoleMethod(Entity, reOrder, bool, 3, 3, "(ComponentInstance inst, [int desiredIndex = 0])\n"
   "@param inst The behavior instance you want to reorder\n"
   "@param desiredIndex The index you want the behavior instance to be reordered to\n"
   "@return (bool success) Whether or not the behavior instance was successfully reordered")
{
   Component *inst = dynamic_cast<Component *>(Sim::findObject(argv[1]));

   if (inst == NULL)
      return false;

   U32 idx = 0;
   if (argc > 2)
      idx = dAtoi(argv[2]);

   return object->reOrder(inst, idx);
}*/

DefineEngineMethod(Entity, getComponentCount, S32, (), ,
   "@brief Get the count of behaviors on an object\n"
   "@return (int count) The number of behaviors on an object")
{
   return object->getComponentCount();
}

DefineEngineMethod(Entity, setComponentDirty, void, (S32 componentID, bool forceUpdate), (0, false),
   "Get the number of static fields on the object.\n"
   "@return The number of static fields defined on the object.")
{
   /*Component* comp;
   if (Sim::findObject(componentID, comp))
      object->setComponentDirty(comp, forceUpdate);*/
}

DefineEngineMethod(Entity, getMoveVector, VectorF, (), ,
   "Get the number of static fields on the object.\n"
   "@return The number of static fields defined on the object.")
{
   //fetch our last move
   if (object->getLastMove().x != 0 || object->getLastMove().y != 0 || object->getLastMove().z != 0)
      return VectorF(object->getLastMove().x, object->getLastMove().y, object->getLastMove().z);

   return VectorF::Zero;
}

DefineEngineMethod(Entity, getMoveRotation, VectorF, (), ,
   "Get the number of static fields on the object.\n"
   "@return The number of static fields defined on the object.")
{
   //fetch our last move
   if (object->getLastMove().pitch != 0 || object->getLastMove().roll != 0 || object->getLastMove().yaw != 0)
      return VectorF(object->getLastMove().pitch, object->getLastMove().roll, object->getLastMove().yaw);

   return VectorF::Zero;
}

DefineEngineMethod(Entity, getMoveTrigger, bool, (S32 triggerNum), (0),
   "Get the number of static fields on the object.\n"
   "@return The number of static fields defined on the object.")
{
   if (object->getControllingClient() != NULL && triggerNum < MaxTriggerKeys)
   {
      return object->getLastMove().trigger[triggerNum];
   }

   return false;
}

DefineEngineMethod(Entity, getForwardVector, VectorF, (), ,
   "Get the direction this object is facing.\n"
   "@return a vector indicating the direction this object is facing.\n"
   "@note This is the object's y axis.")
{
   VectorF forVec = object->getTransform().getForwardVector();
   return forVec;
}

DefineEngineMethod(Entity, setForwardVector, void, (VectorF newForward), (VectorF(0, 0, 0)),
   "Get the number of static fields on the object.\n"
   "@return The number of static fields defined on the object.")
{
   object->setForwardVector(newForward);
}

DefineEngineMethod(Entity, lookAt, void, (Point3F lookPosition), ,
   "Get the number of static fields on the object.\n"
   "@return The number of static fields defined on the object.")
{
   //object->setForwardVector(newForward);
}

DefineEngineMethod(Entity, rotateTo, void, (Point3F lookPosition, F32 degreePerSecond), (1.0),
   "Get the number of static fields on the object.\n"
   "@return The number of static fields defined on the object.")
{
   //object->setForwardVector(newForward);
}

DefineEngineMethod(Entity, notify, void, (String signalFunction, String argA, String argB, String argC, String argD, String argE),
   ("", "", "", "", "", ""),
   "Triggers a signal call to all components for a certain function.")
{
   if (signalFunction == String(""))
      return;

   object->notifyComponents(signalFunction, argA, argB, argC, argD, argE);
}

DefineEngineFunction(findEntitiesByTag, const char*, (SimGroup* searchingGroup, String tags), (nullAsType<SimGroup*>(), ""),
   "Finds all entities that have the provided tags.\n"
   "@param searchingGroup The SimGroup to search inside. If null, we'll search the entire dictionary(this can be slow!).\n"
   "@param tags Word delimited list of tags to search for. If multiple tags are included, the list is eclusively parsed, requiring all tags provided to be found on an entity for a match.\n"
   "@return A word list of IDs of entities that match the tag search terms.")
{
   //if (tags.isEmpty())
   return "";

   /*if (searchingGroup == nullptr)
   {
      searchingGroup = Sim::getRootGroup();
   }

   StringTableEntry entityStr = StringTable->insert("Entity");

   std::thread threadBob;

   std::thread::id a = threadBob.get_id();
   std::thread::id b = std::this_thread::get_id().;

   if (a == b)
   {
      //do
   }

   for (SimGroup::iterator itr = searchingGroup->begin(); itr != searchingGroup->end(); itr++)
   {
      Entity* ent = dynamic_cast<Entity*>((*itr));

      if (ent != nullptr)
      {
         ent->mTags.
      }
   }

   object->notifyComponents(signalFunction, argA, argB, argC, argD, argE);*/
}
