//----- Properties.h --------------------------------------------------

#ifndef Uintah_Component_Arches_Properties_h
#define Uintah_Component_Arches_Properties_h

/***************************************************************************
CLASS
    Properties
       Sets up the Properties ????
       
GENERAL INFORMATION
    Properties.h - Declaration of Properties class

    Author: Rajesh Rawat (rawat@crsim.utah.edu)
    
    Creation Date : 05-30-2000

    C-SAFE
    
    Copyright U of U 2000

KEYWORDS
    
DESCRIPTION

PATTERNS
    None

WARNINGS
    None

POSSIBLE REVISIONS
    None
***************************************************************************/

#include <Packages/Uintah/CCA/Components/Arches/ArchesLabel.h>
#include <Packages/Uintah/CCA/Components/Arches/BoundaryCondition.h>
#include <Packages/Uintah/Core/Parallel/UintahParallelComponent.h>
#include <Packages/Uintah/CCA/Ports/CFDInterface.h>
#include <Packages/Uintah/Core/Grid/Patch.h>
#include <Packages/Uintah/Core/Grid/VarLabel.h>
#include <Core/Geometry/IntVector.h>

#include <vector>

namespace Uintah {
class Properties {

public:

      // GROUP: Constructors:
      ///////////////////////////////////////////////////////////////////////
      // Constructor taking
      //   [in] 
      Properties(const ArchesLabel* label);

      // GROUP: Destructors :
      ///////////////////////////////////////////////////////////////////////
      // Destructor
      ~Properties();

      // GROUP: Problem Setup :
      ///////////////////////////////////////////////////////////////////////
      // Set up the problem specification database
      void problemSetup(const ProblemSpecP& params);

      // GROUP: Compute properties 
      ///////////////////////////////////////////////////////////////////////
      // Compute properties for inlet/outlet streams
      double computeInletProperties(const std::vector<double>&
				    mixfractionStream);

      // GROUP: Schedule Action :
      ///////////////////////////////////////////////////////////////////////
      // Schedule the computation of proprties
      void sched_computeProps(const LevelP& level,
			      SchedulerP&, 
			      DataWarehouseP& old_dw,
			      DataWarehouseP& new_dw);

      ///////////////////////////////////////////////////////////////////////
      // Schedule the recomputation of proprties
      void sched_reComputeProps(const LevelP& level,
				SchedulerP&, 
				DataWarehouseP& old_dw,
				DataWarehouseP& new_dw);

      // GROUP: Get Methods :
      ///////////////////////////////////////////////////////////////////////
      // Get the number of mixing variables
      inline int getNumMixVars() const{ 
	return d_numMixingVars; 
      }

      // GROUP: Set Methods :
      ///////////////////////////////////////////////////////////////////////
      // Set the boundary consition pointer
      inline void setBC(const BoundaryCondition* bc) {
	d_bc = bc;
      }

protected :

private:

      // GROUP: Actual Action Methods :
      ///////////////////////////////////////////////////////////////////////
      // Carry out actual computation of properties
      void computeProps(const ProcessorGroup*,
			const Patch* patch,
			DataWarehouseP& old_dw,
			DataWarehouseP& new_dw);

      ///////////////////////////////////////////////////////////////////////
      // Carry out actual recomputation of properties
      void reComputeProps(const ProcessorGroup*,
			const Patch* patch,
			DataWarehouseP& old_dw,
			DataWarehouseP& new_dw);

      // GROUP: Constructors Not Instantiated:
      ///////////////////////////////////////////////////////////////////////
      // Copy Constructor (never instantiated)
      //   [in] 
      //        const Properties&   
      Properties(const Properties&);

      // GROUP: Operators Not Instantiated:
      ///////////////////////////////////////////////////////////////////////
      // Assignment Operator (never instantiated)
      //   [in] 
      //        const Properties&   
      Properties& operator=(const Properties&);

private:

      int d_numMixingVars;
      double d_denUnderrelax;
      IntVector d_denRef;
      struct Stream {
	double d_density;
	double d_temperature;
	Stream();
	void problemSetup(ProblemSpecP&);
      };
      std::vector<Stream> d_streams; 
      const BoundaryCondition* d_bc;

      // Variable labels used by simulation controller
      const ArchesLabel* d_lab;
}; // end class Properties
} // End namespace Uintah


#endif

