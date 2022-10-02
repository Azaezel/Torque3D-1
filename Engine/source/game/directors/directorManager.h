#pragma once
#ifndef _SIM_H_
#include "console/sim.h"
#endif

class DirectorManager;

class Director
{
friend DirectorManager;

protected:
   U32 mTimingGroup;

public:
   Director();
   ~Director();

   void update() {};
};

class DirectorManager
{
public:
   static void init();

   static void shutdown() {
      if (smDirectorManager != nullptr)
         delete smDirectorManager;
   }

   static DirectorManager* get() {
      if (smDirectorManager == nullptr)
         smDirectorManager = new DirectorManager();

      return smDirectorManager;
   }

   enum TimingGroup {
      Rendering = 0,
      PreSim,
      Sim,
      PostSim
   };

   typedef Signal <void(Entity* ent, const Component& comp)> AddComponentSignal;
   typedef Signal <void(Entity* ent, const Component& comp)> RemoveComponentSignal;

private:
   /// @name Device management variables
   /// @{
   static DirectorManager* smDirectorManager; ///< Global GFXDevice

  public:
   Vector<Director> mDirectors;

public:
   DirectorManager();
   ~DirectorManager();

   void update(TimingGroup currentTiming);

   void registerComponent(Entity* ent, const Component& comp);
   void unregisterComponent(Entity* ent, const Component& comp);

   /*void addDirector(Director director)
   {
      mDirectors.push_back_unique(director);
   }

   void removeDirector(consDirector director)
   {
      mDirectors.remove(director);
   }*/
};
