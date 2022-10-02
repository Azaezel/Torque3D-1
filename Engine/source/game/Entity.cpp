#include "Entity.h"

#include "math/mathIO.h"
#include "scene/sceneRenderState.h"
#include "core/stream/bitStream.h"
#include "materials/sceneData.h"
#include "gfx/gfxDebugEvent.h"
#include "gfx/gfxTransformSaver.h"
#include "renderInstance/renderPassManager.h"


IMPLEMENT_CO_NETOBJECT_V1(Entity);

ConsoleDocClass(Entity,
   "@brief \n" );

//-----------------------------------------------------------------------------
// Object setup and teardown
//-----------------------------------------------------------------------------
Entity::Entity()
{
   // Flag this object so that it will always
   // be sent across the network to clients
   mNetFlags.set( Ghostable | ScopeAlways );

   mTypeMask |= EntityObjectType;
}

Entity::~Entity()
{
}

//-----------------------------------------------------------------------------
// Object Editing
//-----------------------------------------------------------------------------
void Entity::initPersistFields()
{
   // SceneObject already handles exposing the transform
   Parent::initPersistFields();
}

bool Entity::onAdd()
{
   if ( !Parent::onAdd() )
      return false;

   // Set up a 1x1x1 bounding box
   mObjBox.set( Point3F( -0.5f, -0.5f, -0.5f ),
                Point3F(  0.5f,  0.5f,  0.5f ) );

   resetWorldBox();

   // Add this object to the scene
   addToScene();

   return true;
}

void Entity::onRemove()
{
   // Remove this object from the scene
   removeFromScene();

   Parent::onRemove();
}

//
//
void Entity::onPostAdd()
{
   //everything's done and added. go ahead and initialize the components
   for (U32 i = 0; i < mComponents.size(); i++)
   {
      mComponents[i]->onComponentAdd();
   }

   //Set up the networked components
   mNetworkedComponents.clear();
   for (U32 i = 0; i < mComponents.size(); i++)
   {
      if (mComponents[i]->isNetworked())
      {
         NetworkedComponent netComp;
         netComp.componentIndex = i;
         netComp.updateState = NetworkedComponent::Adding;
         netComp.updateMaskBits = -1;

         mNetworkedComponents.push_back(netComp);
      }
   }

   if (!mNetworkedComponents.empty())
   {
      setMaskBits(AddComponentsMask);
      setMaskBits(ComponentsUpdateMask);
   }

   if (isMethod("onAdd"))
      Con::executef(this, "onAdd");
}

//
//
void Entity::setTransform(const MatrixF & mat)
{
   // Let SceneObject handle all of the matrix manipulation
   Parent::setTransform( mat );

   // Dirty our network mask so that the new transform gets
   // transmitted to the client object
   setMaskBits( TransformMask );
}

U32 Entity::packUpdate( NetConnection *conn, U32 mask, BitStream *stream )
{
   U32 retMask = Parent::packUpdate(conn, mask, stream);

   if (stream->writeFlag(mask & BoundsMask))
   {
      mathWrite(*stream, mObjBox);
   }

   if (stream->writeFlag(mask & AddComponentsMask))
   {
      U32 toAddComponentCount = 0;

      for (U32 i = 0; i < mNetworkedComponents.size(); i++)
      {
         if (mNetworkedComponents[i].updateState == NetworkedComponent::Adding)
         {
            toAddComponentCount++;
         }
      }

      //you reaaaaally shouldn't have >255 networked components on a single entity
      stream->writeInt(toAddComponentCount, 8);

      for (U32 i = 0; i < mNetworkedComponents.size(); i++)
      {
         if (mNetworkedComponents[i].updateState == NetworkedComponent::Adding)
         {
            const char* className = mComponents[mNetworkedComponents[i].componentIndex]->getClassName();
            stream->writeString(className, strlen(className));

            mNetworkedComponents[i].updateState = NetworkedComponent::Updating;
         }
      }
   }

   if (stream->writeFlag(mask & RemoveComponentsMask))
   {
      /*U32 toRemoveComponentCount = 0;

      for (U32 i = 0; i < mNetworkedComponents.size(); i++)
      {
         if (mNetworkedComponents[i].updateState == NetworkedComponent::Adding)
         {
            toRemoveComponentCount++;
         }
      }

      //you reaaaaally shouldn't have >255 networked components on a single entity
      stream->writeInt(toRemoveComponentCount, 8);

      for (U32 i = 0; i < mNetworkedComponents.size(); i++)
      {
         if (mNetworkedComponents[i].updateState == NetworkedComponent::Removing)
         {
            stream->writeInt(i, 16);
         }
      }*/

      /*for (U32 i = 0; i < mNetworkedComponents.size(); i++)
      {
         if (mNetworkedComponents[i].updateState == NetworkedComponent::UpdateState::Removing)
         {
            removeComponent(mComponents[mNetworkedComponents[i].componentIndex], true);
            mNetworkedComponents.erase(i);
            i--;

         }
      }*/
   }

   //Update our components
   if (stream->writeFlag(mask & ComponentsUpdateMask))
   {
      U32 toUpdateComponentCount = 0;

      for (U32 i = 0; i < mNetworkedComponents.size(); i++)
      {
         if (mNetworkedComponents[i].updateState == NetworkedComponent::Updating)
         {
            toUpdateComponentCount++;
         }
      }

      //you reaaaaally shouldn't have >255 networked components on a single entity
      stream->writeInt(toUpdateComponentCount, 8);

      bool forceUpdate = false;

      for (U32 i = 0; i < mNetworkedComponents.size(); i++)
      {
         if (mNetworkedComponents[i].updateState == NetworkedComponent::Updating)
         {
            stream->writeInt(i, 8);

            mNetworkedComponents[i].updateMaskBits = mComponents[mNetworkedComponents[i].componentIndex]->packUpdate(con, mNetworkedComponents[i].updateMaskBits, stream);

            if (mNetworkedComponents[i].updateMaskBits != 0)
               forceUpdate = true;
            else
               mNetworkedComponents[i].updateState = NetworkedComponent::None;
         }
      }

      //If we have leftover, we need to re-iterate our packing
      if (forceUpdate)
         setMaskBits(ComponentsUpdateMask);
   }

   /*if (stream->writeFlag(mask & NamespaceMask))
   {
      const char* name = getName();
      if (stream->writeFlag(name && name[0]))
         stream->writeString(String(name));

      if (stream->writeFlag(mSuperClassName && mSuperClassName[0]))
         stream->writeString(String(mSuperClassName));

      if (stream->writeFlag(mClassName && mClassName[0]))
         stream->writeString(String(mClassName));
   }*/

   return retMask;
}

