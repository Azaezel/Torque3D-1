#pragma once
#ifndef _SIM_H_
#include "console/sim.h"
#endif

#include "game/components/componentObject.h"

class SceneRenderState;

class DirectorManager;

/// <summary>
/// A Director is an object that 'directs' a particular subset of components.
/// Per the name, the idea is that a Director will be invoked at a specific time, such as when trying to render, or update the physics sim
/// And invoke the componentInstances do their work. Directors are important because they decouple the update invoke from any core system directly
/// Simplifying the call structure, and minimize deep integrations. In particular, it means that we can work on entire sets of components(and owners) in a specific
/// set, in a specific order. And because this happens in a controlled, isolated way, it's much easier to thread a specific update workload.
/// 
/// Beyond that, Directors also do the work of determining which components and owner objects are even to be updated. Instead of components or owners
/// needing to figure out if all dependencies are met in order to work, it's on the Director to validate them, simplifying the component code and ensuring dependency
/// chain nightmares are - if not completely resolved - significantly simplfied and work 'naturally'.
/// </summary>
class Director
{
friend DirectorManager;

protected:
   /// <summary>
   /// The specific timing this Director is to be invoked at. When the Director Manager updates, it updates a whole timing group.
   /// This ensures any Directors associated with a specific phase of the engine update are done in the same block, avoiding
   /// anything being left behind or desync'd
   /// </summary>
   U32 mTimingGroup;

public:
   Director();
   ~Director();

   /// <summary>
   /// The primary update function of this Director. It will process through it's valid componentInstance set(s) and indicate to them to do their work.
   /// One can thread the workload of these updates in here, and resolve all active thread tasks before the end of the update function call to ensure
   /// the core of the engine update sequencing is still threadsafe and coherent step-to-step
   /// </summary>
   virtual void update() {};
};

/// <summary>
/// The Director Manager is a class that handles all Directors the engine uses. Handled primarily via a static instance and static function invokes,
/// the engine indicates to the Director Manager that a certain timing update event, such as "Rendering" is to be done, and the Manager will inform all
/// Directors with that timing to themselves perform their update work.
/// This keeps the core engine update sequencing simple and puts the burden of the work on the Directors, allowing the rest of the engine to be cleaner and
/// simpler to process through.
/// 
/// Directors are registered to the Manager generally at initialization time, via ConsoleInit function invokes, so the Directors are registered and ready before
/// anything in the engine actually attempts to do work.
/// </summary>
class DirectorManager
{
public:
   /// <summary>
   /// Sets up our static instance of the DirectorManager
   /// </summary>
   static void init();

   /// <summary>
   /// Shuts down the DirectorManager and clears the static instance
   /// </summary>
   static void shutdown() {
      if (smDirectorManager != nullptr)
         delete smDirectorManager;
   }

   /// <summary>
   /// Gets the DirectorManager instance. If it does not exist yet, it will be created.
   /// </summary>
   /// <returns>The DirectorManager instance</returns>
   static DirectorManager* get() {
      if (smDirectorManager == nullptr)
         smDirectorManager = new DirectorManager();

      return smDirectorManager;
   }

   /// <summary>
   /// This is an enum of all timing groups that the DirectorManager can be told to invoke. A timing Group is just a unique id that Directors can indicate they're a part of.
   /// When the Manager is told to update a timing group, it loops over all registered Directors and invokes the update of any with that timing group.
   /// </summary>
   enum TimingGroup {
      Rendering = 0,
      PreSim,
      Sim,
      PostSim
   };

private:
   /// <summary>
   /// The static instance of the DirectorManager.
   /// </summary>
   static DirectorManager* smDirectorManager;

  public:
   /// <summary>
   /// The list of all registered Directors the Director Manager is aware of.
   /// </summary>
   Vector<Director*> mDirectors;

public:
   DirectorManager();
   ~DirectorManager();

   /// <summary>
   /// The main update function. When called, a timing group value is passed in, and the Manager will call
   /// any Director marked with that timing group to update.
   /// </summary>
   /// <param name="currentTiming">The specific enum TimingGroup value to invoke updates on</param>
   void update(TimingGroup currentTiming);

   /// <summary>
   /// This is a very temporary variable to hold onto the SceneRenderState data for any Directors that do Render update work
   /// This needs to be moved to a more proper general-purpose access container, alongside stuff like culling info, camera state, etc
   /// </summary>
   static SceneRenderState* sceneRenderState;
};
