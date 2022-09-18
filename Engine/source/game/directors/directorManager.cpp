#include "directorManager.h"

DirectorManager* DirectorManager::smDirectorManager = nullptr;

DirectorManager::DirectorManager()
{

}

DirectorManager::~DirectorManager()
{

}

void DirectorManager::Update()
{
   for (U32 i = 0; i < mDirectors.size(); i++)
   {
      mDirectors[i].update();
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
