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

#include "platform/platform.h"
#include "T3D/examples/physicsMeshExample.h"

#include "math/mathIO.h"
#include "scene/sceneRenderState.h"
#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "materials/materialManager.h"
#include "materials/baseMatInstance.h"
#include "renderInstance/renderPassManager.h"
#include "lighting/lightQuery.h"
#include "console/engineAPI.h"

#include "T3D/physics/physicsPlugin.h"
#include "T3D/physics/physicsCollision.h"

IMPLEMENT_CO_NETOBJECT_V1(PhysicsMeshExample);

ConsoleDocClass( PhysicsMeshExample, 
   "@brief An example scene object which renders a mesh.\n\n"
   "This class implements a basic SceneObject that can exist in the world at a "
   "3D position and render itself. There are several valid ways to render an "
   "object in Torque. This class implements the preferred rendering method which "
   "is to submit a MeshRenderInst along with a Material, vertex buffer, "
   "primitive buffer, and transform and allow the RenderMeshMgr handle the "
   "actual setup and rendering for you.\n\n"
   "See the C++ code for implementation details.\n\n"
   "@ingroup Examples\n" );


//-----------------------------------------------------------------------------
// Object setup and teardown
//-----------------------------------------------------------------------------
PhysicsMeshExample::PhysicsMeshExample()
{
   // Flag this object so that it will always
   // be sent across the network to clients
   mNetFlags.set( Ghostable | ScopeAlways );

   // Set it as a "static" object that casts shadows
   mTypeMask |= StaticObjectType | StaticShapeObjectType;

   INIT_MATERIALASSET(Material);

   mPhysicsRep = NULL;
   mMaterialInst = NULL;
}

PhysicsMeshExample::~PhysicsMeshExample()
{
   if ( mMaterialInst )
      SAFE_DELETE( mMaterialInst );
}

//-----------------------------------------------------------------------------
// Object Editing
//-----------------------------------------------------------------------------
void PhysicsMeshExample::initPersistFields()
{
   addGroup( "Rendering" );
   INITPERSISTFIELD_MATERIALASSET(Material, PhysicsMeshExample, "The material used to render the mesh.");
   endGroup( "Rendering" );

   // SceneObject already handles exposing the transform
   Parent::initPersistFields();
}

void PhysicsMeshExample::inspectPostApply()
{
   Parent::inspectPostApply();

   // Flag the network mask to send the updates
   // to the client object
   setMaskBits( UpdateMask );
}

bool PhysicsMeshExample::onAdd()
{
   if ( !Parent::onAdd() )
      return false;

   // Set up a 1x1x1 bounding box
   mObjBox.set( Point3F( -0.5f, -0.5f, -0.5f ),
                Point3F(  0.5f,  0.5f,  0.5f ) );

   resetWorldBox();

   _createPhysics();

   // Add this object to the scene
   addToScene();

   setProcessTick(true);

   // Refresh this object's material (if any)
   updateMaterial();

   return true;
}

void PhysicsMeshExample::onRemove()
{
   SAFE_DELETE(mPhysicsRep);

   // Remove this object from the scene
   removeFromScene();

   Parent::onRemove();
}

void PhysicsMeshExample::setTransform(const MatrixF & mat)
{
   // Let SceneObject handle all of the matrix manipulation
   Parent::setTransform( mat );

   mState.position = getPosition();
   mState.orientation.set(mat);
   mRenderState[0] = mRenderState[1] = mState;

   if (mPhysicsRep)
      mPhysicsRep->setTransform(getTransform());

   // Dirty our network mask so that the new transform gets
   // transmitted to the client object
   setMaskBits( TransformMask );
}

U32 PhysicsMeshExample::packUpdate( NetConnection *conn, U32 mask, BitStream *stream )
{
   // Allow the Parent to get a crack at writing its info
   U32 retMask = Parent::packUpdate( conn, mask, stream );

   // Write our transform information
   if ( stream->writeFlag( mask & TransformMask ) )
   {
      stream->writeCompressedPoint(mState.position);
      stream->writeQuat(mState.orientation, 9);
      if (!stream->writeFlag(mState.sleeping))
      {
         stream->writeVector(mState.linVelocity, 1000.0f, 16, 9);
         stream->writeVector(mState.angVelocity, 10.0f, 10, 9);
      }
   }

   // Write out any of the updated editable properties
   if (stream->writeFlag(mask & UpdateMask))
   {
      PACK_MATERIALASSET(conn, Material);
   }

   return retMask;
}

