#include <Uintah/Components/Arches/PressureSolver.h>
#include <Uintah/Components/Arches/Discretization.h>
#include <Uintah/Components/Arches/Source.h>
#include <Uintah/Components/Arches/BoundaryCondition.h>
#include <Uintah/Components/Arches/TurbulenceModel.h>
#include <Uintah/Components/Arches/PhysicalConstants.h>
#include <Uintah/Exceptions/InvalidValue.h>
#include <Uintah/Interface/Scheduler.h>
#include <Uintah/Interface/ProblemSpec.h>
#include <SCICore/Util/NotFinished.h>
#include <Uintah/Components/Arches/Arches.h>

using namespace Uintah::Arches;
using namespace std;

PressureSolver::PressureSolver()
{
}

PressureSolver::PressureSolver(TurbulenceModel* turb_model,
			       BoundaryCondition* bndry_cond,
			       PhysicalConstants* physConst)
: d_turbModel(turb_model), d_boundaryCondition(bndry_cond),
  d_physicalConsts(physConst)
{
}

PressureSolver::~PressureSolver()
{
}

void PressureSolver::problemSetup(const ProblemSpecP& params)
{
  ProblemSpecP db = params->findBlock("Pressure Solver");
  db->require("pressureReference", d_pressRef);
  string finite_diff;
  db->require("finite_difference", finite_diff);
  if (finite_diff == "Secondorder") 
    d_discretize = scinew Discretization();
  else 
    throw InvalidValue("Finite Differencing scheme "
		       "not supported: " + finite_diff, db);
  // make source and boundary_condition objects
  d_source = Source(d_turbModel, d_physicalConsts);
  string linear_sol;
  db->require("linear_solver", linear_sol);
  if (linear_sol == "RBGaussSeidel")
    d_linearSolver = scinew RBGSSolver();
  else 
    throw InvalidValue("linear solver option"
		       " not supported" + linear_sol, db);
  d_linearSolver->problemSetup(db);
}

void PressureSolver::solve(double time, double delta_t,
			   const LevelP& level,
			   SchedulerP& sched,
			   const DataWarehouseP& old_dw,
			   DataWarehouseP& new_dw)
{
  //create a new data warehouse to store matrix coeff
  // and source terms. It gets reinitialized after every 
  // pressure solve.
  DataWarehouseP matrix_dw = sched->createDataWarehouse();
  //computes stencil coefficients and source terms
  sched_buildLinearMatrix(delta_t, level, sched, new_dw, matrix_dw);
  //residual at the start of linear solve
  // this can be part of linear solver
#if 0
  calculateResidual(level, sched, new_dw, matrix_dw);
  calculateOrderMagnitude(level, sched, new_dw, matrix_dw);
#endif
  d_linearSolver->sched_pressureSolve(level, sched, new_dw, matrix_dw);
  sched_normPressure(level, sched, new_dw, new_dw);
  
}

void PressureSolver::sched_buildLinearMatrix(const LevelP& level,
					     SchedulerP& sched,
					     const DataWarehouseP& old_dw,
					     DataWarehouseP& new_dw,
					     delta_t)
{
  for(Level::const_patchIterator iter=level->patchesBegin();
      iter != level->patchesEnd(); iter++){
    const Patch* patch=*iter;
    {
      Task* tsk = scinew Task("PressureSolver::BuildCoeff",
			   patch, old_dw, new_dw, this,
			   Discretization::buildLinearMatrix,
			   delta_t);
      tsk->requires(old_dw, "velocity", patch, 1,
		    FCVariable<Vector>::getTypeDescription());
      tsk->requires(old_dw, "density", patch, 1,
		    CCVariable<double>::getTypeDescription());
      tsk->requires(old_dw, "viscosity", patch, 1,
		    CCVariable<double>::getTypeDescription());
      tsk->requires(old_dw, "pressure", patch, 1,
		    CCVariable<double>::getTypeDescription());
      /// requires convection coeff because of the nodal
      // differencing
      // computes all the components of velocity
      tsk->computes(new_dw, "VelocityConvectCoeff", patch, 0,
		    FCVariable<Vector>::getTypeDescription());
      tsk->computes(new_dw, "VelocityCoeff", patch, 0,
		    FCVariable<Vector>::getTypeDescription());
      tsk->computes(new_dw, "VelLinearSource", patch, 0,
		    FCVariable<Vector>::getTypeDescription());
      tsk->computes(new_dw, "VelNonlinearSource", patch, 0,
		    FCVariable<Vector>::getTypeDescription());
      tsk->computes(new_dw, "pressureCoeff", patch, 0,
		    CCVariable<Vector>::getTypeDescription());
      tsk->computes(new_dw, "pressureLinearSource", patch, 0,
		    CCVariable<double>::getTypeDescription());
      tsk->computes(new_dw, "pressureNonlinearSource", patch, 0,
		    CCVariable<double>::getTypeDescription());
     
      sched->addTask(tsk);
    }

  }
}


void PressureSolver::buildLinearMatrix(const ProcessorContext* pc,
				       const Patch* patch,
				       const DataWarehouseP& old_dw,
				       DataWarehouseP& new_dw,
				       double delta_t)
{
  // compute all three componenets of velocity stencil coefficients
  for(int index = 1; index <= NDIM; ++index) {
    d_discretize->calculateVelocityCoeff(pc, patch, old_dw,
					 new_dw,delta_t, index);
    d_source->calculateVelocitySource(pc, patch, old_dw,
				      new_dw,delta_t, index);
    d_boundaryCondition->velocityBC(pc, patch, old_dw,
				    new_dw,delta_t, index);
    // similar to mascal
    d_source->modifyMassSource(pc, patch, old_dw,
			       new_dw,delta_t, index);
    d_discretize->calculateVelDiagonal(pc, patch, old_dw,
				       new_dw,delta_t, index);
  }
  d_discretize->calculatePressureCoeff(pc, patch, old_dw,
				       new_dw,delta_t);
  d_source->calculatePressureSource(pc, patch, old_dw,
				    new_dw,delta_t);
  d_boundaryCondition->pressureBC(pc, patch, old_dw,
				  new_dw,delta_t);
  d_discretize->calculatePressDiagonal(pc, patch, old_dw,
				       new_dw);

}



