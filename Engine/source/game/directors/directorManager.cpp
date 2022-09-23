#include "directorManager.h"
#include "game/3D/rendering/renderMeshComponent.h"

DirectorManager* DirectorManager::smDirectorManager = nullptr;

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
      if (mDirectors[i].mTimingGroup == currentTiming)
      {
         RenderMeshDirector* renderDir = static_cast<RenderMeshDirector*>(&mDirectors[i]);
         renderDir->update();
      }
   }
}
//
//
//
Director::Director()
{

}
Director::~Director()
{

}
