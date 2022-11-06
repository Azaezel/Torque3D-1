#pragma once
#include "component.h"
#include "componentInstance.h"

class Component;
class ComponentInstance;

/// <summary>
/// ComponentObject is the root class to derive from for implementing compnents.
/// It contains and handles most of the general 'meat' of management, tracking and processing of functions.
/// Derived classes are free to special-case certain aspects, like handling the network updates
/// (as not all objects that utilize components may need to traffic them across the network)
/// </summary>
class ComponentObject
{
protected:
   /// <summary>
   /// Primary list of ComponentInstances on this Object. We handle these as pointers, as the main memory is handled
   /// in the instance's respective static list, rather than containered here. This lets updates via Directors better
   /// keep everything in memory and better chance of being cache coherent. The list of pointers here is primarily for
   /// management purposes, rather than enacting against them directly via the Object.
   /// </summary>
   Vector<ComponentInstance*> mComponents;

   /// <summary>
   /// A struct of helper data for tracking what components are being networked for this Object, and their network status
   /// </summary>
   struct NetworkedComponent
   {
      /// <summary>
      /// The Index of the componentInstance in question. 
      /// </summary>
      U32 componentIndex;

      /// <summary>
      /// The networking state for the componentInstance. This is used to ensure we invoke the correct
      /// pack/unpack masking on the Object side.
      /// </summary>
      enum UpdateState
      {
         None,
         Adding,
         Removing,
         Updating
      };

      UpdateState updateState;

      /// <summary>
      /// What mask bits are in effect for this componentInstance?
      /// </summary>
      U32 updateMaskBits;
   };

   /// <summary>
   /// Main list of the NetworkedComponents data
   /// </summary>
   Vector<NetworkedComponent> mNetworkedComponents;

public:
   ComponentObject() {}
   ~ComponentObject() {}

#pragma region Add/Remove Functions
   /// <summary>
   /// Creates a ComponentInstance from the Component passed in, and adds it to this Object
   /// </summary>
   /// <param name="component">The Component to use as a template to create an instance of</param>
   /// <returns>A bool indicating if creation and addition of an instance was successful</returns>
   virtual bool addComponent(Component* component);

   /// <summary>
   /// Removes a ComponentInstance from the Object based on the Component passed in.
   /// Looks up the Instance that utilizes the Component as it's template, and removes it.
   /// </summary>
   /// <param name="component">The Component that was used as a template to create an instance</param>
   /// <returns>A bool indicating if removal and deletion of an instance was successful</returns>
   virtual bool removeComponent(Component* component);

   /// <summary>
   /// This clears all componentInstances from the Object
   /// </summary>
   void clearComponents();
#pragma endregion
   

#pragma region ComponentInstance Management
   /// <summary>
   /// This gets a componentInstance from the Object at a specific index
   /// </summary>
   /// <param name="index">The specific index for our list of componentInstances</param>
   /// <returns>A pointer to the componentInstance. If one is not found, it returns nullptr</returns>
   ComponentInstance* getComponentInstance(const U32& index) {
      if (index >= mComponents.size())
         return nullptr;

      return mComponents[index];
   }

   /// <summary>
   /// This gets a componentInstance by the templated Component data used to create the instance.
   /// </summary>
   /// <param name="component">The Component used to template an instance</param>
   /// <returns>A pointer to the componentInstance. If one is not found, it returns nullptr</returns>
   ComponentInstance* getComponentInstanceByData(Component* component);

   /// <summary>
   /// Gets a componentInstance based on a specific class. This uses dynamic_cast and should be used sparingly.
   /// </summary>
   /// <typeparam name="T">The ComponentInstance-derived class we're looking for</typeparam>
   /// <returns>A pointer to the componentInstance, typed to the class provided. If one is not found, it returns nullptr</returns>
   template <class T>
   T* getComponentInstance() {

      for (U32 i = 0; i < mComponents.size(); i++)
      {
         T* compInst = dynamic_cast<T*>(mComponents[i]);
         if (compInst != nullptr)
         {
            return compInst;
         }
      }

      return nullptr;
   }

   /// <summary>
   /// This gets a componentInstance based on the componentType string, contained in the template Component.
   /// ie, This is used to find a componentInstance with the "Render" type.
   /// </summary>
   /// <param name="componentType">A string of the component type to look for</param>
   /// <returns>A pointer to the componentInstance, typed to the class provided. If one is not found, it returns nullptr</returns>
   ComponentInstance* getComponentInstance(StringTableEntry componentType);
#pragma endregion

   /// <summary>
   /// Gets the number of componentInstances this Object has
   /// </summary>
   /// <returns>Number of componentInstances</returns>
   U32 getComponentCount() const
   {
      return mComponents.size();
   }

#pragma region Network Handling
   /// <summary>
   /// Marks the componentInstances on the Object as dirty, to be updated on the network
   /// </summary>
   void setComponentsDirty();

   /// <summary>
   /// Marks a specific componentInstance on the Object as dirty, based on the Component used to template it.
   /// Dirty instance is to be updated on the network
   /// </summary>
   /// <param name="comp">Component used as a template for an instance on the Object</param>
   void setComponentDirty(Component* comp);

   /// <summary>
   /// Sets a specific componentInstance's netmask and ensures the NetworkedComponent listing is marked as to-be-updated
   /// </summary>
   /// <param name="compInst">The componentInstance to have its mask set</param>
   /// <param name="mask">The netmask bits to be set on the instance</param>
   virtual void setComponentNetMask(ComponentInstance* compInst, const U32& mask);
#pragma endregion

   /// <summary>
   /// Notifies components utilizing a signal function callback. This is intended for calls to behavior components on an Object
   /// into script, not all componentInstances.
   /// </summary>
   /// <param name="signalFunction">Script function to be called on behaviors</param>
   /// <param name="argA">Optional Parameter Arg</param>
   /// <param name="argB">Optional Parameter Arg</param>
   /// <param name="argC">Optional Parameter Arg</param>
   /// <param name="argD">Optional Parameter Arg</param>
   /// <param name="argE">Optional Parameter Arg</param>
   void notifyComponents(String signalFunction, String argA, String argB = "", String argC = "", String argD = "", String argE = "");
};
