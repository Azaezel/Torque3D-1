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
#ifndef STATE_MACHINE_ASSET_H
#define STATE_MACHINE_ASSET_H

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

#include "gui/editor/guiInspectorTypes.h"

//-----------------------------------------------------------------------------
class StateMachineAsset : public AssetBase
{
   typedef AssetBase Parent;

   StringTableEntry mStateMachineFile;
   StringTableEntry mStateMachinePath;

public:
   StateMachineAsset();
   virtual ~StateMachineAsset();

   /// Engine.
   static void initPersistFields();
   void copyTo(SimObject* object) override;

   /// Declare Console Object.
   DECLARE_CONOBJECT(StateMachineAsset);

   void                    setStateMachineFile(const char* pStateMachineFile);
   inline StringTableEntry getStateMachineFile(void) const { return mStateMachineFile; };

   inline StringTableEntry getStateMachinePath(void) const { return mStateMachinePath; };

protected:
   void            initializeAsset(void) override;
   void            onAssetRefresh(void) override;

   static bool setStateMachineFile(void *obj, const char *index, const char *data) { static_cast<StateMachineAsset*>(obj)->setStateMachineFile(data); return false; }
   static const char* getStateMachineFile(void* obj, const char* data) { return static_cast<StateMachineAsset*>(obj)->getStateMachineFile(); }
};

DefineConsoleType(TypeStateMachineAssetPtr, StateMachineAsset)
#ifdef TORQUE_TOOLS
//-----------------------------------------------------------------------------
// TypeAssetId GuiInspectorField Class
//-----------------------------------------------------------------------------
class GuiInspectorTypeStateMachineAssetPtr : public GuiInspectorTypeFileName
{
   typedef GuiInspectorTypeFileName Parent;
public:

   GuiBitmapButtonCtrl  *mSMEdButton;

   DECLARE_CONOBJECT(GuiInspectorTypeStateMachineAssetPtr);
   static void consoleInit();

   GuiControl* constructEditControl() override;
   bool updateRects() override;
};
#endif
#endif

