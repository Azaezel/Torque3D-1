#include "platform/platform.h"
#include "T3D/physics/stock/stockCollision.h"

#include "math/mPoint3.h"
#include "math/mMatrix.h"
#include "collision/boxConvex.h"
#include "collision/abstractPolyList.h"
#include "scene/sceneObject.h"

PolysoupConvex::PolysoupConvex()
   : box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
   normal(0.0f, 0.0f, 0.0f, 0.0f),
   idx(0),
   mesh(NULL)
{
   mType = TSPolysoupConvexType;

   for (U32 i = 0; i < 4; ++i)
   {
      verts[i].set(0.0f, 0.0f, 0.0f);
   }
}

Point3F PolysoupConvex::support(const VectorF& vec) const
{
   F32 bestDot = mDot(verts[0], vec);

   const Point3F* bestP = &verts[0];
   for (S32 i = 1; i < 4; i++)
   {
      F32 newD = mDot(verts[i], vec);
      if (newD > bestDot)
      {
         bestDot = newD;
         bestP = &verts[i];
      }
   }

   return *bestP;
}

Box3F PolysoupConvex::getBoundingBox() const
{
   //Box3F wbox = box;
   //wbox.minExtents.convolve(mObject->getScale());
   //wbox.maxExtents.convolve(mObject->getScale());
   //mObject->getTransform().mul(wbox);
   return box;
}

Box3F PolysoupConvex::getBoundingBox(const MatrixF& mat, const Point3F& scale) const
{
   AssertISV(false, "PolysoupConvex::getBoundingBox(m,p) - Not implemented. -- XEA");
   return box;
}

void PolysoupConvex::getPolyList(AbstractPolyList* list)
{
   // Transform the list into object space and set the pointer to the object
   MatrixF i(mObject->getTransform());
   Point3F iS(mObject->getScale());
   list->setTransform(&i, iS);
   list->setObject(mObject);

   // Add only the original collision triangle
   S32 base = list->addPoint(verts[0]);
   list->addPoint(verts[2]);
   list->addPoint(verts[1]);

   list->begin(0, (U32)idx ^ (uintptr_t)mesh);
   list->vertex(base + 2);
   list->vertex(base + 1);
   list->vertex(base + 0);
   list->plane(base + 0, base + 1, base + 2);
   list->end();
}

void PolysoupConvex::getFeatures(const MatrixF& mat, const VectorF& n, ConvexFeature* cf)
{
   cf->material = 0;
   cf->mObject = mObject;

   // For a tetrahedron this is pretty easy... first
   // convert everything into world space.
   Point3F tverts[4];
   mat.mulP(verts[0], &tverts[0]);
   mat.mulP(verts[1], &tverts[1]);
   mat.mulP(verts[2], &tverts[2]);
   mat.mulP(verts[3], &tverts[3]);

   // points...
   S32 firstVert = cf->mVertexList.size();
   cf->mVertexList.increment(); cf->mVertexList.last() = tverts[0];
   cf->mVertexList.increment(); cf->mVertexList.last() = tverts[1];
   cf->mVertexList.increment(); cf->mVertexList.last() = tverts[2];
   cf->mVertexList.increment(); cf->mVertexList.last() = tverts[3];

   //    edges...
   cf->mEdgeList.increment();
   cf->mEdgeList.last().vertex[0] = firstVert + 0;
   cf->mEdgeList.last().vertex[1] = firstVert + 1;

   cf->mEdgeList.increment();
   cf->mEdgeList.last().vertex[0] = firstVert + 1;
   cf->mEdgeList.last().vertex[1] = firstVert + 2;

   cf->mEdgeList.increment();
   cf->mEdgeList.last().vertex[0] = firstVert + 2;
   cf->mEdgeList.last().vertex[1] = firstVert + 0;

   cf->mEdgeList.increment();
   cf->mEdgeList.last().vertex[0] = firstVert + 3;
   cf->mEdgeList.last().vertex[1] = firstVert + 0;

   cf->mEdgeList.increment();
   cf->mEdgeList.last().vertex[0] = firstVert + 3;
   cf->mEdgeList.last().vertex[1] = firstVert + 1;

   cf->mEdgeList.increment();
   cf->mEdgeList.last().vertex[0] = firstVert + 3;
   cf->mEdgeList.last().vertex[1] = firstVert + 2;

   //    triangles...
   cf->mFaceList.increment();
   cf->mFaceList.last().normal = PlaneF(tverts[2], tverts[1], tverts[0]);
   cf->mFaceList.last().vertex[0] = firstVert + 2;
   cf->mFaceList.last().vertex[1] = firstVert + 1;
   cf->mFaceList.last().vertex[2] = firstVert + 0;

   cf->mFaceList.increment();
   cf->mFaceList.last().normal = PlaneF(tverts[1], tverts[0], tverts[3]);
   cf->mFaceList.last().vertex[0] = firstVert + 1;
   cf->mFaceList.last().vertex[1] = firstVert + 0;
   cf->mFaceList.last().vertex[2] = firstVert + 3;

   cf->mFaceList.increment();
   cf->mFaceList.last().normal = PlaneF(tverts[2], tverts[1], tverts[3]);
   cf->mFaceList.last().vertex[0] = firstVert + 2;
   cf->mFaceList.last().vertex[1] = firstVert + 1;
   cf->mFaceList.last().vertex[2] = firstVert + 3;

   cf->mFaceList.increment();
   cf->mFaceList.last().normal = PlaneF(tverts[0], tverts[2], tverts[3]);
   cf->mFaceList.last().vertex[0] = firstVert + 0;
   cf->mFaceList.last().vertex[1] = firstVert + 2;
   cf->mFaceList.last().vertex[2] = firstVert + 3;

   // All done!
}

