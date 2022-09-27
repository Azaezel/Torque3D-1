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
   // Allow the Parent to get a crack at writing its info
   U32 retMask = Parent::packUpdate( conn, mask, stream );

   // Write our transform information
   if ( stream->writeFlag( mask & TransformMask ) )
   {
      mathWrite(*stream, getTransform());
      mathWrite(*stream, getScale());
   }

   return retMask;
}

void Entity::unpackUpdate(NetConnection *conn, BitStream *stream)
{
   // Let the Parent read any info it sent
   Parent::unpackUpdate(conn, stream);

   if ( stream->readFlag() )  // TransformMask
   {
      mathRead(*stream, &mObjToWorld);
      mathRead(*stream, &mObjScale);

      setTransform( mObjToWorld );
   }
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

bool Entity::addComponent(const Component& component)
{
   mComponents.push_back(component.createInstance(this));
   return true;
}

DefineEngineMethod(Entity, addComponent, bool, (Component* toAddComponent), (nullAsType<Component*>()),
   "@brief Get if this model has this node name.\n\n")
{
   return object->addComponent(*toAddComponent);
}