void PhysicsMeshExample::unpackUpdate(NetConnection *conn, BitStream *stream)
{
   // Let the Parent read any info it sent
   Parent::unpackUpdate(conn, stream);

   if ( stream->readFlag() )  // TransformMask
   {
      PhysicsState state;
      stream->readCompressedPoint(&state.position);
      stream->readQuat(&state.orientation, 9);
      state.sleeping = stream->readFlag();
      if (!state.sleeping)
      {
         stream->readVector(&state.linVelocity, 1000.0f, 16, 9);
         stream->readVector(&state.angVelocity, 10.0f, 10, 9);
      }

      if (mPhysicsRep && mPhysicsRep->isDynamic())
      {
         // Set the new state on the physics object immediately.
         mPhysicsRep->applyCorrection(state.getTransform());

         mPhysicsRep->setSleeping(state.sleeping);
         if (!state.sleeping)
         {
            mPhysicsRep->setLinVelocity(state.linVelocity);
            mPhysicsRep->setAngVelocity(state.angVelocity);
         }

         mPhysicsRep->getState(&mState);
      }

      if (!mPhysicsRep || !mPhysicsRep->isDynamic())
         mState = state;

   }

   if ( stream->readFlag() )  // UpdateMask
   {
      UNPACK_MATERIALASSET(conn, Material);

      if ( isProperlyAdded() )
         updateMaterial();
   }
}

//-----------------------------------------------------------------------------
// Object Rendering
//-----------------------------------------------------------------------------
void PhysicsMeshExample::createGeometry()
{
   static const Point3F cubePoints[8] = 
   {
      Point3F( 1, -1, -1), Point3F( 1, -1,  1), Point3F( 1,  1, -1), Point3F( 1,  1,  1),
      Point3F(-1, -1, -1), Point3F(-1,  1, -1), Point3F(-1, -1,  1), Point3F(-1,  1,  1)
   };

   static const Point3F cubeNormals[6] = 
   {
      Point3F( 1,  0,  0), Point3F(-1,  0,  0), Point3F( 0,  1,  0),
      Point3F( 0, -1,  0), Point3F( 0,  0,  1), Point3F( 0,  0, -1)
   };

   static const Point2F cubeTexCoords[4] = 
   {
      Point2F( 0,  0), Point2F( 0, -1),
      Point2F( 1,  0), Point2F( 1, -1)
   };

   static const U32 cubeFaces[36][3] = 
   {
      { 3, 0, 3 }, { 0, 0, 0 }, { 1, 0, 1 },
      { 2, 0, 2 }, { 0, 0, 0 }, { 3, 0, 3 },
      { 7, 1, 1 }, { 4, 1, 2 }, { 5, 1, 0 },
      { 6, 1, 3 }, { 4, 1, 2 }, { 7, 1, 1 },
      { 3, 2, 1 }, { 5, 2, 2 }, { 2, 2, 0 },
      { 7, 2, 3 }, { 5, 2, 2 }, { 3, 2, 1 },
      { 1, 3, 3 }, { 4, 3, 0 }, { 6, 3, 1 },
      { 0, 3, 2 }, { 4, 3, 0 }, { 1, 3, 3 },
      { 3, 4, 3 }, { 6, 4, 0 }, { 7, 4, 1 },
      { 1, 4, 2 }, { 6, 4, 0 }, { 3, 4, 3 },
      { 2, 5, 1 }, { 4, 5, 2 }, { 0, 5, 0 },
      { 5, 5, 3 }, { 4, 5, 2 }, { 2, 5, 1 }
   };

   // Fill the vertex buffer
   VertexType *pVert = NULL;

   mVertexBuffer.set( GFX, 36, GFXBufferTypeStatic );
   pVert = mVertexBuffer.lock();

   Point3F halfSize = getObjBox().getExtents() * 0.5f;

   for (U32 i = 0; i < 36; i++)
   {
      const U32& vdx = cubeFaces[i][0];
      const U32& ndx = cubeFaces[i][1];
      const U32& tdx = cubeFaces[i][2];

      pVert[i].point    = cubePoints[vdx] * halfSize;
      pVert[i].normal   = cubeNormals[ndx];
      pVert[i].texCoord = cubeTexCoords[tdx];
   }

   mVertexBuffer.unlock();

   // Fill the primitive buffer
   U16 *pIdx = NULL;

   mPrimitiveBuffer.set( GFX, 36, 12, GFXBufferTypeStatic );

   mPrimitiveBuffer.lock(&pIdx);     
   
   for (U16 i = 0; i < 36; i++)
      pIdx[i] = i;

   mPrimitiveBuffer.unlock();
}

void PhysicsMeshExample::updateMaterial()
{
   if (mMaterialAsset.notNull())
   {
      if (mMaterialInst && String(mMaterialAsset->getMaterialDefinitionName()).equal(mMaterialInst->getMaterial()->getName(), String::NoCase))
         return;

      SAFE_DELETE(mMaterialInst);

      mMaterialInst = MATMGR->createMatInstance(mMaterialAsset->getMaterialDefinitionName(), getGFXVertexFormat< VertexType >());

      if (!mMaterialInst)
         Con::errorf("PhysicsMeshExample::updateMaterial - no Material called '%s'", mMaterialAsset->getMaterialDefinitionName());
   }
}

