#pragma once
#include "componentInstance.h"

class ComponentObject
{
protected:
   Vector<ComponentInstance> mComponents;

public:
   ComponentInstance* getComponentInstance(const U32& index) {
      if (index >= mComponents.size())
         return nullptr;

      return &mComponents[index];
   }

   template <class T>
   T* getComponentInstance() {

      for (U32 i = 0; i < mComponents.size(); i++)
      {
         T* compInst = dynamic_cast<T*>(mComponents[i]);
         if (compInst != nullptr)
         {
            return T;
         }
      }

      return nullptr;
   }

   U32 getComponentCount() const
   {
      return mComponents.size();
   }

   bool addComponent(Component* component);
   bool removeComponent(Component* component);
};
