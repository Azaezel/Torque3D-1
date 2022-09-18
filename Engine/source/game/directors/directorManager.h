#pragma once
#ifndef _SIM_H_
#include "console/sim.h"
#endif

class Director
{
protected:
   U32 mTimingGroup;

public:
   Director();
   ~Director();

   virtual void update();
};

class DirectorManager
{
public:
   static DirectorManager* get() { return smDirectorManager; }

   enum {
      Rendering = 0,
      PreSim,
      Sim,
      PostSim
   } TimingGroup;

private:
   /// @name Device management variables
   /// @{
   static DirectorManager* smDirectorManager; ///< Global GFXDevice

   Vector<Director> mDirectors;

public:
   DirectorManager();
   ~DirectorManager();

   void Update();
};
