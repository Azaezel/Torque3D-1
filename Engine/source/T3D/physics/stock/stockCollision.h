#ifndef _T3D_STOCKCOLLISION_H_
#define _T3D_STOCKCOLLISION_H_

#ifndef _T3D_PHYSICS_PHYSICSCOLLISION_H_
#include "T3D/physics/physicsCollision.h"
#endif

#ifndef _MMATRIX_H_
#include "math/mMatrix.h"
#endif

#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

#include "collision/convex.h"
#include "ts/tsMesh.h"

class PolysoupConvex : public Convex
{
   typedef Convex Parent;
   friend class StockCollision;

public:
   PolysoupConvex();
   ~PolysoupConvex() {};

public:
   Box3F                box;
   Point3F              verts[4];
   PlaneF               normal;
   S32                  idx;
   TSMesh*              mesh;

public:

   // Returns the bounding box in world coordinates
   Box3F getBoundingBox() const;
   Box3F getBoundingBox(const MatrixF& mat, const Point3F& scale) const;

   void getFeatures(const MatrixF& mat, const VectorF& n, ConvexFeature* cf);

   // This returns a list of convex faces to collide against
   void getPolyList(AbstractPolyList* list);

   // This returns the furthest point from the input vector
   Point3F support(const VectorF& v) const;
};

class StockCollision : public PhysicsCollision
{
protected:

   SceneObject* mObject;

   Convex* mConvexList;

   /// local transform is calculated here.
   MatrixF mLocalXfm;

   void _addShape();

public:
   /// THIS CLASS SHOULD BE ROLLED INTO PHYSICS BODY!!
   StockCollision();
   virtual ~StockCollision();

   //collisionShape* getShape();

   const MatrixF& getLocalTransform() const { return mLocalXfm; }

   // PhysicsCollision
   virtual void addPlane(const PlaneF &plane);
   virtual void addBox(const Point3F &halfWidth, const MatrixF &localXfm);
   virtual void addSphere(F32 radius, const MatrixF &localXfm);
   virtual void addCapsule(F32 radius, F32 height, const MatrixF &localXfm);
   virtual bool addConvex(const Point3F *points, U32 count, const MatrixF &localXfm);
   virtual bool addTriangleMesh(const Point3F *vert, U32 vertCount, const U32 *index, U32 triCount, const MatrixF &localXfm);
   virtual bool addHeightfield(const U16 *heights, const bool *holes, U32 blockSize, F32 metersPerSample, const MatrixF &localXfm);

   Convex* getConvexList() { return mConvexList; }

   void setObject(SceneObject* obj);
};

#endif // !_T3D_STOCKCOLLISION_H_
