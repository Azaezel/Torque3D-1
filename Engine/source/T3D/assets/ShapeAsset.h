//-----------------------------------------------------------------------------
// Copyright (c) 2013 GarageGames, LLC
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
#ifndef SHAPE_ASSET_H
#define SHAPE_ASSET_H

#ifndef _ASSET_BASE_H_
#include "assets/assetBase.h"
#endif

#ifndef _ASSET_DEFINITION_H_
#include "assets/assetDefinition.h"
#endif

#ifndef _STRINGUNIT_H_
#include "string/stringUnit.h"
#endif

#ifndef _ASSET_FIELD_TYPES_H_
#include "assets/assetFieldTypes.h"
#endif

#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif
#ifndef __RESOURCE_H__
#include "core/resource.h"
#endif
#ifndef _ASSET_PTR_H_
#include "assets/assetPtr.h"
#endif 
#ifndef MATERIALASSET_H
#include "MaterialAsset.h"
#endif
#ifndef SHAPE_ANIMATION_ASSET_H
#include "ShapeAnimationAsset.h"
#endif

#ifdef TORQUE_TOOLS
#include "gui/editor/guiInspectorTypes.h"
#endif
#ifndef _BITSTREAM_H_
#include "core/stream/bitStream.h"
#endif
//-----------------------------------------------------------------------------
class ShapeAsset : public AssetBase
{
   typedef AssetBase Parent;

protected:
   StringTableEntry   mFileName;
   StringTableEntry   mConstructorFileName;
   StringTableEntry   mFilePath;
   StringTableEntry   mConstructorFilePath;
   Resource<TSShape>	 mShape;

   //Material assets we're dependent on and use
   Vector<StringTableEntry> mMaterialAssetIds;
   Vector<AssetPtr<MaterialAsset>> mMaterialAssets;

   //Animation assets we're dependent on and use
   Vector<StringTableEntry> mAnimationAssetIds;
   Vector<AssetPtr<ShapeAnimationAsset>> mAnimationAssets;

   typedef Signal<void()> ShapeAssetChanged;

   ShapeAssetChanged mChangeSignal;

public:
   enum ShapeAssetErrCode
   {
      TooManyVerts = AssetErrCode::Extended,
      TooManyBones,
      MissingAnimatons,
      Extended
   };

   static const String mErrCodeStrings[ShapeAssetErrCode::Extended - Parent::Extended + 1];

   static U32 getAssetErrCode(AssetPtr<ShapeAsset> shapeAsset) { if (shapeAsset) return shapeAsset->mLoadedState; else return 0; }

   static String getAssetErrstrn(U32 errCode)
   {
      if (errCode < Parent::Extended) return Parent::getAssetErrstrn(errCode);
      if (errCode > ShapeAssetErrCode::Extended) return "undefined error";
      return mErrCodeStrings[errCode];
   };

   ShapeAsset();
   virtual ~ShapeAsset();

   /// Engine.
   static void initPersistFields();
   virtual void copyTo(SimObject* object);

   virtual void setDataField(StringTableEntry slotName, StringTableEntry array, StringTableEntry value);

   virtual void initializeAsset();

   /// Declare Console Object.
   DECLARE_CONOBJECT(ShapeAsset);

   bool loadShape();

   TSShape* getShape() { return mShape; }

   Resource<TSShape> getShapeResource() { return mShape; }

   void SplitSequencePathAndName(String& srcPath, String& srcName);
   StringTableEntry getShapeFileName() { return mFileName; }
   StringTableEntry getShapePath() { return mFilePath; }
   
   U32 getShapeFilenameHash() { return _StringTable::hashString(mFilePath); }

   Vector<AssetPtr<MaterialAsset>> getMaterialAssets() { return mMaterialAssets; }

   inline AssetPtr<MaterialAsset> getMaterialAsset(U32 matId) 
   { 
      if(matId >= mMaterialAssets.size()) 
          return nullptr; 
      else 
         return mMaterialAssets[matId]; 
   }

   void clearMaterialAssets() { mMaterialAssets.clear(); }

   void addMaterialAssets(AssetPtr<MaterialAsset> matPtr) { mMaterialAssets.push_back(matPtr); }

