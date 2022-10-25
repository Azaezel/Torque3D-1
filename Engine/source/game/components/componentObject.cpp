#include "componentObject.h"

bool ComponentObject::addComponent(Component* component)
{
   mComponents.push_back(component->createInstance(this));
   component->addComponent(this); //trips the notify system
   return true;
}

bool ComponentObject::removeComponent(Component* component)
{
   for (U32 i = 0; i < mComponents.size(); i++)
   {
      if (mComponents[i].getComponentData().getId() == component->getId())
      {
         mComponents.erase(i);
         return true;
      }
   }

   component->removeComponent(this);

   return false;
}

void ComponentObject::setComponentNetMask(ComponentInstance* comp, U32 mask)
{
   for (U32 i = 0; i < mNetworkedComponents.size(); i++)
   {
      U32 netCompId = mComponents[mNetworkedComponents[i].componentIndex].getComponentData().getId();
      U32 compId = comp->getComponentData().getId();

      if (netCompId == compId &&
         (mNetworkedComponents[i].updateState == NetworkedComponent::None || mNetworkedComponents[i].updateState == NetworkedComponent::Updating))
      {
         mNetworkedComponents[i].updateState = NetworkedComponent::Updating;
         mNetworkedComponents[i].updateMaskBits |= mask;

         break;
      }
   }
}

void ComponentObject::setComponentsDirty()
{
   /*if (mToLoadComponents.empty())
      mStartComponentUpdate = true;

   //we need to build a list of behaviors that need to be pushed across the network
   for (U32 i = 0; i < mComponents.size(); i++)
   {
      // We can do this because both are in the string table
      Component *comp = mComponents[i];

      if (comp->isNetworked())
      {
         bool unique = true;
         for (U32 i = 0; i < mToLoadComponents.size(); i++)
         {
            if (mToLoadComponents[i]->getId() == comp->getId())
            {
               unique = false;
               break;
            }
         }
         if (unique)
            mToLoadComponents.push_back(comp);
      }
   }

   setMaskBits(ComponentsMask);*/
}

void ComponentObject::setComponentDirty(Component* comp, bool forceUpdate)
{
   for (U32 i = 0; i < mComponents.size(); i++)
   {
      if (mComponents[i].getComponentData().getId() == comp->getId())
      {
         mComponents[i].setMaskBits(-1); //force general update
         return;
      }
   }

   //if (!found)
   //   return;

   //if(mToLoadComponents.empty())
   //	mStartComponentUpdate = true;

   /*if (comp->isNetworked() || forceUpdate)
   {
      bool unique = true;
      for (U32 i = 0; i < mToLoadComponents.size(); i++)
      {
         if (mToLoadComponents[i]->getId() == comp->getId())
         {
            unique = false;
            break;
         }
      }
      if (unique)
         mToLoadComponents.push_back(comp);
   }

   setMaskBits(ComponentsMask);*/

}

void ComponentObject::notifyComponents(String signalFunction, String argA, String argB, String argC, String argD, String argE)
{
   /*for (U32 i = 0; i < mComponents.size(); i++)
   {
      // We can do this because both are in the string table
      const Component& comp = mComponents[i].getComponentData();

      if (mComponents[i].isEnabled())
      {
         if (comp.isMethod(signalFunction))
            Con::executef(comp, signalFunction, argA, argB, argC, argD, argE);
      }
   }*/
}

//////////////////////////////////////////////////////////////////////////
//NOTE:
//The actor class calls this and flags the deletion of the behaviors to false so that behaviors that should no longer be attached during
//a network update will indeed be removed from the object. The reason it doesn't delete them is because when clearing the local behavior
//list, it would delete them, purging the ghost, and causing a crash when the unpack update tried to fetch any existing behaviors' ghosts
//to re-add them. Need to implement a clean clear function that will clear the local list, and only delete unused behaviors during an update.
void ComponentObject::clearComponents(bool deleteComponents)
{
   if (!deleteComponents)
   {
      while (mComponents.size() > 0)
      {
         removeComponent(mComponents.first().getComponentDataPtr());
      }
   }
   else
   {
      while (mComponents.size() > 0)
      {
         Component* comp = mComponents.last().getComponentDataPtr();

         if (comp)
         {
            //comp->onComponentRemove(); //in case the behavior needs to do cleanup on the owner

            comp->deleteObject();
         }
         mComponents.pop_back();
      }
   }
}

ComponentInstance* ComponentObject::getComponent(const U32 index) const
{

   if (index < mComponents.size())
      return const_cast<ComponentInstance*>(&mComponents[index]);

   return nullptr;
}

ComponentInstance* ComponentObject::getComponent(StringTableEntry componentType)
{
   for (U32 i = 0; i < mComponents.size(); i++)
   {
      ComponentInstance* comp = const_cast<ComponentInstance*>(&mComponents[i]);

      if (comp->getComponentData().getComponentType() == componentType)
         return comp;
   }

   return NULL;
}
