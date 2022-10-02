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
   /// Transform from object space to world space.
   MatrixF mObjToWorld;

   /// Transform from world space to object space (inverse).
   MatrixF mWorldToObj;

   /// Object scale.
   Point3F mObjScale;

   struct StateDelta
   {
      Move move;                    ///< Last move from server
      F32 dt;                       ///< Last interpolation time
      // Interpolation data
      Point3F pos;
      Point3F posVec;
      QuatF rot[2];
      // Warp data
      S32 warpTicks;                ///< Number of ticks to warp
      S32 warpCount;                ///< Current pos in warp
      Point3F warpOffset;
      QuatF warpRot[2];
   };

private:
   static Vector<Transform3DComponentInstance> sComponentInstanceList;

protected:
   virtual void update();

   virtual void destroyInstance();

public:
   Transform3DComponentInstance() { mComponentData = nullptr; mOwner = nullptr; }
   Transform3DComponentInstance(const Transform3DComponent& componentData, const Entity& ownerEntity);
   ~Transform3DComponentInstance();

   const MatrixF& getWorldTransform() { return mObjToWorld; }
};
