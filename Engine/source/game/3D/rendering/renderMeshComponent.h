#pragma once

#include "game/components/component.h"
#include "game/components/componentInstance.h"

#include "game/directors/directorManager.h"
//#include "game/3D/transforms/transform3DComponent.h"

#include "T3D/assets/ShapeAsset.h"

class RenderMeshDirector;

class RenderMeshComponent : public Component
{
   typedef Component Parent;

private:
   void onShapeChange() {}

public:
   RenderMeshComponent();

   DECLARE_CONOBJECT(RenderMeshComponent);

   bool onAdd();
   static void initPersistFields();
   static void consoleInit();

   /// <summary>
   /// See Component::packData()
   /// </summary>
   virtual void packData(BitStream* stream);
   /// <summary>
   /// See Component::unpackData()
   /// </summary>
   virtual void unpackData(BitStream* stream);

   /// <summary>
   /// This sets up some common, boilerplate signal calls that hook into the Director notifications
   /// Namely, when a component is added or removed, the signals will inform the associated director
   /// And the event can be handled
   /// </summary>
   COMP_REGISTER_SIGNALS(RenderMeshComponent);

   /// <summary>
   /// Creates a RenderMeshComponentInstance based on this template RenderMeshComponent.
   /// Also invokes setupFields to ensure that the RenderMeshComponentInstance is fully templated from this RenderMeshComponent
   /// Once created, the RenderMeshComponent is added to it's own static master list for self-management and retention
   /// </summary>
   /// <param name="owner">Owner ComponentObject to associate to the RenderMeshComponentInstance</param>
   /// <returns>The created ComponentInstance</returns>
   virtual ComponentInstance* createInstance(ComponentObject* owner);
};

class ObjectRenderInst;
class BaseMatInstance;

//
class RenderMeshComponentInstance : public ComponentInstance
{
   typedef ComponentInstance Parent;

   friend RenderMeshComponent;
   friend RenderMeshDirector;

private:
   static Vector<RenderMeshComponentInstance*> sComponentInstanceList;

   /// <summary>
   /// This is purely as a temporary variable for testing/validation purposes
   /// </summary>
   MatrixF transform;

   /// <summary>
   /// This is our shapeInstance we use to render our shape action
   /// </summary>
   TSShapeInstance* mShapeInstance;

   /// <summary>
   /// Normal ShapeAsset macros to set up the shape asset, resource and fields
   /// </summary>
   void onShapeChange() {}
   DECLARE_SHAPEASSET(RenderMeshComponentInstance, Shape, onShapeChange);
   DECLARE_ASSET_SETGET(RenderMeshComponentInstance, Shape);

public:
   DECLARE_CONOBJECT(RenderMeshComponentInstance);

   /// <summary>
   /// Obligatory default constructor
   /// </summary>
   RenderMeshComponentInstance() { mComponentData = nullptr; mOwner = nullptr; }
   /// <summary>
   /// The main constructor actually utilized by DOCs
   /// This will assign the template componentData and owner ComponentObject for this componentInstance
   /// </summary>
   /// <param name="componentData">Template Component</param>
   /// <param name="owner">Owner ComopnentObject</param>
   RenderMeshComponentInstance(const RenderMeshComponent& componentData, const ComponentObject& owner);
   ~RenderMeshComponentInstance();

   static void initPersistFields();

   /// <summary>
   /// Destroys this ComponentInstance, removing it from the static list
   /// </summary>
   virtual void destroyInstance();

   /// <summary>
   /// Called by RenderMeshDirector when it runs, this will render our shapeInstance(if we have one)
   /// </summary>
   /// <param name="transform">The transform to render at</param>
   virtual void update(const MatrixF& transform);

   /// <summary>
   /// A delegate function called during editor rendering passes to draw useful debugging information
   /// </summary>
   /// <param name="ri">The renderInst used to draw</param>
   /// <param name="state">Current SceneRenderState</param>
   /// <param name="">N/A</param>
   void drawDebug(ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance*);

   /// <summary>
   /// See ComponentInstance::packUpdate();
   /// </summary>
   virtual U32 packUpdate(NetConnection* con, U32 mask, BitStream* stream);
   /// <summary>
   /// See ComponentInstance::unpackUpdate();
   /// </summary>
   virtual void unpackUpdate(NetConnection* con, BitStream* stream);

   /// <summary>
   /// Validates and processes a shapeAsset to ensure our shape resources are good to go for use
   /// </summary>
   void updateShape();
   /// <summary>
   /// Updates the current shape and shapeInstance stuffs if we have a valid asset
   /// </summary>
   void setupShape();
};

//
class RenderMeshDirector : public Director
{
   typedef Director Parent;
   friend DirectorManager;

   /// <summary>
   /// A struct containing an owner ComponentObject and its relevent components
   /// If this Director tracks if it's a valid ref or not, so if any of the required components
   /// are missing, invalid, or disabled, we can easily skip it and move on without needing to
   /// re-juggle lists or dependency tracking
   /// </summary>
   struct RenderMeshEntityRef
   {
      ComponentObject* owner;
      StrongRefPtr<RenderMeshComponentInstance> mesh;
      //StrongRefPtr<Transform3DComponentInstance> transform;

      bool isValid()
      {
         if (owner != nullptr && !mesh.isNull() /* && !transform.isNull()*/)
            return true;

         return false;
      }
   };

private:
   /// <summary>
   /// A list of validated RenderMeshEntityRef entries.
   /// </summary>
   Vector<RenderMeshEntityRef> mValidEntriesList;

public:
   RenderMeshDirector();
   ~RenderMeshDirector();

   void registerComponent(ComponentObject* owner, const Component& comp);
   void unregisterComponent(ComponentObject* owner, const Component& comp);

   /// <summary>
   /// The main update function. Will iterate over valid RenderMeshEntityRef's and invoke them to update
   /// In our case, this means we take the mesh ref to a RenderMeshComponentInstance and have it render the shape
   /// </summary>
   virtual void update();
};
