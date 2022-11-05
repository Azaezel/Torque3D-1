#pragma once

//Register/Unregister Signal Blocks
#define COMP_REGISTER_SIGNALS(className) \
   typedef Signal <void(ComponentObject* obj, const Component& comp)> AddComponentSignal;\
   static AddComponentSignal& getAddedComponentSignal();\
   \
   typedef Signal <void(ComponentObject* obj, const Component& comp)> RemoveComponentSignal;\
   static RemoveComponentSignal& getRemovedComponentSignal(); \
   \
   virtual void addComponent(ComponentObject* obj)\
   {\
      className##::getAddedComponentSignal().trigger(obj, *this);\
   }\
   \
   virtual void removeComponent(ComponentObject* obj)\
   {\
      className##::getRemovedComponentSignal().trigger(obj, *this);\
   }

#define IMPL_COMP_REGISTER_SIGNALS(className) \
   className##::AddComponentSignal& className##::getAddedComponentSignal()\
   {\
      static AddComponentSignal addedComponentSignal;\
      return addedComponentSignal;\
   }\
   \
   className##::RemoveComponentSignal& className##::getRemovedComponentSignal()\
   {\
      static RemoveComponentSignal removedComponentSignal;\
      return removedComponentSignal;\
   }
