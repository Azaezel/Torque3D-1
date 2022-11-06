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
   virtual void packData(BitStream* stream);
   virtual void unpackData(BitStream* stream);

   COMP_REGISTER_SIGNALS(RenderMeshComponent);
   //
   //
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

   TSShapeInstance* mShapeInstance;

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

   virtual void destroyInstance();

   virtual void update(const MatrixF& transform);

   void drawDebug(ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance*);

   //
   virtual U32 packUpdate(NetConnection* con, U32 mask, BitStream* stream);
   virtual void unpackUpdate(NetConnection* con, BitStream* stream);

   //
   void updateShape();
   void setupShape();
};

//
class RenderMeshDirector : public Director
{
   typedef Director Parent;
   friend DirectorManager;

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
   Vector<RenderMeshEntityRef> mValidEntriesList;

public:
   RenderMeshDirector();
   ~RenderMeshDirector();

   void registerComponent(ComponentObject* owner, const Component& comp);
   void unregisterComponent(ComponentObject* owner, const Component& comp);

   virtual void update();
};
