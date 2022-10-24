#pragma once

#include "game/components/component.h"
#include "game/components/componentInstance.h"

#include "game/directors/directorManager.h"
#include "game/3D/transforms/transform3DComponent.h"

class RenderMeshDirector;

class RenderMeshComponent : public Component
{
   typedef Component Parent;

public:
   RenderMeshComponent();

   DECLARE_CONOBJECT(RenderMeshComponent);

   bool onAdd();
   static void initPersistFields();
   static void consoleInit();
   virtual void packData(BitStream* stream);
   virtual void unpackData(BitStream* stream);

   virtual bool addComponent(Entity* ent);
   virtual bool removeComponent(Entity* ent);
   //
   //
   virtual ComponentInstance createInstance(Entity* owner) const;

   COMP_REGISTER_SIGNALS();
};

//
class RenderMeshComponentInstance : public ComponentInstance
{
   friend RenderMeshComponent;
   friend RenderMeshDirector;

private:
   static Vector<RenderMeshComponentInstance> sComponentInstanceList;

public:
   RenderMeshComponentInstance() { mComponentData = nullptr; mOwner = nullptr; }
   RenderMeshComponentInstance(const RenderMeshComponent& componentData, const Entity& ownerEntity);
   ~RenderMeshComponentInstance();

   virtual void destroyInstance();

   virtual void update(const MatrixF& transform);
};

//
class RenderMeshDirector : public Director
{
   friend DirectorManager;
   typedef Director Parent;

   struct RenderMeshEntityRef
   {
      Entity* ownerEntity;
      StrongRefPtr<RenderMeshComponentInstance> mesh;
      StrongRefPtr<Transform3DComponentInstance> transform;

      bool isValid()
      {
         if (ownerEntity != nullptr && !mesh.isNull() && !transform.isNull())
            return true;

         return false;
      }
   };

private:
   Vector<RenderMeshEntityRef> mValidEntriesList;

public:
   RenderMeshDirector();
   ~RenderMeshDirector();

   void registerComponent(Entity* entity, const Component& comp);
   void unregisterComponent(Entity* entity, const Component& comp);

   void update();
};
