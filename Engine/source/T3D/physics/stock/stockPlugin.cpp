#include "platform/platform.h"
#include "T3D/physics/stock/stockPlugin.h"

#include "T3D/physics/physicsShape.h"
/// stock physics sets.
#include "T3D/physics/stock/stockWorld.h"
#include "T3D/gameBase/gameProcess.h"
#include "core/util/tNamedFactory.h"


//AFTER_MODULE_INIT(Sim)
//{
//   NamedFactory < PhysicsPlugin>::add("Stock", &StockPlugin::create);

//}

StockPlugin::StockPlugin()
{
}

StockPlugin::~StockPlugin()
{
}

PhysicsPlugin* StockPlugin::create()
{
   return new StockPlugin();

}

void StockPlugin::destroyPlugin()
{
   // Cleanup any worlds that are still kicking.
   Map<StringNoCase, PhysicsWorld*>::Iterator iter = mPhysicsWorldLookup.begin();
   for (; iter != mPhysicsWorldLookup.end(); iter++)
   {
      iter->value->destroyWorld();
      delete iter->value;
   }
   mPhysicsWorldLookup.clear();

   delete this;
}

PhysicsCollision * StockPlugin::createCollision()
{
   return nullptr;
}

PhysicsBody * StockPlugin::createBody()
{
   return nullptr;
}

PhysicsPlayer * StockPlugin::createPlayer()
{
   return nullptr;
}

bool StockPlugin::isSimulationEnabled() const
{
   return false;
}

void StockPlugin::enableSimulation(const String & worldName, bool enable)
{
}

void StockPlugin::setTimeScale(const F32 timeScale)
{
}

const F32 StockPlugin::getTimeScale() const
{
   return F32();
}

bool StockPlugin::createWorld(const String & worldName)
{
   Map<StringNoCase, PhysicsWorld*>::Iterator iter = mPhysicsWorldLookup.find(worldName);
   PhysicsWorld *world = NULL;

   iter != mPhysicsWorldLookup.end() ? world = (*iter).value : world = NULL;
   if (world)
   {
      Con::errorf("StockPlugin::createWorld() - %s world already exists!", worldName.c_str());
      return false;
   }

   world = new StockWorld();

   if (worldName.equal(smClientWorldName, String::NoCase))
   {
      world->initWorld(false, ClientProcessList::get());
   }
   else
   {
      world->initWorld(true, ServerProcessList::get());
   }

   mPhysicsWorldLookup.insert(worldName, world);

   return world != NULL;

}

void StockPlugin::destroyWorld(const String & worldName)
{
   Map<StringNoCase, PhysicsWorld*>::Iterator iter = mPhysicsWorldLookup.find(worldName);
   if (iter == mPhysicsWorldLookup.end())
      return;

   PhysicsWorld *world = (*iter).value;
   world->destroyWorld();
   delete world;

   mPhysicsWorldLookup.erase(iter);

}

PhysicsWorld * StockPlugin::getWorld(const String & worldName) const
{
   if (mPhysicsWorldLookup.isEmpty())
      return NULL;

   Map<StringNoCase, PhysicsWorld*>::ConstIterator iter = mPhysicsWorldLookup.find(worldName);

   return iter != mPhysicsWorldLookup.end() ? (*iter).value : NULL;
}

PhysicsWorld* StockPlugin::getWorld() const
{
   if (mPhysicsWorldLookup.size() == 0)
      return NULL;

   Map<StringNoCase, PhysicsWorld*>::ConstIterator iter = mPhysicsWorldLookup.begin();
   return iter->value;
}

U32 StockPlugin::getWorldCount() const
{
   return mPhysicsWorldLookup.size();
}