void Entity::unpackUpdate(NetConnection *conn, BitStream *stream)
{
   Parent::unpackUpdate(con, stream);

   if (stream->readFlag())
   {
      Point3F pos;
      stream->readCompressedPoint(&pos);

      RotationF rot;
      mathRead(*stream, &rot);

      mDelta.move.unpack(stream);

      if (stream->readFlag() && isProperlyAdded())
      {
         // Determine number of ticks to warp based on the average
         // of the client and server velocities.
         Point3F cp = mDelta.pos + mDelta.posVec * mDelta.dt;
         mDelta.warpOffset = pos - cp;

         // Calc the distance covered in one tick as the average of
         // the old speed and the new speed from the server.
         VectorF vel = pos - mDelta.pos;
         F32 dt, as = vel.len() * 0.5 * TickSec;

         // Cal how many ticks it will take to cover the warp offset.
         // If it's less than what's left in the current tick, we'll just
         // warp in the remaining time.
         if (!as || (dt = mDelta.warpOffset.len() / as) > sMaxWarpTicks)
            dt = mDelta.dt + sMaxWarpTicks;
         else
            dt = (dt <= mDelta.dt) ? mDelta.dt : mCeil(dt - mDelta.dt) + mDelta.dt;

         // Adjust current frame interpolation
         if (mDelta.dt)
         {
            mDelta.pos = cp + (mDelta.warpOffset * (mDelta.dt / dt));
            mDelta.posVec = (cp - mDelta.pos) / mDelta.dt;
            QuatF cr;
            cr.interpolate(mDelta.rot[1], mDelta.rot[0], mDelta.dt);

            mDelta.rot[1].interpolate(cr, rot.asQuatF(), mDelta.dt / dt);
            mDelta.rot[0].extrapolate(mDelta.rot[1], cr, mDelta.dt);
         }

         // Calculated multi-tick warp
         mDelta.warpCount = 0;
         mDelta.warpTicks = (S32)(mFloor(dt));
         if (mDelta.warpTicks)
         {
            mDelta.warpOffset = pos - mDelta.pos;
            mDelta.warpOffset /= mDelta.warpTicks;
            mDelta.warpRot[0] = mDelta.rot[1];
            mDelta.warpRot[1] = rot.asQuatF();
         }
      }
      else
      {
         // Set the entity to the server position
         mDelta.dt = 0;
         mDelta.pos = pos;
         mDelta.posVec.set(0, 0, 0);
         mDelta.rot[1] = mDelta.rot[0] = rot.asQuatF();
         mDelta.warpCount = mDelta.warpTicks = 0;
         setTransform(pos, rot);
      }
   }

   if (stream->readFlag())
   {
      mathRead(*stream, &mObjBox);
      resetWorldBox();
   }

   //AddComponentMask
   if (stream->readFlag())
   {
      U32 addedComponentCount = stream->readInt(8);

      for (U32 i = 0; i < addedComponentCount; i++)
      {
         char className[256] = "";
         stream->readString(className);

         //Change to components, so iterate our list and create any new components
         // Well, looks like we have to create a new object.
         const char* componentType = className;

         ConsoleObject* object = ConsoleObject::create(componentType);

         // Finally, set currentNewObject to point to the new one.
         Component* newComponent = dynamic_cast<Component*>(object);

         if (newComponent)
         {
            addComponent(newComponent);
         }
      }
   }

   //RemoveComponentMask
   if (stream->readFlag())
   {

   }

   //ComponentUpdateMask
   if (stream->readFlag())
   {
      U32 updatingComponents = stream->readInt(8);

      for (U32 i = 0; i < updatingComponents; i++)
      {
         U32 updateComponentIndex = stream->readInt(8);

         ComponentInstance* comp = &mComponents[updateComponentIndex];
         comp->unpackUpdate(con, stream);
      }
   }

   /*if (stream->readFlag())
   {
      if (stream->readFlag())
      {
         char name[256];
         stream->readString(name);
         assignName(name);
      }

      if (stream->readFlag())
      {
         char superClassname[256];
         stream->readString(superClassname);
         mSuperClassName = superClassname;
      }

      if (stream->readFlag())
      {
         char classname[256];
         stream->readString(classname);
         mClassName = classname;
      }

      linkNamespaces();
   }*/
}

