#include "componentObject.h"

bool ComponentObject::addComponent(Component* component)
{
   component->addComponent(this);

   mComponents.push_back(component->createInstance(this));
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
