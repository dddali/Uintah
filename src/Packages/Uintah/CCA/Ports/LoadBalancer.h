
#ifndef UINTAH_HOMEBREW_LOADBALANCER_H
#define UINTAH_HOMEBREW_LOADBALANCER_H

#include <Packages/Uintah/Core/Parallel/UintahParallelPort.h>
#include <Packages/Uintah/Core/Grid/ComputeSet.h>
#include <Packages/Uintah/Core/Grid/GridP.h>
#include <Packages/Uintah/Core/Grid/LevelP.h>
#include <Packages/Uintah/Core/Grid/SimulationStateP.h>
#include <Packages/Uintah/Core/ProblemSpec/ProblemSpecP.h>
#include <Packages/Uintah/CCA/Ports/SchedulerP.h>

#include <xercesc/util/XMLURL.hpp>

namespace Uintah {

  class Patch;
  class ProcessorGroup;
  class DetailedTasks;
  class Scheduler;
  class VarLabel;
/****************************************

CLASS
   LoadBalancer
   
   Short description...

GENERAL INFORMATION

   LoadBalancer.h

   Steven G. Parker
   Department of Computer Science
   University of Utah

   Center for the Simulation of Accidental Fires and Explosions (C-SAFE)
  
   Copyright (C) 2000 SCI Group

KEYWORDS
   Scheduler

DESCRIPTION
   Long description...
  
WARNING
  
****************************************/

  class LoadBalancer : public UintahParallelPort {
  public:
    LoadBalancer();
    virtual ~LoadBalancer();
    
    virtual void assignResources(DetailedTasks& tg) = 0;
    virtual int getPatchwiseProcessorAssignment(const Patch* patch) = 0;
    virtual int getOldProcessorAssignment(const VarLabel*,
					  const Patch* patch, const int)
      { return getPatchwiseProcessorAssignment(patch); }
    virtual bool needRecompile(double, double, const GridP&)
      { return false; }
    virtual void problemSetup(ProblemSpecP&, SimulationStateP& state) = 0;
    virtual void createNeighborhood(const GridP& grid) = 0;
    virtual bool inNeighborhood(const PatchSubset*, const MaterialSubset*) = 0;
    virtual bool inNeighborhood(const Patch*) = 0;

    virtual const PatchSet* createPerProcessorPatchSet(const LevelP& level) = 0;
    virtual void dynamicReallocation(const GridP&, const SchedulerP&) {}
    virtual void restartInitialize(ProblemSpecP&, XMLURL tsurl) {}
  private:
    LoadBalancer(const LoadBalancer&);
    LoadBalancer& operator=(const LoadBalancer&);
  };
} // End namespace Uintah
    
#endif