   S32 getMaterialCount() { return mMaterialAssets.size(); }
   S32 getAnimationCount() { return mAnimationAssets.size(); }
   ShapeAnimationAsset* getAnimation(S32 index);

   void _onResourceChanged(const Torque::Path &path);

   ShapeAssetChanged& getChangedSignal() { return mChangeSignal; }

   void                    setShapeFile(const char* pScriptFile);
   inline StringTableEntry getShapeFile(void) const { return mFileName; };

   void                    setShapeConstructorFile(const char* pScriptFile);
   inline StringTableEntry getShapeConstructorFile(void) const { return mConstructorFileName; };

   inline StringTableEntry getShapeFilePath(void) const { return mFilePath; };
   inline StringTableEntry getShapeConstructorFilePath(void) const { return mConstructorFilePath; };

   static bool getAssetByFilename(StringTableEntry fileName, AssetPtr<ShapeAsset>* shapeAsset);

   static StringTableEntry getAssetIdByFilename(StringTableEntry fileName);
   static U32 getAssetById(StringTableEntry assetId, AssetPtr<ShapeAsset>* shapeAsset);

   static StringTableEntry getNoShapeAssetId() { return StringTable->insert("Core_Rendering:noshape"); }

#ifdef TORQUE_TOOLS
   const char* generateCachedPreviewImage(S32 resolution);
#endif

protected:
   virtual void            onAssetRefresh(void);

   static bool setShapeFile(void *obj, StringTableEntry index, StringTableEntry data) { static_cast<ShapeAsset*>(obj)->setShapeFile(data); return false; }
   static const char* getShapeFile(void* obj, const char* data) { return static_cast<ShapeAsset*>(obj)->getShapeFile(); }

   static bool setShapeConstructorFile(void* obj, const char* index, const char* data) { static_cast<ShapeAsset*>(obj)->setShapeConstructorFile(data); return false; }
   static const char* getShapeConstructorFile(void* obj, const char* data) { return static_cast<ShapeAsset*>(obj)->getShapeConstructorFile(); }

};

#ifdef TORQUE_TOOLS
DefineConsoleType(TypeShapeAssetPtr, S32)
DefineConsoleType(TypeShapeAssetId, String)

//-----------------------------------------------------------------------------
// TypeAssetId GuiInspectorField Class
//-----------------------------------------------------------------------------
class GuiInspectorTypeShapeAssetPtr : public GuiInspectorTypeFileName
{
   typedef GuiInspectorTypeFileName Parent;
public:

   GuiBitmapButtonCtrl  *mShapeEdButton;

   DECLARE_CONOBJECT(GuiInspectorTypeShapeAssetPtr);
   static void consoleInit();

   virtual GuiControl* constructEditControl();
   virtual bool updateRects();
};

class GuiInspectorTypeShapeAssetId : public GuiInspectorTypeShapeAssetPtr
{
   typedef GuiInspectorTypeShapeAssetPtr Parent;
public:

   DECLARE_CONOBJECT(GuiInspectorTypeShapeAssetId);
   static void consoleInit();
};
#endif

#define DECLARE_SHAPEASSET(className,name)\
   Resource<TSShape>m##name;\
   StringTableEntry m##name##Filename; \
   StringTableEntry m##name##AssetId;\
   AssetPtr<ShapeAsset>  m##name##Asset;\
   const StringTableEntry get##name##File() const { return StringTable->insert(m##name##Filename); }\
   void set##name(const FileName &_in) { m##name##Filename = StringTable->insert(_in.c_str());}\
   const AssetPtr<ShapeAsset> & get##name##Asset() const { return m##name##Asset; }\
   void set##name##Asset(const AssetPtr<ShapeAsset> &_in) { m##name##Asset = _in;}\
