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
      if (mDirectors[i]->mTimingGroup == currentTiming)
      {
         //RenderMeshDirector* renderDir = dynamic_cast<RenderMeshDirector*>(mDirectors[i]);
         //if(renderDir)
         mDirectors[i]->update();
      }
   }
}

/*void DirectorManager::registerComponent(ComponentObject* owner, const Component& comp)
{
   for (U32 i = 0; i < mDirectors.size(); i++)
   {
      mDirectors[i].registerComponent(owner, comp);
   }
}

void DirectorManager::unregisterComponent(ComponentObject* owner, const Component& comp)
{

}*/
//
//
//
Director::Director()
{
   mTimingGroup = 0;
}
Director::~Director()
{

}
