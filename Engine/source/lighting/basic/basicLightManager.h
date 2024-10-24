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

#ifndef _BASICLIGHTMANAGER_H_
#define _BASICLIGHTMANAGER_H_

#ifndef _LIGHTMANAGER_H_
#include "lighting/lightManager.h"
#endif 
#ifndef _TDICTIONARY_H_
#include "core/util/tDictionary.h"
#endif
#ifndef _GFXSHADER_H_
#include "gfx/gfxShader.h"
#endif
#ifndef _SIMOBJECT_H_
#include "console/simObject.h"
#endif
#ifndef _TSINGLETON_H_
#include "core/util/tSingleton.h"
#endif

class AvailableSLInterfaces;
class GFXShaderConstHandle;
class RenderDeferredMgr;
class PlatformTimer;

class blTerrainSystem;

class BasicLightManager : public LightManager
{
   typedef LightManager Parent;

   // For access to protected constructor.
   friend class ManagedSingleton<BasicLightManager>;

public:

   // LightManager
   bool isCompatible() const override;
   void activate( SceneManager *sceneManager ) override;
   void deactivate() override;
   void setLightInfo(ProcessedMaterial* pmat, const Material* mat, const SceneData& sgData, const SceneRenderState *state, U32 pass, GFXShaderConstBuffer* shaderConsts) override;
   bool setTextureStage(const SceneData& sgData, const U32 currTexFlag, const U32 textureSlot, GFXShaderConstBuffer* shaderConsts, ShaderConstHandles* handles) override { return false; }

   static F32 getShadowFilterDistance() { return smProjectedShadowFilterDistance; }

protected:

   // LightManager
   void _addLightInfoEx( LightInfo *lightInfo ) override { }
   void _initLightFields() override { }

   void _onPreRender( SceneManager *sceneManger, const SceneRenderState *state );

   // These are protected because we're a singleton and
   // no one else should be creating us!
   BasicLightManager();
   virtual ~BasicLightManager();

   SimObjectPtr<RenderDeferredMgr> mDeferredRenderBin;

   struct LightingShaderConstants
   {
      bool mInit;

      GFXShaderRef mShader;

      GFXShaderConstHandle *mLightPosition;
      GFXShaderConstHandle *mLightDiffuse;
      GFXShaderConstHandle *mLightAmbient;
      GFXShaderConstHandle *mLightConfigDataSC;
      GFXShaderConstHandle *mLightSpotDir;
      GFXShaderConstHandle *mLightSpotParamsSC;

      GFXShaderConstHandle* mHasVectorLightSC;
      GFXShaderConstHandle* mVectorLightDirectionSC;
      GFXShaderConstHandle* mVectorLightColorSC;
      GFXShaderConstHandle* mVectorLightBrightnessSC;

      LightingShaderConstants();
      ~LightingShaderConstants();

      void init( GFXShader *shader );

      void _onShaderReload();
   };

   typedef Map<GFXShader*, LightingShaderConstants*> LightConstantMap;

   LightConstantMap mConstantLookup;
   GFXShaderRef mLastShader;
   LightingShaderConstants* mLastConstants;
   
   /// Statics used for light manager/projected shadow metrics.
   static U32 smActiveShadowPlugins;
   static U32 smShadowsUpdated;
   static U32 smElapsedUpdateMs;

   /// This is used to determine the distance
   /// at which the shadow filtering PostEffect
   /// will be enabled for ProjectedShadow.
   static F32 smProjectedShadowFilterDistance;

   /// A timer used for tracking update time.
   PlatformTimer *mTimer;
   
   blTerrainSystem* mTerrainSystem;

public:
   // For ManagedSingleton.
   static const char* getSingletonName() { return "BasicLightManager"; }   
};

#define BLM ManagedSingleton<BasicLightManager>::instance()

#endif // _BASICLIGHTMANAGER_H_
