//----- PicardNonlinearSolver.h -----------------------------------------------

#ifndef Uintah_Component_Arches_NonlinearSolver_h
#define Uintah_Component_Arches_NonlinearSolver_h

#include <CCA/Components/Arches/Arches.h>
#include <Core/Grid/Variables/ComputeSet.h>

/**************************************
CLASS
   NonlinearSolver
   
   Class NonlinearSolver is an abstract base class
   which defines the operations needed to implement
   a nonlinear solver for the ImplicitTimeIntegrator.

GENERAL INFORMATION
   NonlinearSolver.h - declaration of the class
   
   Author: Rajesh Rawat (rawat@crsim.utah.edu)

   All major modifications since 01.01.2004 done by:
   Stanislav Borodai(borodai@crsim.utah.edu)
      
   Creation Date:   Mar 1, 2000
   
   C-SAFE 
   
   Copyright U of U 2000

KEYWORDS


DESCRIPTION
   Class NonlinearSolver is an abstract type defining the interface
   for operations necessary for solving the nonlinear systems that
   arise during an implicit integration.

WARNING
   none
****************************************/

namespace Uintah {
  class EnthalpySolver;
class TimeIntegratorLabel;
class ExtraScalarSolver;
class NonlinearSolver {

public:

      // GROUP: Constructors:
      ////////////////////////////////////////////////////////////////////////
      // Blank constructor for NonlinearSolver.
      NonlinearSolver(const ProcessorGroup* myworld);

      // GROUP: Destructors:
      ////////////////////////////////////////////////////////////////////////
      // Virtual destructor for NonlinearSolver.
      virtual ~NonlinearSolver();


      // GROUP: Problem Setup :
      ///////////////////////////////////////////////////////////////////////
      // Interface for Set up of the problem specification database
      virtual void problemSetup(const ProblemSpecP& db) = 0;

      // GROUP: Access Functions:

      // GROUP: Access Functions
      ///////////////////////////////////////////////////////////////////////
	
	EnthalpySolver* getEnthalpySolver() const
	  {
	    return d_enthalpySolver;
	  }

      virtual void sched_interpolateFromFCToCC(SchedulerP&, const PatchSet* patches,
					       const MaterialSet* matls,
				 const TimeIntegratorLabel* timelabels) = 0;
      // GROUP: Schedule Action Computations :
      ///////////////////////////////////////////////////////////////////////
      // Interface for Solve the nonlinear system, return some error code.
      //    [in] 
      //        documentation here
      //    [out] 
      //        documentation here
      virtual int nonlinearSolve( const LevelP& level,
				  SchedulerP& sched) = 0;
  
      ///////////////////////////////////////////////////////////////////////
      // Interface for dummy Solve of the nonlinear System
      virtual int noSolve(const LevelP& level,
			  SchedulerP& sched) = 0;

      const string& getTimeIntegratorType() const
	{
	  return d_timeIntegratorType;
	}
      virtual double recomputeTimestep(double current_dt) = 0;
      
      virtual double getAdiabaticAirEnthalpy() const = 0;

      virtual bool restartableTimesteps() = 0;

      virtual void setMMS(bool doMMS) = 0;

      virtual bool getMMS() const = 0;

      virtual void setExtraProjection(bool extraProjection) = 0;

      virtual void setEKTCorrection(bool EKTCorrection) = 0;

      virtual void setCalcExtraScalars(bool calcExtraScalars) = 0;

      virtual void setExtraScalars(vector<ExtraScalarSolver*>* extraScalars) = 0;
protected:
   const ProcessorGroup* d_myworld;
   string d_timeIntegratorType;
   // enthalpy solver
   EnthalpySolver* d_enthalpySolver;
private:

}; // End class NonlinearSolver
} // End namespace Uintah

#endif


