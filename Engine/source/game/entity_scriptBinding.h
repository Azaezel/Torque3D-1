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

DefineEngineMethod(Entity, addComponent, bool, (Component* toAddComponent), (nullAsType<Component*>()),
   "@brief Add a component to the entity\n\n")
{
   return object->addComponent(toAddComponent);
}

DefineEngineMethod(Entity, removeComponent, bool, (Component* toRemoveComponent), (nullAsType<Component*>()),
   "@brief Remove a component from the entity\n")
{
   return object->removeComponent(toRemoveComponent);
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
   return object->getComponentInstance(index)->getComponentDataPtr();
}

DefineEngineMethod(Entity, getComponent, Component*, (String componentName), (""),
   "Get the number of static fields on the object.\n"
   "@return The number of static fields defined on the object.")
{
   return object->getComponentInstance(componentName)->getComponentDataPtr();
}

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
