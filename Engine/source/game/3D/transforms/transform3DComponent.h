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

   /// <summary>
   /// This sets up some common, boilerplate signal calls that hook into the Director notifications
   /// Namely, when a component is added or removed, the signals will inform the associated director
   /// And the event can be handled
   /// </summary>
   COMP_REGISTER_SIGNALS(Transform3DComponent);


   //
   //
   virtual ComponentInstance* createInstance(ComponentObject* owner);
};

//
class Transform3DComponentInstance : public ComponentInstance
{
   typedef ComponentInstance Parent;
   friend Transform3DComponent;

public:
   enum MaskBits
   {
      TransformMask = Parent::NextFreeMask << 0,
      BoundsMask = Parent::NextFreeMask << 1,
      ScaleMask = Parent::NextFreeMask << 2,
      NextFreeMask = Parent::NextFreeMask << 3,
   };
protected:
   /// Transform from object space to world space.
   MatrixF mObjToWorld;

   /// Transform from world space to object space (inverse).
   MatrixF mWorldToObj;

   /// Object scale.
   Point3F mObjScale;

   /// Bounding box in object space.
   Box3F mObjBox;

   /// Bounding box (AABB) in world space.
   Box3F mWorldBox;

   /// Bounding sphere in world space.
   SphereF mWorldSphere;

   /// Render matrix to transform object space to world space.
   MatrixF mRenderObjToWorld;

   /// Render matrix to transform world space to object space.
   MatrixF mRenderWorldToObj;

   /// Render bounding box in world space.
   Box3F mRenderWorldBox;

   /// Render bounding sphere in world space.
   SphereF mRenderWorldSphere;

   /// Whether this object is considered to have an infinite bounding box.
   bool mGlobalBounds;

   struct StateDelta
   {
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

   StateDelta mDelta;
   S32 mPredictionCount;            ///< Number of ticks to predict

private:
   static Vector<Transform3DComponentInstance*> sComponentInstanceList;

protected:
   virtual void update();

   virtual void destroyInstance();

public:
   DECLARE_CONOBJECT(Transform3DComponentInstance);

   Transform3DComponentInstance() { mComponentData = nullptr; mOwner = nullptr; }
   Transform3DComponentInstance(const Transform3DComponent& componentData, const ComponentObject& ownerEntity);
   ~Transform3DComponentInstance();

   static void initPersistFields();

   /// <summary>
   /// See ComponentInstance::packUpdate();
   /// </summary>
   virtual U32 packUpdate(NetConnection* con, U32 mask, BitStream* stream);
   /// <summary>
   /// See ComponentInstance::unpackUpdate();
   /// </summary>
   virtual void unpackUpdate(NetConnection* con, BitStream* stream);

#pragma region World/Transform
   static bool _setFieldPosition(void* object, const char* index, const char* data);
   static bool _setFieldRotation(void* object, const char* index, const char* data);
   static bool _setFieldScale(void* object, const char* index, const char* data);

   /// Regenerates the world-space bounding box and bounding sphere.
   void resetWorldBox();

   /// Regenerates the render-world-space bounding box and sphere.
   void resetRenderWorldBox();

   /// Regenerates the object-space bounding box from the world-space
   /// bounding box, the world space to object space transform, and
   /// the object scale.
   void resetObjectBox();

   /// Returns the transform which can be used to convert object space
      /// to world space
   virtual const MatrixF& getTransform() const { return mObjToWorld; }

   /// Returns the transform which can be used to convert world space
   /// into object space
   const MatrixF& getWorldTransform() const { return mWorldToObj; }

   /// Returns the scale of the object
   virtual const VectorF& getScale() const { return mObjScale; }

   /// Returns the bounding box for this object in local coordinates.
   const Box3F& getObjBox() const { return mObjBox; }

   /// Returns the bounding box for this object in world coordinates.
   const Box3F& getWorldBox() const { return mWorldBox; }

   /// Returns the bounding sphere for this object in world coordinates.
   const SphereF& getWorldSphere() const { return mWorldSphere; }

   /// Returns the center of the bounding box in world coordinates
   Point3F getBoxCenter() const { return (mWorldBox.minExtents + mWorldBox.maxExtents) * 0.5f; }

   /// Sets the Object -> World transform
   ///
   /// @param   mat   New transform matrix
   virtual void setTransform(const MatrixF& mat);

   /// Sets the scale for the object
   /// @param   scale   Scaling values
   virtual void setScale(const VectorF& scale);

   /// Called when the size of the object changes.
   virtual void onScaleChanged() {}

   /// Sets the forward vector of the object
   void setForwardVector(VectorF newForward, VectorF upVector = VectorF(0, 0, 1));

   /// This sets the render transform for this object
   /// @param   mat   New render transform
   virtual void setRenderTransform(const MatrixF& mat);

   /// Returns the render transform
   const MatrixF& getRenderTransform() const { return mRenderObjToWorld; }

   /// Returns the render transform to convert world to local coordinates
   const MatrixF& getRenderWorldTransform() const { return mRenderWorldToObj; }

   /// Returns the render world box
   const Box3F& getRenderWorldBox()  const { return mRenderWorldBox; }

   /// Returns the position of the object.
   virtual Point3F getPosition() const;

   /// Returns the render-position of the object.
   ///
   /// @see getRenderTransform
   Point3F getRenderPosition() const;

   /// Sets the position of the object
   void setPosition(const Point3F& pos);
#pragma endregion

   typedef Signal <void(ComponentObject* obj, const Component& comp)> EditorChangedTransformSignal;
   static EditorChangedTransformSignal& getEditorChangedTransformSignal();


};
