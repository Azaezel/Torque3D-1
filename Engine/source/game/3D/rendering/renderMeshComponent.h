#pragma once

#include "game/components/component.h"
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


   //
   //
   virtual ComponentInstance createInstance(Entity* owner) const;
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

   void registerEntity(Entity* entity);
   void unregisterEntity(Entity* entity);

   void update();
};