const StringTableEntry get##name() const\
{\
   if (m##name##Asset && (m##name##Asset->getShapeFileName() != StringTable->EmptyString()))\
      return  Platform::makeRelativePathName(m##name##Asset->getShapePath(), Platform::getMainDotCsDir());\
   else if (m##name##Filename != StringTable->EmptyString())\
      return StringTable->insert(Platform::makeRelativePathName(m##name##Filename, Platform::getMainDotCsDir()));\
   else\
      return StringTable->EmptyString();\
}\
static bool _set##name##Filename(void* obj, const char* index, const char* data)\
{\
   className* object = static_cast<className*>(obj);\
   \
   StringTableEntry assetId = ShapeAsset::getAssetIdByFilename(StringTable->insert(data));\
   if (assetId != StringTable->EmptyString())\
   {\
      if (object->_set##name##Asset(obj, index, assetId))\
      {\
         if (assetId == StringTable->insert("Core_Rendering:noShape"))\
         {\
            object->m##name##Filename = StringTable->insert(data);\
            object->m##name##AssetId = StringTable->EmptyString();\
            \
            return true;\
         }\
         else\
         {\
            object->m##name##AssetId = assetId;\
            object->m##name##Filename = StringTable->EmptyString();\
            \
            return false;\
         }\
      }\
   }\
   else\
   {\
      object->m##name##Asset = StringTable->EmptyString();\
   }\
   \
   return true;\
}\
\
static bool _set##name##Asset(void* obj, const char* index, const char* data)\
{\
   className* object = static_cast<className*>(obj);\
   object->m##name##AssetId = StringTable->insert(data);\
   if (ShapeAsset::getAssetById(object->m##name##AssetId, &object->m##name##Asset))\
   {\
      if (object->m##name##Asset.getAssetId() != StringTable->insert("Core_Rendering:noShape"))\
         object->m##name##Filename = StringTable->EmptyString();\
      return true;\
   }\
   return true;\
}
#define DECLARE_NET_SHAPEASSET(className, name, bitmask) public: \
   StringTableEntry m##name##Filename; \
   StringTableEntry m##name##AssetId;\
   AssetPtr<ShapeAsset>  m##name##Asset;\
public: \
   void set##name(const FileName &_in) { m##name##Filename = StringTable->insert(_in.c_str()); }\
   const AssetPtr<ShapeAsset> & get##name##Asset() const { return m##name##Asset; }\
   void set##name##Asset(const AssetPtr<ShapeAsset> &_in) { m##name##Asset = _in; }\
const StringTableEntry get##name() const\
{\
   if (m##name##Asset && (m##name##Asset->getShapeFileName() != StringTable->EmptyString()))\
      return  Platform::makeRelativePathName(m##name##Asset->getShapePath(), Platform::getMainDotCsDir());\
   else if (m##name##Filename != StringTable->EmptyString())\
      return StringTable->insert(Platform::makeRelativePathName(m##name##Filename, Platform::getMainDotCsDir()));\
   else\
      return StringTable->EmptyString();\
}\
static bool _set##name##Filename(void* obj, const char* index, const char* data)\
{\
   className* object = static_cast<className*>(obj);\
   \
   StringTableEntry assetId = ShapeAsset::getAssetIdByFilename(StringTable->insert(data));\
   if (assetId != StringTable->EmptyString())\
   {\
      if (object->_set##name##Asset(obj, index, assetId))\
      {\
         object->setMaskBits(bitmask);\
         if (assetId == StringTable->insert("Core_Rendering:noShape"))\
         {\
            object->m##name##Filename = StringTable->insert(data);\
            object->m##name##AssetId = StringTable->EmptyString();\
            \
            return true;\
         }\
         else\
         {\
            object->m##name##AssetId = assetId;\
            object->m##name##Filename = StringTable->EmptyString();\
            \
            return false;\
         }\
      }\
   }\
   else\
   {\
      object->m##name##Asset = StringTable->EmptyString();\
   }\
   \
   return true;\
}\
\
static bool _set##name##Asset(void* obj, const char* index, const char* data)\
{\
   className* object = static_cast<className*>(obj);\
   object->m##name##AssetId = StringTable->insert(data);\
   if (ShapeAsset::getAssetById(object->m##name##AssetId, &object->m##name##Asset))\
   {\
      if (object->m##name##Asset.getAssetId() != StringTable->insert("Core_Rendering:noShape"))\
         object->m##name##Filename = StringTable->EmptyString();\
\
      object->setMaskBits(bitmask); \
      return true;\
   }\
   return true;\
}

#define DEF_SHAPEASSET_BINDS(className,name)\
DefineEngineMethod(className, get##name, String, (), , "get name")\
{\
   return object->get##name(); \
}\
DefineEngineMethod(className, get##name##Asset, String, (), , assetText(name, asset reference))\
{\
   return object->m##name##AssetId; \
}\
DefineEngineMethod(className, set##name, String, (String map), , assetText(name,assignment. first tries asset then flat file.))\
{\
   AssetPtr<ShapeAsset> shpAsset;\
   U32 assetState = ShapeAsset::getAssetById(map, &shpAsset);\
   if (ShapeAsset::Ok == assetState)\
   {\
      object->set##name##Asset(shpAsset);\
   }\
   else\
   {\
      object->set##name(map);\
   }\
   return ShapeAsset::getAssetErrstrn(assetState).c_str();\
}

#define INIT_SHAPEASSET(name) \
   m##name##Filename = String::EmptyString; \
   m##name##AssetId = StringTable->EmptyString(); \
   m##name##Asset = NULL;

#define INITPERSISTFIELD_SHAPEASSET(name, consoleClass, docs) \
   addField(assetText(name, File), TypeShapeFilename, Offset(m##name##Filename, consoleClass), assetText(name, docs)); \
   addProtectedField(assetText(name, Asset), TypeShapeAssetId, Offset(m##name##AssetId, consoleClass), consoleClass::_set##name##Asset, & defaultProtectedGetFn, assetText(name, asset reference.));

#define CLONE_SHAPEASSET(name) \
   m##name##Filename = other.m##name##Filename;\
   m##name##AssetId = other.m##name##AssetId;\
   m##name##Asset = other.m##name##Asset;

#define AUTOCONVERT_SHAPEASSET(name)\
if (m##name##Filename != StringTable->EmptyString())\
{\
   PersistenceManager* persistMgr;\
   if (!Sim::findObject("ShapeAssetValidator", persistMgr))\
      Con::errorf("ShapeAssetValidator not found!");\
   \
   if (persistMgr && m##name##Filename != StringTable->EmptyString() && m####name##AssetId == StringTable->EmptyString())\
   {\
      persistMgr->setDirty(this);\
   }\
   if (m##name##Filename != StringTable->EmptyString())\
   {\
      Torque::Path shapePath = m##name##Filename;\
      if (shapePath.getPath() == String::EmptyString)\
      {\
         String subPath = Torque::Path(getFilename()).getPath();\
         shapePath.setPath(subPath);\
      }\
      \
      m##name##AssetId = ShapeAsset::getAssetIdByFilename(shapePath.getFullPath());\
   }\
}

#define LOAD_SHAPEASSET(name)\
if (m##name##AssetId != StringTable->EmptyString())\
{\
   S32 assetState = ShapeAsset::getAssetById(m##name##AssetId, &m##name##Asset);\
   if (assetState == ShapeAsset::Ok )\
   {\
      m##name##Filename = StringTable->EmptyString();\
   }\
   else Con::warnf("Warning: %s::LOAD_SHAPEASSET(%s)-%s", mClassName, m##name##AssetId, ShapeAsset::getAssetErrstrn(assetState).c_str());\
}

#define PACKDATA_SHAPEASSET(name)\
   if (stream->writeFlag(m##name##Asset.notNull()))\
   {\
      stream->writeString(m##name##Asset.getAssetId());\
   }\
   else\
      stream->writeString(m##name##Filename);

#define UNPACKDATA_SHAPEASSET(name)\
   if (stream->readFlag())\
   {\
      m##name##AssetId = stream->readSTString();\
   }\
   else\
      m##name##Filename = stream->readSTString();

#define PACK_SHAPEASSET(netconn, name)\
   if (stream->writeFlag(m##name##Asset.notNull()))\
   {\
      NetStringHandle assetIdStr = m##name##Asset.getAssetId();\
      netconn->packNetStringHandleU(stream, assetIdStr);\
   }\
   else\
      stream->writeString(m##name##Filename);

#define UNPACK_SHAPEASSET(netconn, name)\
   if (stream->readFlag())\
   {\
      m##name##AssetId = StringTable->insert(netconn->unpackNetStringHandleU(stream).getString());\
   }\
   else\
      m##name##Filename = stream->readSTString();

#endif
