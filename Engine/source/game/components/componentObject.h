#pragma once
#include "component.h"
#include "componentInstance.h"

class Component;
class ComponentInstance;

class ComponentObject
{
protected:
   Vector<ComponentInstance> mComponents;

   //Bit of helper data to let us track and manage the adding, removal and updating of networked components
   struct NetworkedComponent
   {
      U32 componentIndex;

      enum UpdateState
      {
         None,
         Adding,
         Removing,
         Updating
      };

      UpdateState updateState;

      U32 updateMaskBits;
   };

   Vector<NetworkedComponent> mNetworkedComponents;

   U32                        mComponentNetMask;

   bool                       mStartComponentUpdate;

public:
   ComponentObject() {}
   ~ComponentObject() {}

   ComponentInstance* getComponentInstance(const U32& index) {
      if (index >= mComponents.size())
         return nullptr;

      return &mComponents[index];
   }

   template <class T>
   T* getComponentInstance() {

      for (U32 i = 0; i < mComponents.size(); i++)
      {
         T* compInst = dynamic_cast<T*>(&mComponents[i]);
         if (compInst != nullptr)
         {
            return compInst;
         }
      }

      return nullptr;
   }

   ComponentInstance* getComponent(const U32 index) const;
   ComponentInstance* getComponent(StringTableEntry componentType);

   U32 getComponentCount() const
   {
      return mComponents.size();
   }

   bool addComponent(Component* component);
   bool removeComponent(Component* component);

   void setComponentsDirty();
   void setComponentDirty(Component* comp, bool forceUpdate = false);

   virtual void setComponentNetMask(ComponentInstance* comp, U32 mask);

   void notifyComponents(String signalFunction, String argA, String argB = "", String argC = "", String argD = "", String argE = "");

   void clearComponents(bool deleteComponents);
};
