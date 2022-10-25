#pragma once

#include "game/components/component.h"
#include "game/components/componentInstance.h"

#include "game/directors/directorManager.h"
//#include "game/3D/transforms/transform3DComponent.h"

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

   COMP_REGISTER_SIGNALS(RenderMeshComponent);
   //
   //
   virtual ComponentInstance createInstance(ComponentObject* owner) const;
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
   RenderMeshComponentInstance(const RenderMeshComponent& componentData, const ComponentObject& owner);
   ~RenderMeshComponentInstance();

   virtual void destroyInstance();

   virtual void update(const MatrixF& transform);
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
