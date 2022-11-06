#include "componentObject.h"

#pragma region Add/Remove Functions
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
      if (mComponents[i]->getComponentData().getId() == component->getId())
      {
         mComponents[i]->destroyInstance();
         mComponents.erase(i);
         return true;
      }
   }

   component->removeComponent(this);

   return false;
}

void ComponentObject::clearComponents()
{
   while (mComponents.size() > 0)
   {
      removeComponent(mComponents.first()->getComponentDataPtr());
   }
}
#pragma endregion


#pragma region ComponentInstance Management
ComponentInstance* ComponentObject::getComponentInstance(StringTableEntry componentType)
{
   for (U32 i = 0; i < mComponents.size(); i++)
   {
      //Check the template component's type. If it matches we're good
      if (mComponents[i]->getComponentData().getComponentType() == componentType)
         return mComponents[i];
   }

   return NULL;
}

ComponentInstance* ComponentObject::getComponentInstanceByData(Component* component)
{
   for (U32 i = 0; i < mComponents.size(); i++)
   {
      //Check if the id of the template component matches the passed in component
      if (mComponents[i]->getComponentData().getId() == component->getId())
         return mComponents[i];
   }

   return nullptr;
}
#pragma endregion


#pragma region Network Handling
void ComponentObject::setComponentNetMask(ComponentInstance* comp, const U32& mask)
{
   for (U32 i = 0; i < mNetworkedComponents.size(); i++)
   {
      U32 netCompId = mComponents[mNetworkedComponents[i].componentIndex]->getComponentData().getId();
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
   for (U32 i = 0; i < mComponents.size(); i++)
   {
      //force general update
      mComponents[i]->setMaskBits(-1); 
   }
}

void ComponentObject::setComponentDirty(Component* comp)
{
   for (U32 i = 0; i < mComponents.size(); i++)
   {
      //If the pass-in and template component's id's match, mark the instance's bits
      if (mComponents[i]->getComponentData().getId() == comp->getId())
      {
         //force general update
         mComponents[i]->setMaskBits(-1); 
         return;
      }
   }
}
#pragma endregion


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