void Entity::setComponentNetMask(Component* comp, U32 mask)
{
   setMaskBits(Entity::ComponentsUpdateMask);

   for (U32 i = 0; i < mNetworkedComponents.size(); i++)
   {
      U32 netCompId = mComponents[mNetworkedComponents[i].componentIndex]->getId();
      U32 compId = comp->getId();

      if (netCompId == compId &&
         (mNetworkedComponents[i].updateState == NetworkedComponent::None || mNetworkedComponents[i].updateState == NetworkedComponent::Updating))
      {
         mNetworkedComponents[i].updateState = NetworkedComponent::Updating;
         mNetworkedComponents[i].updateMaskBits |= mask;

         break;
      }
   }
}

void Entity::setComponentsDirty()
{
   /*if (mToLoadComponents.empty())
      mStartComponentUpdate = true;

   //we need to build a list of behaviors that need to be pushed across the network
   for (U32 i = 0; i < mComponents.size(); i++)
   {
      // We can do this because both are in the string table
      Component *comp = mComponents[i];

      if (comp->isNetworked())
      {
         bool unique = true;
         for (U32 i = 0; i < mToLoadComponents.size(); i++)
         {
            if (mToLoadComponents[i]->getId() == comp->getId())
            {
               unique = false;
               break;
            }
         }
         if (unique)
            mToLoadComponents.push_back(comp);
      }
   }

   setMaskBits(ComponentsMask);*/
}

void Entity::setComponentDirty(Component* comp, bool forceUpdate)
{
   for (U32 i = 0; i < mComponents.size(); i++)
   {
      if (mComponents[i]->getId() == comp->getId())
      {
         mComponents[i]->setOwner(this);
         return;
      }
   }

   //if (!found)
   //   return;

   //if(mToLoadComponents.empty())
   //	mStartComponentUpdate = true;

   /*if (comp->isNetworked() || forceUpdate)
   {
      bool unique = true;
      for (U32 i = 0; i < mToLoadComponents.size(); i++)
      {
         if (mToLoadComponents[i]->getId() == comp->getId())
         {
            unique = false;
            break;
         }
      }
      if (unique)
         mToLoadComponents.push_back(comp);
   }

   setMaskBits(ComponentsMask);*/

}

//
//
//
#ifdef TORQUE_TOOLS
void Entity::onInspect(GuiInspector* inspector)
{
}

void Entity::onEndInspect()
{

}
#endif

bool Entity::addComponent(Component* component)
{
   component->addComponent(this);

   mComponents.push_back(component->createInstance(this));
   return true;
}

bool Entity::removeComponent(Component* component)
{
   for (U32 i = 0; i < mComponents.size(); i++)
   {
      if (mComponents[i].getComponentData().getId() == component->getId())
      {
         mComponents.erase(i);
         return true;
      }
   }

   component->removeComponent(this);

   return false;
}

DefineEngineMethod(Entity, addComponent, bool, (Component* toAddComponent), (nullAsType<Component*>()),
   "@brief Add a component to the entity\n\n")
{
   return object->addComponent(*toAddComponent);
}

DefineEngineMethod(Entity, removeComponent, bool, (Component* toRemoveComponent), (nullAsType<Component*>()),
   "@brief Remove a component from the entity\n")
{
   return object->removeComponent(*toRemoveComponent);
}

