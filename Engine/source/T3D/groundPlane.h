//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef _TORQUE_T3D_GROUNDPLANE_H_
#define _TORQUE_T3D_GROUNDPLANE_H_

#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif
#ifndef _GFXVERTEXBUFFER_H_
#include "gfx/gfxVertexBuffer.h"
#endif
#ifndef _GFXPRIMITIVEBUFFER_H_
#include "gfx/gfxPrimitiveBuffer.h"
#endif

#include "T3D/assets/MaterialAsset.h"

class PhysicsBody;
class BaseMatInstance;


/// A virtually infinite XY ground plane primitive.
///
/// For rendering, a subset of the plane spanning the view frustum is generated
/// and rendered.  Tesselation is determined by the given squareSize property.
///
/// For collision detection, a finite bounding box is used to deal with finite
/// precision of floating-point operations (we can't use floating-point infinity
/// as infinity*0 is undefined.)
///
/// The ground plane can be textured like regular geometry by assigning a material
/// name to its 'material' property.  UVs mirror grid coordinates so that when
/// using UV wrapping, textures will tile nicely.


class GroundPlane : public SceneObject
{
public:

   typedef SceneObject Parent;

   DECLARE_CONOBJECT( GroundPlane );

   GroundPlane();
   virtual ~GroundPlane();

   virtual bool      onAdd();
   virtual void      onRemove();
   virtual U32       packUpdate( NetConnection* connection, U32 mask, BitStream* stream );
   virtual void      unpackUpdate( NetConnection* connection, BitStream* stream );
   virtual void      prepRenderImage( SceneRenderState* state );
   virtual bool      castRay( const Point3F& start, const Point3F& end, RayInfo* info );
   virtual void      buildConvex( const Box3F& box, Convex* convex );
   virtual bool      buildPolyList( PolyListContext context, AbstractPolyList* polyList, const Box3F& box, const SphereF& sphere );
   virtual void      inspectPostApply();
   virtual void      setTransform( const MatrixF &mat );
   virtual void      setScale( const Point3F& scale );

   static void       initPersistFields();

   virtual void getUtilizedAssets(Vector<StringTableEntry>* usedAssetsList);

protected:

   typedef GFXVertexPNTBT VertexType;

   void _updateMaterial();

   void              createGeometry( const Frustum& frustum );
   void              projectFrustum( const Frustum& frustum, F32 squareSize,
                                     Point2F& outMin, Point2F& outMax );
   void              generateGrid( U32 width, U32 height, F32 squareSize,
                                   const Point2F& min, const Point2F& max,
                                   GFXVertexBufferHandle< VertexType >& outVertices,
                                   GFXPrimitiveBufferHandle& outPrimitives );

   Box3F             getPlaneBox();

private:

   typedef GFXVertexBufferHandle< VertexType > VertexBuffer;
   typedef GFXPrimitiveBufferHandle PrimitiveBuffer;

   F32               mSquareSize;   ///< World units per grid cell edge.
   F32               mScaleU;       ///< Scale factor for U texture coordinates.
   F32               mScaleV;       ///< Scale factor for V texture coordinates.

   //DECLARE_MATERIALASSET(GroundPlane, Material);


public:
   String mMaterialName;
   StringTableEntry mMaterialAssetId;
   AssetPtr<MaterialAsset>  mMaterialAsset;
   SimObjectPtr<Material> mMaterial;
   BaseMatInstance* mMaterialInst;
public:
   const StringTableEntry getMaterialFile() const { return StringTable->insert(mMaterialName.c_str()); }
   void setMaterialName(const FileName& _in) { mMaterialName = _in; }
   const AssetPtr<MaterialAsset>& getMaterialAsset() const { return mMaterialAsset; }
   void setMaterialAsset(const AssetPtr<MaterialAsset>& _in) { mMaterialAsset = _in; }

