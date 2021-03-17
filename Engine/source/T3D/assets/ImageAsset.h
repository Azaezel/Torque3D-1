#pragma once
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
#ifndef IMAGE_ASSET_H
#define IMAGE_ASSET_H

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
#ifndef _ASSET_PTR_H_
#include "assets/assetPtr.h"
#endif 

#include "gfx/bitmap/gBitmap.h"
#include "gfx/gfxTextureHandle.h"

#include "gui/editor/guiInspectorTypes.h"

#include <string>

//-----------------------------------------------------------------------------
class ImageAsset : public AssetBase
{
   typedef AssetBase Parent;

public:
   /// The different types of image use cases
   enum ImageTypes
   {
      Albedo = 0,
      Normal = 1,
      ORMConfig = 2,
      GUI = 3,
      Roughness = 4,
      AO = 5,
      Metalness = 6,
      Glow = 7,
      Particle = 8,
      Decal = 9,
      Cubemap = 10,
      ImageTypeCount = 11
   };

protected:
   StringTableEntry mImageFileName;
   StringTableEntry mImagePath;

   GBitmap* mBitmap;
   //GFXTexHandle mTexture;

   bool mIsValidImage;
   bool mUseMips;
   bool mIsHDRImage;

   ImageTypes mImageType;

   HashMap<GFXTextureProfile*, GFXTexHandle> mResourceMap;

public:
   ImageAsset();
   virtual ~ImageAsset();

   /// Engine.
   static void initPersistFields();
   virtual void copyTo(SimObject* object);

   /// Declare Console Object.
   DECLARE_CONOBJECT(ImageAsset);

   void                    setImageFileName(StringTableEntry pScriptFile);
   inline StringTableEntry getImageFileName(void) const { return mImageFileName; };

   inline StringTableEntry getImagePath(void) const { return mImagePath; };

   bool isValid() { return mIsValidImage; }

   const GBitmap& getImage();
   GFXTexHandle getTexture(GFXTextureProfile* requestedProfile);

   StringTableEntry getImageInfo();

   static StringTableEntry getImageTypeNameFromType(ImageTypes type);
   static ImageTypes getImageTypeFromName(StringTableEntry name);

   void setImageType(ImageTypes type) { mImageType = type; }

   static bool getAssetByFilename(StringTableEntry fileName, AssetPtr<ImageAsset>* imageAsset);
   static StringTableEntry getAssetIdByFilename(StringTableEntry fileName);
   static U32 getAssetById(StringTableEntry assetId, AssetPtr<ImageAsset>* imageAsset);
   static U32 getAssetById(String assetId, AssetPtr<ImageAsset>* imageAsset) { return getAssetById(assetId.c_str(), imageAsset); };

protected:
   virtual void            initializeAsset(void);
   virtual void            onAssetRefresh(void);

   static bool setImageFileName(void* obj, StringTableEntry index, StringTableEntry data) { static_cast<ImageAsset*>(obj)->setImageFileName(data); return false; }
   static StringTableEntry getImageFileName(void* obj, StringTableEntry data) { return static_cast<ImageAsset*>(obj)->getImageFileName(); }

   void loadImage();
   U32 mLoadedState;
};

DefineConsoleType(TypeImageAssetPtr, ImageAsset)
DefineConsoleType(TypeImageAssetId, String)

typedef ImageAsset::ImageTypes ImageAssetType;
DefineEnumType(ImageAssetType);

class GuiInspectorTypeImageAssetPtr : public GuiInspectorTypeFileName
{
   typedef GuiInspectorTypeFileName Parent;
public:

   GuiBitmapButtonCtrl* mImageEdButton;

   DECLARE_CONOBJECT(GuiInspectorTypeImageAssetPtr);
   static void consoleInit();

   virtual GuiControl* constructEditControl();
   virtual bool updateRects();
};

class GuiInspectorTypeImageAssetId : public GuiInspectorTypeImageAssetPtr
{
   typedef GuiInspectorTypeImageAssetPtr Parent;
public:

