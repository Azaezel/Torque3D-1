#pragma once

#include "game/components/component.h"
#include "game/directors/directorManager.h"

//class RenderMeshDirector;

class Transform3DComponent : public Component
{
   typedef Component Parent;

public:
   Transform3DComponent();

   DECLARE_CONOBJECT(Transform3DComponent);

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
class Transform3DComponentInstance : public ComponentInstance
{
   friend Transform3DComponent;
   //friend RenderMeshDirector;

protected:
   MatrixF mWorldTransform;
   MatrixF mObjectTransform;
   MatrixF mScale;

private:
   static Vector<Transform3DComponentInstance> sComponentInstanceList;

protected:
   virtual void update();

   virtual void destroyInstance();

public:
   Transform3DComponentInstance() { mComponentData = nullptr; mOwner = nullptr; }
   Transform3DComponentInstance(const Transform3DComponent& componentData, const Entity& ownerEntity);
   ~Transform3DComponentInstance();

   const MatrixF& getWorldTransform() { return mWorldTransform; }
};