void PhysicsMeshExample::prepRenderImage( SceneRenderState *state )
{
   // Do a little prep work if needed
   if ( mVertexBuffer.isNull() )
      createGeometry();

   // If we have no material then skip out.
   if ( !mMaterialInst || !state)
      return;

   // If we don't have a material instance after the override then 
   // we can skip rendering all together.
   BaseMatInstance *matInst = state->getOverrideMaterial( mMaterialInst );
   if ( !matInst )
      return;

   // Get a handy pointer to our RenderPassmanager
   RenderPassManager *renderPass = state->getRenderPass();

   // Allocate an MeshRenderInst so that we can submit it to the RenderPassManager
   MeshRenderInst *ri = renderPass->allocInst<MeshRenderInst>();

   // Set our RenderInst as a standard mesh render
   ri->type = RenderPassManager::RIT_Mesh;

   //If our material has transparency set on this will redirect it to proper render bin
   if ( matInst->getMaterial()->isTranslucent() )
   {
      ri->type = RenderPassManager::RIT_Translucent;
      ri->translucentSort = true;
   }

   // Calculate our sorting point
   if ( state )
   {
      // Calculate our sort point manually.
      const Box3F& rBox = getRenderWorldBox();
      ri->sortDistSq = rBox.getSqDistanceToPoint( state->getCameraPosition() );      
   } 
   else 
      ri->sortDistSq = 0.0f;

   // Set up our transforms
   MatrixF objectToWorld = getRenderTransform();
   objectToWorld.scale( getScale() );

   ri->objectToWorld = renderPass->allocUniqueXform( objectToWorld );
   ri->worldToCamera = renderPass->allocSharedXform(RenderPassManager::View);
   ri->projection    = renderPass->allocSharedXform(RenderPassManager::Projection);

	// If our material needs lights then fill the RIs 
   // light vector with the best lights.
   if ( matInst->isForwardLit() )
   {
      LightQuery query;
      query.init( getWorldSphere() );
		query.getLights( ri->lights, 8 );
   }

   // Make sure we have an up-to-date backbuffer in case
   // our Material would like to make use of it
   // NOTICE: SFXBB is removed and refraction is disabled!
   //ri->backBuffTex = GFX->getSfxBackBuffer();

   // Set our Material
   ri->matInst = matInst;

   // Set up our vertex buffer and primitive buffer
   ri->vertBuff = &mVertexBuffer;
   ri->primBuff = &mPrimitiveBuffer;

   ri->prim = renderPass->allocPrim();
   ri->prim->type = GFXTriangleList;
   ri->prim->minIndex = 0;
   ri->prim->startIndex = 0;
   ri->prim->numPrimitives = 12;
   ri->prim->startVertex = 0;
   ri->prim->numVertices = 36;

   // We sort by the material then vertex buffer
   ri->defaultKey = matInst->getStateHint();
   ri->defaultKey2 = (uintptr_t)ri->vertBuff; // Not 64bit safe!

   // Submit our RenderInst to the RenderPassManager
   state->getRenderPass()->addInst( ri );
}

//
void PhysicsMeshExample::_createPhysics()
{
   SAFE_DELETE(mPhysicsRep);

   if (!PHYSICSMGR)
      return;

   F32 mass = 10;

   PhysicsCollision* colShape = PHYSICSMGR->createCollision();
   colShape->addBox(mObjBox.getExtents() * 0.5f, MatrixF::Identity);

   PhysicsWorld* world = PHYSICSMGR->getWorld(isServerObject() ? "server" : "client");
   mPhysicsRep = PHYSICSMGR->createBody();
   mPhysicsRep->init(colShape, mass, PhysicsBody::BF_DYNAMIC, this, world);
   mPhysicsRep->setTransform(getTransform());
}
//

void PhysicsMeshExample::processTick(const Move* move)
{
   Parent::processTick(move);

   if (isMounted())
      return;

   if (!mPhysicsRep->isDynamic())
      return;

   // Store the last render state.
   mRenderState[0] = mRenderState[1];

   // If the last render state doesn't match the last simulation 
   // state then we got a correction and need to 
   Point3F errorDelta = mRenderState[1].position - mState.position;
   const bool doSmoothing = !errorDelta.isZero();

   const bool wasSleeping = mState.sleeping;

   // Get the new physics state.
   if (mPhysicsRep)
   {
      mPhysicsRep->getState(&mState);
   }
   else
   {
      /// put mdelta here i guess?
   }

   mRenderState[1] = mState;
   if (doSmoothing)
   {
      F32 correction = mClampF(errorDelta.len() / 20.0f, 0.1f, 0.9f);
      mRenderState[1].position.interpolate(mState.position, mRenderState[0].position, correction);
      mRenderState[1].orientation.interpolate(mState.orientation, mRenderState[0].orientation, correction);
   }

   if (!wasSleeping || !mState.sleeping)
   {
      Parent::setTransform(mState.getTransform());

      if (isServerObject() && mPhysicsRep && !PHYSICSMGR->isSinglePlayer())
         setMaskBits(TransformMask);

   }

}

void PhysicsMeshExample::interpolateTick(F32 dt)
{
   Parent::interpolateTick(dt);
   if (isMounted())
      return;

   PhysicsState state;
   state.interpolate(mRenderState[1], mRenderState[0], dt);

   // PATHSHAPE
   updateRenderChangesByParent();
   // PATHSHAPE END
}


//

DefineEngineMethod( PhysicsMeshExample, postApply, void, (),,
   "A utility method for forcing a network update.\n")
{
	object->inspectPostApply();
}