   bool _setMaterial(StringTableEntry _in)
   {
      if (_in == StringTable->EmptyString())
      {
         mMaterialName = String::EmptyString;
         mMaterialAssetId = StringTable->EmptyString();
         mMaterialAsset = NULL;
         mMaterial = NULL;
         SAFE_DELETE(mMaterialInst);
         return true;
      }

      if (AssetDatabase.isDeclaredAsset(_in))
      {
         mMaterialAssetId = _in;

         U32 assetState = MaterialAsset::getAssetById(mMaterialAssetId, &mMaterialAsset);

         if (MaterialAsset::Ok == assetState)
         {
            mMaterialName = StringTable->EmptyString();
         }
         else
         {
            mMaterialName = _in;
            mMaterialAsset = NULL;
         }
      }
      else
      {
         StringTableEntry assetId = MaterialAsset::getAssetIdByMaterialName(_in);
         if (assetId != StringTable->EmptyString())
         {
            mMaterialAssetId = assetId;
            if (MaterialAsset::getAssetById(mMaterialAssetId, &mMaterialAsset))
            {
               if (mMaterialAsset.getAssetId() != StringTable->insert("Core_Rendering:noMaterial"))
               {
                  mMaterialName = StringTable->EmptyString();
               }
               else
               {
                  mMaterialAssetId = StringTable->EmptyString();
               }
            }
         }
         else
         {
            mMaterialName = _in;
            mMaterialAssetId = StringTable->EmptyString();
            mMaterialAsset = NULL;
         }
      }
      if (!mMaterialAsset.isNull())
      {
         if (mMaterialInst && String(mMaterialAsset->getMaterialDefinitionName()).equal(mMaterialInst->getMaterial()->getName(), String::NoCase))
            return false;

         if (isClientObject())
         {
            SAFE_DELETE(mMaterialInst);

            mMaterial = mMaterialAsset->getMaterialDefinition();

            if (mMaterial)
               mMaterialInst = mMaterial->createMatInstance();
            else
               mMaterialInst = MATMGR->createMatInstance("WarningMaterial");

            if (!mMaterialInst)
               Con::errorf("classname::_initMaterial - no Material called '%s'", mMaterialAsset->getMaterialDefinitionName());

            FeatureSet features = MATMGR->getDefaultFeatures();

            mMaterialInst->init(features, getGFXVertexFormat<VertexType>());
         }

         return true;
      }
      return false;
   }

   const StringTableEntry getMaterial() const
   {
      if (mMaterialAsset && (mMaterialAsset->getMaterialDefinitionName() != StringTable->EmptyString()))
         return mMaterialAsset->getMaterialDefinitionName();
      else if (mMaterialName.isNotEmpty())
         return StringTable->insert(mMaterialName.c_str());
      else
         return StringTable->EmptyString();
   }

   //DECLARE_MATERIALASSET_NET_SETGET(GroundPlane, Material, -1);
   static bool _setMaterialName(void* obj, const char* index, const char* data)\
   {\
      bool ret = false;\
      GroundPlane* object = static_cast<GroundPlane*>(obj);\
      ret = object->_setMaterial(StringTable->insert(data));\
      if(ret)\
         object->setMaskBits(-1);\
      return ret;\
   }\
   \
   static bool _setMaterialAsset(void* obj, const char* index, const char* data)\
   {\
      bool ret = false;\
         GroundPlane* object = static_cast<GroundPlane*>(obj);\
      ret = object->_setMaterial(StringTable->insert(data));\
      if(ret)\
         object->setMaskBits(-1);\
      return ret;\
   }

   PhysicsBody *mPhysicsRep;

   /// @name Rendering State
   /// @{

   Point2F           mMin;
   Point2F           mMax;
   VertexBuffer      mVertexBuffer;
   PrimitiveBuffer   mPrimitiveBuffer;
   GFXPrimitive      mPrimitive;

   /// @}

   Convex*           mConvexList;   ///< List of collision convexes we have created; for cleanup.
};

static const F32 GROUND_PLANE_BOX_HEIGHT_HALF = 1.0f;
static const F32 GROUND_PLANE_BOX_EXTENT_HALF = 16000.0f;

inline Box3F GroundPlane::getPlaneBox()
{
   Box3F planeBox;

   planeBox.minExtents = Point3F( - GROUND_PLANE_BOX_EXTENT_HALF,
                                  - GROUND_PLANE_BOX_EXTENT_HALF,
                                  - 0.05f );
   planeBox.maxExtents = Point3F( GROUND_PLANE_BOX_EXTENT_HALF,
                                  GROUND_PLANE_BOX_EXTENT_HALF,
                                  0.05f );
   return planeBox;
}

#endif // _TORQUE_T3D_GROUNDPLANE_H_
