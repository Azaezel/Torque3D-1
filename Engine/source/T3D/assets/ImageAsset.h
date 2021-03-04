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

#define assetText(x,suff) std::string(std::string(#x) + std::string(#suff)).c_str()

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
   const StringTableEntry get##name() const { return (m##name##Asset && m##name##Asset->getImageFileName() != StringTable->EmptyString()) ? m##name##Asset->getImagePath() : StringTable->insert(m##name##Filename.c_str()); }\
   const StringTableEntry get##name##File() const { return StringTable->insert(m##name##Filename.c_str()); }\
   void set##name(FileName _in) { m##name##Filename = _in; }\
   const AssetPtr<ImageAsset> & get##name##Asset() const { return m##name##Asset; }\
   void set##name##Asset(AssetPtr<ImageAsset>_in) { m##name##Asset = _in; }\
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
   const StringTableEntry get##name() const { return (m##name##Asset && m##name##Asset->getImageFileName() != StringTable->EmptyString()) ? m##name##Asset->getImagePath() : StringTable->insert(m##name##Filename.c_str()); }\
   void set##name(FileName _in) { m##name##Filename = _in; }\
   const AssetPtr<ImageAsset> & get##name##Asset() const { return m##name##Asset; }\
   void set##name##Asset(AssetPtr<ImageAsset>_in) { m##name##Asset = _in; }\
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

#define INIT_IMAGEASSET(name) \
   m##name##Filename = String::EmptyString; \
   m##name##AssetId = StringTable->EmptyString(); \
   m##name##Asset = NULL;

#define INITPERSISTFIELD_IMAGEASSET(name, consoleClass, docs) \
   addField(#name, TypeImageFilename, Offset(m##name##Filename, consoleClass), assetText(name, docs)); \
   addProtectedField(assetText(name, Asset), TypeImageAssetId, Offset(m##name##AssetId, consoleClass), consoleClass::_set##name##Asset, & defaultProtectedGetFn, assetText(name, asset reference.));
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
   const StringTableEntry get##name(const U32& id) const \
      { return (m##name##Asset[id] && m##name##Asset[id]->getImageFileName() != StringTable->EmptyString()) ? m##name##Asset[id]->getImagePath() : StringTable->insert(m##name##Filename[id].c_str()); }\
   void set##name(FileName _in,const U32& id) { m##name##Filename[id] = _in; }\
   const AssetPtr<ImageAsset> & get##name##Asset(const U32& id) const { return m##name##Asset[id]; }\
   void set##name##Asset(AssetPtr<ImageAsset>_in, const U32& id) { m##name##Asset[id] = _in; }\
static bool _set##name##Filename(void* obj, const char* index, const char* data)\
{\
   if (!index) return true;\
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
   if (!index) return true;\
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
   const StringTableEntry get##name(const U32& id) const \
      { return (m##name##Asset[id] && m##name##Asset[id]->getImageFileName() != StringTable->EmptyString()) ? m##name##Asset[id]->getImagePath() : StringTable->insert(m##name##Filename[id].c_str()); }\
   void set##name(FileName _in,const U32& id) { m##name##Filename[id] = _in; }\
   const AssetPtr<ImageAsset> & get##name##Asset(const U32& id) const { return m##name##Asset[id]; }\
   void set##name##Asset(AssetPtr<ImageAsset>_in, const U32& id) { m##name##Asset[id] = _in; }\
static bool _set##name##Filename(void* obj, const char* index, const char* data)\
{\
   if (!index) return true;\
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
   if (!index) return true;\
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

#define INIT_IMAGEASSET_ARRAY(name,id) \
   m##name##Filename[id] = String::EmptyString; \
   m##name##AssetId[id] = StringTable->EmptyString(); \
   m##name##Asset[id] = NULL;

#define INITPERSISTFIELD_IMAGEASSET_ARRAY(name, arraySize, consoleClass, docs) \
   addField(#name, TypeImageFilename, Offset(m##name##Filename, consoleClass), arraySize, assetText(name, docs)); \
   addProtectedField(assetText(name, Asset), TypeImageAssetId, Offset(m##name##AssetId, consoleClass), consoleClass::_set##name##Asset, & defaultProtectedGetFn, arraySize, assetText(name, asset reference.));
   /*addProtectedField(assetText(name, File), TypeImageFilename, Offset(m##name##Filename, consoleClass), consoleClass::_set##name##Filename,  & defaultProtectedGetFn, arraySize, assetText(name, docs)); \
   addProtectedField(assetText(name, Asset), TypeImageAssetId, Offset(m##name##AssetId, consoleClass), consoleClass::_set##name##Asset, & defaultProtectedGetFn, arraySize, assetText(name, asset reference.));*/

#define CLONE_IMAGEASSET_ARRAY(name, id) \
   m##name##Filename[id] = other.m##name##Filename;\
   m##name##AssetId[id] = other.m##name##AssetId;\
   m##name##Asset[id] = other.m##name##Asset;

#define AUTOCONVERT_IMAGEASSET_ARRAY(name, id)\
if (m##name##Filename[id] != String::EmptyString)\
{\
   PersistenceManager* persistMgr;\
   if (!Sim::findObject("ImageAssetValidator", persistMgr))\
      Con::errorf("ImageAssetValidator not found!");\
   \
   if (persistMgr && m##name##Filename[id] != String::EmptyString && m####name##AssetId[id] == StringTable->EmptyString())\
   {\
      persistMgr->setDirty(this);\
   }\
   if (m##name##Filename[id] != String::EmptyString)\
   {\
      Torque::Path imagePath = m##name##Filename[id];\
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
      m##name##AssetId[id] = ImageAsset::getAssetIdByFilename(imagePath.getFullPath());\
   }\
}

#define LOAD_IMAGEASSET_ARRAY(name, id)\
if (m##name##AssetId[id] != StringTable->EmptyString())\
{\
   S32 assetState = ImageAsset::getAssetById(m##name##AssetId[id], &m##name##Asset[id]);\
   if (assetState == ImageAsset::Ok)\
   {\
      m##name##Filename[id] = StringTable->EmptyString();\
   }\
   else Con::warnf("Warning: %s::LOAD_IMAGEASSET_ARRAY(%s)-%s", getName(), m##name##AssetId[id], ImageAsset::getAssetErrstrn(assetState).c_str());\
}

#define PACK_IMAGEASSET_ARRAY(netconn, name, id)\
   if (stream->writeFlag(m##name##Asset[id].notNull()))\
   {\
      NetStringHandle assetIdStr = m##name##Asset[id].getAssetId();\
      netconn->packNetStringHandleU(stream, assetIdStr);\
   }\
   else\
      stream->writeString(m##name##Filename[id]);

#define UNPACK_IMAGEASSET_ARRAY(netconn, name, id)\
   if (stream->readFlag())\
   {\
     m##name##AssetId[id] = StringTable->insert(netconn->unpackNetStringHandleU(stream).getString());\
   }\
   else\
      m##name##Filename[id] = stream->readSTString();

#endif

