#pragma once

//Register/Unregister Signal Blocks
#define COMP_REGISTER_SIGNALS() \
   typedef Signal <void(ComponentObject* obj, const Component& comp)> AddComponentSignal;\
   static AddComponentSignal smAddedComponentSignal;\
   virtual AddComponentSignal& getAddedSignal() { return smAddedComponentSignal; }\
   \
   typedef Signal <void(ComponentObject* obj, const Component& comp)> RemoveComponentSignal;\
   static RemoveComponentSignal smRemovedComponentSignal;\
   virtual RemoveComponentSignal& getRemovedSignal() { return smRemovedComponentSignal; }
