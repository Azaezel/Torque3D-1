#pragma once
#include "game/directors/directorManager.h"

class RenderMeshDirector : public Director
{
public:
   RenderMeshDirector();
   ~RenderMeshDirector();

   virtual void Update();
};