   DECLARE_CONOBJECT(GuiInspectorTypeImageAssetId);
   static void consoleInit();
};

//Singular assets
/// <Summary>
/// Declares an image asset
/// This establishes the assetId, asset and legacy filepath fields, along with supplemental getter and setter functions
/// </Summary>
#define DECLARE_IMAGEASSET(className, name) public: \
   FileName m##name##Filename; \
   StringTableEntry m##name##AssetId;\
   AssetPtr<ImageAsset>  m##name##Asset;\
public: \
   const StringTableEntry get##name##File() const { return StringTable->insert(m##name##Filename.c_str()); }\
   void set##name(const FileName &_in) { m##name##Filename = _in;}\
   const AssetPtr<ImageAsset> & get##name##Asset() const { return m##name##Asset; }\
   void set##name##Asset(const AssetPtr<ImageAsset> &_in) { m##name##Asset = _in;}\
const StringTableEntry get##name() const\
{\
   if (m##name##Asset && (m##name##Asset->getImageFileName() != StringTable->EmptyString()))\
      return  Platform::makeRelativePathName(m##name##Asset->getImagePath(), Platform::getMainDotCsDir());\
   else if (m##name##Filename.isNotEmpty())\
      return StringTable->insert(Platform::makeRelativePathName(m##name##Filename.c_str(), Platform::getMainDotCsDir()));\
   else\
      return StringTable->EmptyString();\
}\
static bool _set##name##Filename(void* obj, const char* index, const char* data)\
{\
   className* object = static_cast<className*>(obj);\
   \
   StringTableEntry assetId = ImageAsset::getAssetIdByFilename(StringTable->insert(data));\
   if (assetId != StringTable->EmptyString())\
   {\
      if (object->_set##name##Asset(obj, index, assetId))\
      {\
         if (assetId == StringTable->insert("Core_Rendering:missingTexture"))\
         {\
            object->m##name##Filename = data;\
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
   if (ImageAsset::getAssetById(object->m##name##AssetId, &object->m##name##Asset))\
   {\
      if (object->m##name##Asset.getAssetId() != StringTable->insert("Core_Rendering:missingTexture"))\
         object->m##name##Filename = StringTable->EmptyString();\
      return true;\
   }\
   return true;\
}
#define DECLARE_NET_IMAGEASSET(className, name, bitmask) public: \
   FileName m##name##Filename; \
   StringTableEntry m##name##AssetId;\
   AssetPtr<ImageAsset>  m##name##Asset;\
public: \
   void set##name(const FileName &_in) { m##name##Filename = _in; }\
   const AssetPtr<ImageAsset> & get##name##Asset() const { return m##name##Asset; }\
   void set##name##Asset(const AssetPtr<ImageAsset> &_in) { m##name##Asset = _in; }\
const StringTableEntry get##name() const\
{\
   if (m##name##Asset && (m##name##Asset->getImageFileName() != StringTable->EmptyString()))\
      return  Platform::makeRelativePathName(m##name##Asset->getImagePath(), Platform::getMainDotCsDir());\
   else if (m##name##Filename.isNotEmpty())\
      return StringTable->insert(Platform::makeRelativePathName(m##name##Filename.c_str(), Platform::getMainDotCsDir()));\
   else\
      return StringTable->EmptyString();\
}\
static bool _set##name##Filename(void* obj, const char* index, const char* data)\
{\
   className* object = static_cast<className*>(obj);\
   \
   StringTableEntry assetId = ImageAsset::getAssetIdByFilename(StringTable->insert(data));\
   if (assetId != StringTable->EmptyString())\
   {\
      if (object->_set##name##Asset(obj, index, assetId))\
      {\
         object->setMaskBits(bitmask);\
         if (assetId == StringTable->insert("Core_Rendering:missingTexture"))\
         {\
            object->m##name##Filename = data;\
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
   if (ImageAsset::getAssetById(object->m##name##AssetId, &object->m##name##Asset))\
   {\
      if (object->m##name##Asset.getAssetId() != StringTable->insert("Core_Rendering:missingTexture"))\
         object->m##name##Filename = StringTable->EmptyString();\
\
      object->setMaskBits(bitmask); \
      return true;\
   }\
   return true;\
}

#define DEF_IMAGEASSET_BINDS(className,name)\
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
   AssetPtr<ImageAsset> imgAsset;\
   U32 assetState = ImageAsset::getAssetById(map, &imgAsset);\
   if (ImageAsset::Ok == assetState)\
   {\
      object->set##name##Asset(imgAsset);\
   }\
   else\
   {\
      object->set##name(map);\
   }\
   return ImageAsset::getAssetErrstrn(assetState).c_str();\
}

#define INIT_IMAGEASSET(name) \
   m##name##Filename = String::EmptyString; \
   m##name##AssetId = StringTable->EmptyString(); \
   m##name##Asset = NULL;

#define INITPERSISTFIELD_IMAGEASSET(name, consoleClass, docs) \
   addField(#name, TypeImageFilename, Offset(m##name##Filename, consoleClass), assetDoc(name, docs)); \
   addProtectedField(assetText(name, Asset), TypeImageAssetId, Offset(m##name##AssetId, consoleClass), consoleClass::_set##name##Asset, & defaultProtectedGetFn, assetDoc(name, asset docs.));
   /*addProtectedField(assetText(name, File), TypeImageFilename, Offset(m##name##Filename, consoleClass), consoleClass::_set##name##Filename,  & defaultProtectedGetFn, assetText(name, docs)); \
   addProtectedField(assetText(name, Asset), TypeImageAssetId, Offset(m##name##AssetId, consoleClass), consoleClass::_set##name##Asset, & defaultProtectedGetFn, assetText(name, asset reference.));*/

#define CLONE_IMAGEASSET(name) \
   m##name##Filename = other.m##name##Filename;\
   m##name##AssetId = other.m##name##AssetId;\
   m##name##Asset = other.m##name##Asset;

#define AUTOCONVERT_IMAGEASSET(name)\
if (m##name##Filename != String::EmptyString)\
{\
   PersistenceManager* persistMgr;\
   if (!Sim::findObject("ImageAssetValidator", persistMgr))\
      Con::errorf("ImageAssetValidator not found!");\
   \
   if (persistMgr && m##name##Filename != String::EmptyString && m####name##AssetId == StringTable->EmptyString())\
   {\
      persistMgr->setDirty(this);\
   }\
   if (m##name##Filename != String::EmptyString)\
   {\
      Torque::Path imagePath = m##name##Filename;\
      if (imagePath.getPath() == String::EmptyString)\
      {\
         String subPath = Torque::Path(getFilename()).getPath();\
         imagePath.setPath(subPath);\
      }\
      \
      if (imagePath.getExtension() == String::EmptyString)\
      {\
         if (Platform::isFile(imagePath.getFullPath() + ".png"))\
            imagePath.setExtension("png");\
         else if (Platform::isFile(imagePath.getFullPath() + ".dds"))\
            imagePath.setExtension("dds");\
         else if (Platform::isFile(imagePath.getFullPath() + ".jpg"))\
            imagePath.setExtension("jpg");\
      }\
      \
      m##name##AssetId = ImageAsset::getAssetIdByFilename(imagePath.getFullPath());\
   }\
}

#define LOAD_IMAGEASSET(name)\
if (m##name##AssetId != StringTable->EmptyString())\
{\
   S32 assetState = ImageAsset::getAssetById(m##name##AssetId, &m##name##Asset);\
   if (assetState == ImageAsset::Ok )\
   {\
      m##name##Filename = StringTable->EmptyString();\
   }\
   else Con::warnf("Warning: %s::LOAD_IMAGEASSET(%s)-%s", mClassName, m##name##AssetId, ImageAsset::getAssetErrstrn(assetState).c_str());\
}

#define PACK_IMAGEASSET(netconn, name)\
   if (stream->writeFlag(m##name##Asset.notNull()))\
   {\
      NetStringHandle assetIdStr = m##name##Asset.getAssetId();\
      netconn->packNetStringHandleU(stream, assetIdStr);\
   }\
   else\
      stream->writeString(m##name##Filename);

#define UNPACK_IMAGEASSET(netconn, name)\
   if (stream->readFlag())\
   {\
      m##name##AssetId = StringTable->insert(netconn->unpackNetStringHandleU(stream).getString());\
   }\
   else\
      m##name##Filename = stream->readSTString();

   //Arrayed Assets
#define DECLARE_IMAGEASSET_ARRAY(className, name, max) public: \
   FileName m##name##Filename[max]; \
   StringTableEntry m##name##AssetId[max];\
   AssetPtr<ImageAsset>  m##name##Asset[max];\
public: \
   void set##name(const FileName &_in,const U32& layer) { m##name##Filename[layer] = _in; }\
   const AssetPtr<ImageAsset> & get##name##Asset(const U32& layer) const { return m##name##Asset[layer]; }\
   void set##name##Asset(const AssetPtr<ImageAsset> &_in, const U32& layer) { m##name##Asset[layer] = _in; }\
const StringTableEntry get##name(U32 layer) const\
{\
   if (m##name##Asset[layer] && (m##name##Asset[layer]->getImageFileName() != StringTable->EmptyString()))\
      return  Platform::makeRelativePathName(m##name##Asset[layer]->getImagePath(), Platform::getMainDotCsDir());\
   else if (m##name##Filename[layer].isNotEmpty())\
      return StringTable->insert(Platform::makeRelativePathName(m##name##Filename[layer].c_str(), Platform::getMainDotCsDir()));\
   else\
      return StringTable->EmptyString();\
}\
static bool _set##name##Filename(void* obj, const char* index, const char* data)\
{\
   if (!index) return false;\
   U32 idx = dAtoi(index);\
   if (idx >= max)\
      return false;\
   \
   className* object = static_cast<className*>(obj);\
   \
   StringTableEntry assetId = ImageAsset::getAssetIdByFilename(StringTable->insert(data));\
   if (assetId != StringTable->EmptyString())\
   {\
      if (object->_set##name##Asset(obj, index, assetId))\
      {\
         if (assetId == StringTable->insert("Core_Rendering:missingTexture"))\
         {\
            object->m##name##Filename[idx] = data;\
            object->m##name##AssetId[idx] = StringTable->EmptyString();\
            \
            return true;\
         }\
         else\
         {\
            object->m##name##AssetId[idx] = assetId;\
            object->m##name##Filename[idx] = StringTable->EmptyString();\
            \
            return false;\
         }\
      }\
   }\
   else\
   {\
      object->m##name##Asset[idx] = StringTable->EmptyString();\
   }\
   \
   return true;\
}\
\
static bool _set##name##Asset(void* obj, const char* index, const char* data)\
{\
   className* object = static_cast<className*>(obj);\
   if (!index) return false;\
   U32 idx = dAtoi(index);\
   if (idx >= max)\
      return false;\
   object->m##name##AssetId[idx] = StringTable->insert(data);\
   if (ImageAsset::getAssetById(object->m##name##AssetId[idx], &object->m##name##Asset[idx]))\
   {\
      if (object->m##name##Asset[idx].getAssetId() != StringTable->insert("Core_Rendering:missingTexture"))\
      {\
         object->m##name##Filename[idx] = StringTable->EmptyString();\
      }\
      return true;\
   }\
   return true;\
}

#define DECLARE_NET_IMAGEASSET_ARRAY(className, name, max, bitmask) public: \
   FileName m##name##Filename[max]; \
   StringTableEntry m##name##AssetId[max];\
   AssetPtr<ImageAsset>  m##name##Asset[max];\
public: \
   void set##name(const FileName &_in,const U32& layer) { m##name##Filename[layer] = _in; }\
   const AssetPtr<ImageAsset> & get##name##Asset(const U32& layer) const { return m##name##Asset[layer]; }\
   void set##name##Asset(const AssetPtr<ImageAsset> &_in, const U32& layer) { m##name##Asset[layer] = _in; }\
const StringTableEntry get##name(U32 layer) const\
{\
   if (m##name##Asset[layer] && (m##name##Asset[layer]->getImageFileName() != StringTable->EmptyString()))\
      return  Platform::makeRelativePathName(m##name##Asset[layer]->getImagePath(), Platform::getMainDotCsDir());\
   else if (m##name##Filename[layer].isNotEmpty())\
      return StringTable->insert(Platform::makeRelativePathName(m##name##Filename[layer].c_str(), Platform::getMainDotCsDir()));\
   else\
      return StringTable->EmptyString();\
}\
static bool _set##name##Filename(void* obj, const char* index, const char* data)\
{\
   if (!index) return false;\
   U32 idx = dAtoi(index);\
   if (idx >= max)\
      return false;\
   \
   className* object = static_cast<className*>(obj);\
   \
   StringTableEntry assetId = ImageAsset::getAssetIdByFilename(StringTable->insert(data));\
   if (assetId != StringTable->EmptyString())\
   {\
      if (object->_set##name##Asset(obj, index, assetId))\
      {\
         object->setMaskBits(bitmask);\
         if (assetId == StringTable->insert("Core_Rendering:missingTexture"))\
         {\
            object->m##name##Filename[idx] = data;\
            object->m##name##AssetId[idx] = StringTable->EmptyString();\
            \
            return true;\
         }\
         else\
         {\
            object->m##name##AssetId[idx] = assetId;\
            object->m##name##Filename[idx] = StringTable->EmptyString();\
            \
            return false;\
         }\
      }\
   }\
   else\
   {\
      object->m##name##Asset[idx] = StringTable->EmptyString();\
   }\
   \
   return true;\
}\
\
static bool _set##name##Asset(void* obj, const char* index, const char* data)\
{\
   className* object = static_cast<className*>(obj);\
   if (!index) return false;\
   U32 idx = dAtoi(index);\
   if (idx >= max)\
      return false;\
   object->m##name##AssetId[idx] = StringTable->insert(data);\
   if (ImageAsset::getAssetById(object->m##name##AssetId[idx], &object->m##name##Asset[idx]))\
   {\
      if (object->m##name##Asset[idx].getAssetId() != StringTable->insert("Core_Rendering:missingTexture"))\
      {\
         object->m##name##Filename[idx] = StringTable->EmptyString();\
      }\
      object->setMaskBits(bitmask);\
      return true;\
   }\
   return true;\
}

#define INIT_IMAGEASSET_ARRAY(name,layer) \
   m##name##Filename[layer] = String::EmptyString; \
   m##name##AssetId[layer] = StringTable->EmptyString(); \
   m##name##Asset[layer] = NULL;

#define INITPERSISTFIELD_IMAGEASSET_ARRAY(name, arraySize, consoleClass, docs) \
   addField(#name, TypeImageFilename, Offset(m##name##Filename, consoleClass), arraySize, assetDoc(name, docs)); \
   addProtectedField(assetText(name, Asset), TypeImageAssetId, Offset(m##name##AssetId, consoleClass), consoleClass::_set##name##Asset, & defaultProtectedGetFn, arraySize, assetDoc(name, asset docs.));
   /*addProtectedField(assetText(name, File), TypeImageFilename, Offset(m##name##Filename, consoleClass), consoleClass::_set##name##Filename,  & defaultProtectedGetFn, arraySize, assetText(name, docs)); \
   addProtectedField(assetText(name, Asset), TypeImageAssetId, Offset(m##name##AssetId, consoleClass), consoleClass::_set##name##Asset, & defaultProtectedGetFn, arraySize, assetText(name, asset reference.));*/

#define CLONE_IMAGEASSET_ARRAY(name, layer) \
   m##name##Filename[layer] = other.m##name##Filename;\
   m##name##AssetId[layer] = other.m##name##AssetId;\
   m##name##Asset[layer] = other.m##name##Asset;

#define AUTOCONVERT_IMAGEASSET_ARRAY(name, layer)\
if (m##name##Filename[layer] != String::EmptyString)\
{\
   PersistenceManager* persistMgr;\
   if (!Sim::findObject("ImageAssetValidator", persistMgr))\
      Con::errorf("ImageAssetValidator not found!");\
   \
   if (persistMgr && m##name##Filename[layer] != String::EmptyString && m####name##AssetId[layer] == StringTable->EmptyString())\
   {\
      persistMgr->setDirty(this);\
   }\
   if (m##name##Filename[layer] != String::EmptyString)\
   {\
      Torque::Path imagePath = m##name##Filename[layer];\
      if (imagePath.getPath() == String::EmptyString)\
      {\
         String subPath = Torque::Path(getFilename()).getPath();\
         imagePath.setPath(subPath);\
      }\
      \
      if (imagePath.getExtension() == String::EmptyString)\
      {\
         if (Platform::isFile(imagePath.getFullPath() + ".png"))\
            imagePath.setExtension("png");\
         else if (Platform::isFile(imagePath.getFullPath() + ".dds"))\
            imagePath.setExtension("dds");\
         else if (Platform::isFile(imagePath.getFullPath() + ".jpg"))\
            imagePath.setExtension("jpg");\
      }\
      \
      m##name##AssetId[layer] = ImageAsset::getAssetIdByFilename(imagePath.getFullPath());\
   }\
}

#define LOAD_IMAGEASSET_ARRAY(name, layer)\
if (m##name##AssetId[layer] != StringTable->EmptyString())\
{\
   S32 assetState = ImageAsset::getAssetById(m##name##AssetId[layer], &m##name##Asset[layer]);\
   if (assetState == ImageAsset::Ok)\
   {\
      m##name##Filename[layer] = StringTable->EmptyString();\
   }\
   else Con::warnf("Warning: %s::LOAD_IMAGEASSET_ARRAY(%s)-%s", getName(), m##name##AssetId[layer], ImageAsset::getAssetErrstrn(assetState).c_str());\
}

#define PACK_IMAGEASSET_ARRAY(netconn, name, layer)\
   if (stream->writeFlag(m##name##Asset[layer].notNull()))\
   {\
      NetStringHandle assetIdStr = m##name##Asset[layer].getAssetId();\
      netconn->packNetStringHandleU(stream, assetIdStr);\
   }\
   else\
      stream->writeString(m##name##Filename[layer]);

#define UNPACK_IMAGEASSET_ARRAY(netconn, name, layer)\
   if (stream->readFlag())\
   {\
     m##name##AssetId[layer] = StringTable->insert(netconn->unpackNetStringHandleU(stream).getString());\
   }\
   else\
      m##name##Filename[layer] = stream->readSTString();

#define DEF_IMAGEASSET_ARRAY_BINDS(className,name)\
DefineEngineMethod(className, get##name, String, (U32 layer), , assetText(name, raw file reference))\
{\
   return object->get##name(layer); \
}\
DefineEngineMethod(className, get##name##Asset, String, (U32 layer), , assetText(name, asset reference))\
{\
   return object->m##name##AssetId[layer]; \
}\
DefineEngineMethod(className, set##name, String, (String map, U32 layer), , assetText(name,assignment. first tries asset then flat file.))\
{\
   AssetPtr<ImageAsset> imgAsset;\
   U32 assetState = ImageAsset::getAssetById(map, &imgAsset);\
   if (ImageAsset::Ok == assetState)\
   {\
      object->set##name##Asset(imgAsset, layer);\
   }\
   else\
   {\
      object->set##name(map, layer);\
   }\
   return ImageAsset::getAssetErrstrn(assetState).c_str();\
}
#endif

