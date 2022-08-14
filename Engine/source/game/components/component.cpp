#include "platform/platform.h"
#include "component.h"

#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "core/stream/bitStream.h"
#include "math/mathIO.h"

//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------

IMPLEMENT_CO_DATABLOCK_V1(Component);

ConsoleDocClass( Component,
   "@brief \n"
   "@ingroup Datablocks\n"
);

Component::Component()
{
}

bool Component::onAdd()
{
   if (!Parent::onAdd())
      return false;

   return true;
}

void Component::initPersistFields()
{
   Parent::initPersistFields();
}

//--------------------------------------------------------------------------
void Component::packData(BitStream* stream)
{
   Parent::packData(stream);
}

void Component::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);
}

ComponentInstance Component::createInstance(Entity* owner) const
{
   return ComponentInstance(*this, *owner);
}

//
//
//
ComponentInstance::ComponentInstance(const Component& componentData, const Entity& ownerEntity)
{
   mComponentData = &componentData;
   mOwner = &ownerEntity;
}

ComponentInstance::~ComponentInstance()
{
}
void ComponentInstance::update()
{
}