//
StockCollision::StockCollision() :
   mLocalXfm(true),
   mObject(NULL)
{
   mConvexList = new Convex;
}

StockCollision::~StockCollision()
{
   mObject = NULL;
}

void StockCollision::setObject(SceneObject* obj)
{
   mConvexList->setObject(obj);

   for (Convex* itr = mConvexList->getNext();
      itr != mConvexList;
      itr = itr->getNext())
   {
      if (itr->getObject() != obj)
         itr->setObject(obj);
   }
}

//
void StockCollision::addPlane(const PlaneF& plane)
{
}

void StockCollision::addBox(const Point3F& halfWidth, const MatrixF& localXfm)
{
   // Create a new convex.
   BoxConvex* cp = new BoxConvex;
   mConvexList->registerObject(cp);

   cp->mCenter = localXfm.getPosition();

   cp->mSize = halfWidth;
}

void StockCollision::addSphere(F32 radius, const MatrixF& localXfm)
{

}

void StockCollision::addCapsule(F32 radius, F32 height, const MatrixF& localXfm)
{

}

bool StockCollision::addConvex(const Point3F* points, U32 count, const MatrixF& localXfm)
{
   for (U32 i = 0; i < count; i+3)
   {
      Point3F a = points[i];
      Point3F b = points[i++];
      Point3F c = points[i++];

      // Transform the result into object space!
      localXfm.mulP(a);
      localXfm.mulP(b);
      localXfm.mulP(c);

      PlaneF p(c, b, a);
      Point3F peak = ((a + b + c) / 3.0f) - (p * 0.15f);

      // Set up the convex...
      PolysoupConvex* cp = new PolysoupConvex();

      mConvexList->registerObject(cp);

      //cp->mesh = this;
      cp->idx = i;
      cp->mObject = mObject;

      cp->normal = p;
      cp->verts[0] = a;
      cp->verts[1] = b;
      cp->verts[2] = c;
      cp->verts[3] = peak;

      // Update the bounding box.
      Box3F& bounds = cp->box;
      bounds.minExtents.set(F32_MAX, F32_MAX, F32_MAX);
      bounds.maxExtents.set(-F32_MAX, -F32_MAX, -F32_MAX);

      bounds.minExtents.setMin(a);
      bounds.minExtents.setMin(b);
      bounds.minExtents.setMin(c);
      bounds.minExtents.setMin(peak);

      bounds.maxExtents.setMax(a);
      bounds.maxExtents.setMax(b);
      bounds.maxExtents.setMax(c);
      bounds.maxExtents.setMax(peak);
   }

   return true;
}

bool StockCollision::addTriangleMesh(const Point3F* vert, U32 vertCount, const U32* index, U32 triCount, const MatrixF& localXfm)
{
   return false;
}

bool StockCollision::addHeightfield(const U16* heights, const bool* holes, U32 blockSize, F32 metersPerSample, const MatrixF& localXfm)
{
   return false;
}
