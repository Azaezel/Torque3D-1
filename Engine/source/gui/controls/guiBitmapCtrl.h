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

#ifndef _GUIBITMAPCTRL_H_
#define _GUIBITMAPCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/core/guiControl.h"
#endif

#include "T3D/assets/ImageAsset.h"
/// Renders a bitmap.
class GuiBitmapCtrl : public GuiControl
{
   public:
   
      typedef GuiControl Parent;

   protected:
   
      /// Name of the bitmap file.  If this is 'texhandle' the bitmap is not loaded
      /// from a file but rather set explicitly on the control.
      //DECLARE_IMAGEASSET(GuiBitmapCtrl, Bitmap, GFXDefaultGUIProfile);
public: 
   GFXTexHandle mBitmap = NULL; 
   FileName mBitmapFilename = String::EmptyString; 
   StringTableEntry mBitmapAssetId = StringTable->EmptyString();
   AssetPtr<ImageAsset>  mBitmapAsset = NULL;
   GFXTextureProfile * mBitmapProfile = &GFXDefaultGUIProfile;
public: \
   const StringTableEntry getBitmapFile() const { return StringTable->insert(mBitmapFilename.c_str()); }
   void setBitmapFile(const FileName &_in) { mBitmapFilename = _in;}
   const AssetPtr<ImageAsset> & getBitmapAsset() const { return mBitmapAsset; }
   void setBitmapAsset(const AssetPtr<ImageAsset> &_in) { mBitmapAsset = _in;}
   
   bool _setBitmap(StringTableEntry _in)
   {
      if (_in == StringTable->EmptyString())
      {
         mBitmapFilename = String::EmptyString;
         mBitmapAssetId = StringTable->EmptyString();
         mBitmapAsset = NULL;
         mBitmap.free();
         mBitmap = NULL;
         return true;
      }
      
      if (AssetDatabase.isDeclaredAsset(_in))
      {
         mBitmapAssetId = _in;
         
         U32 assetState = ImageAsset::getAssetById(mBitmapAssetId, &mBitmapAsset);
         
         if (ImageAsset::Ok == assetState)
         {
            mBitmapFilename = StringTable->EmptyString();
         }
         else
         {
            mBitmapFilename = _in;
            mBitmapAsset = NULL;
         }
      }
      else
      {
         Torque::Path imagePath = _in;
         if (imagePath.getExtension() == String::EmptyString)
         {
            if (Platform::isFile(imagePath.getFullPath() + ".png"))
               imagePath.setExtension("png");
            else if (Platform::isFile(imagePath.getFullPath() + ".dds"))
               imagePath.setExtension("dds");
            else if (Platform::isFile(imagePath.getFullPath() + ".jpg"))
               imagePath.setExtension("jpg");
         }
         if (ImageAsset::getAssetByFilename(StringTable->insert(imagePath.getFullPath()), &mBitmapAsset))
         {
            mBitmapAssetId = mBitmapAsset.getAssetId();
            
            if (ImageAsset::Ok == mBitmapAsset->getStatus())
               mBitmapFilename = StringTable->EmptyString();
         }
         else
         {
            mBitmapFilename = _in;
            mBitmapAssetId = StringTable->EmptyString();
            mBitmapAsset = NULL;
         }
      }
      if (getBitmap() != StringTable->EmptyString() && !mBitmapFilename.equal("texhandle", String::NoCase))
      {
         mBitmap.set(getBitmap(), mBitmapProfile, avar("%s() - mTextureObject (line %d)", __FUNCTION__, __LINE__));
         return true;
      }
      return false;
   }
   
   const StringTableEntry getBitmap() const
   {
      if (mBitmapAsset && (mBitmapAsset->getImageFileName() != StringTable->EmptyString()))
         return  Platform::makeRelativePathName(mBitmapAsset->getImagePath(), Platform::getMainDotCsDir());
      else if (mBitmapFilename.isNotEmpty())
         return StringTable->insert(Platform::makeRelativePathName(mBitmapFilename.c_str(), Platform::getMainDotCsDir()));
      else\
         return StringTable->EmptyString();
   }

      DECLARE_IMAGEASSET_SETGET(GuiBitmapCtrl, Bitmap);
            
      Point2I mStartPoint;
      ColorI   mColor;
      
      /// If true, bitmap tiles inside control.  Otherwise stretches.
      bool mWrap;

      static bool setBitmapName( void *object, const char *index, const char *data );
      static const char *getBitmapName( void *obj, const char *data );

   public:
      
      GuiBitmapCtrl();
      static void initPersistFields();

      void setBitmap(const char *name,bool resize = false);
      void setBitmapHandle(GFXTexHandle handle, bool resize = false);

      // GuiControl.
      bool onWake();
      void onSleep();
      void inspectPostApply();

      void updateSizing();

      void onRender(Point2I offset, const RectI &updateRect);
      void setValue(S32 x, S32 y);

      DECLARE_CONOBJECT( GuiBitmapCtrl );
      DECLARE_CATEGORY( "Gui Images" );
      DECLARE_DESCRIPTION( "A control that displays a single, static image from a file.n"
                           "The bitmap can either be tiled or stretched inside the control." );
};

#endif
