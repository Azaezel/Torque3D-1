#include "directorManager.h"
#include "game/3D/rendering/renderMeshComponent.h"
#include "scene/sceneRenderState.h"

DirectorManager* DirectorManager::smDirectorManager = nullptr;
SceneRenderState* DirectorManager::sceneRenderState = nullptr;

DirectorManager::DirectorManager()
{
}

DirectorManager::~DirectorManager()
{
   mDirectors.clear();
}

void DirectorManager::init()
{
   if (smDirectorManager == nullptr)
      smDirectorManager = new DirectorManager();
}

void DirectorManager::update(TimingGroup currentTiming)
{
   for (U32 i = 0; i < mDirectors.size(); i++)
   {
      //If the director's timing group matches, we run the update
      if (mDirectors[i]->mTimingGroup == currentTiming)
      {
         mDirectors[i]->update();
      }

#ifdef TORQUE_TOOLS
      if (currentTiming == Rendering)
      {
         mDirectors[i]->debugDraw();
      }
#endif
   }

  
}

//
//
//
Director::Director()
{
   mTimingGroup = DirectorManager::TimingGroup::Sim;
}
Director::~Director()
{

}
