//----- PressureSolver.cc ----------------------------------------------

/* REFERENCED */
static char *id="@(#) $Id$";

#include <Uintah/Components/Arches/PressureSolver.h>
#include <Uintah/Components/Arches/CellInformationP.h>
#include <Uintah/Components/Arches/Discretization.h>
#include <Uintah/Components/Arches/Source.h>
#include <Uintah/Components/Arches/BoundaryCondition.h>
#include <Uintah/Components/Arches/TurbulenceModel.h>
#include <Uintah/Components/Arches/PhysicalConstants.h>
#include <Uintah/Components/Arches/RBGSSolver.h>
#include <Uintah/Components/Arches/PetscSolver.h>
#include <Uintah/Components/Arches/ArchesVariables.h>
#include <Uintah/Exceptions/InvalidValue.h>
#include <Uintah/Interface/Scheduler.h>
#include <Uintah/Interface/DataWarehouse.h>
#include <Uintah/Interface/ProblemSpec.h>
#include <Uintah/Grid/CellIterator.h>
#include <Uintah/Grid/Level.h>
#include <Uintah/Grid/Task.h>
#include <Uintah/Grid/CCVariable.h>
#include <Uintah/Grid/SFCXVariable.h>
#include <Uintah/Grid/SFCYVariable.h>
#include <Uintah/Grid/SFCZVariable.h>
#include <Uintah/Interface/LoadBalancer.h>
#include <Uintah/Parallel/ProcessorGroup.h>
#include <SCICore/Util/NotFinished.h>
#include <Uintah/Components/Arches/Arches.h>
#include <Uintah/Components/Arches/ArchesFort.h>
using namespace Uintah::ArchesSpace;
using namespace std;

//****************************************************************************
// Default constructor for PressureSolver
//****************************************************************************
PressureSolver::PressureSolver(const ArchesLabel* label,
			       TurbulenceModel* turb_model,
			       BoundaryCondition* bndry_cond,
			       PhysicalConstants* physConst,
			       const ProcessorGroup* myworld):
                                      d_lab(label),
                                     d_turbModel(turb_model), 
                                     d_boundaryCondition(bndry_cond),
				     d_physicalConsts(physConst),
				     d_myworld(myworld)
{
}

//****************************************************************************
// Destructor
//****************************************************************************
PressureSolver::~PressureSolver()
{
}

//****************************************************************************
// Problem Setup
//****************************************************************************
void 
PressureSolver::problemSetup(const ProblemSpecP& params)
{
  ProblemSpecP db = params->findBlock("PressureSolver");
  db->require("ref_point", d_pressRef);
  string finite_diff;
  db->require("finite_difference", finite_diff);
  if (finite_diff == "second") d_discretize = scinew Discretization();
  else {
    throw InvalidValue("Finite Differencing scheme "
		       "not supported: " + finite_diff);
  }

  // make source and boundary_condition objects
  d_source = scinew Source(d_turbModel, d_physicalConsts);

  string linear_sol;
  db->require("linear_solver", linear_sol);
  if (linear_sol == "linegs") d_linearSolver = scinew RBGSSolver();
  else if (linear_sol == "petsc") d_linearSolver = scinew PetscSolver(d_myworld);
  else {
    throw InvalidValue("Linear solver option"
		       " not supported" + linear_sol);
  }
  d_linearSolver->problemSetup(db);
}

//****************************************************************************
// Schedule solve of linearized pressure equation
//****************************************************************************
void PressureSolver::solve(const LevelP& level,
			   SchedulerP& sched,
			   DataWarehouseP& old_dw,
			   DataWarehouseP& new_dw,
			   double time, double delta_t)
{
  //computes stencil coefficients and source terms
  // require : old_dw -> pressureSPBC, densityCP, viscosityCTS, [u,v,w]VelocitySPBC
  //           new_dw -> pressureSPBC, densityCP, viscosityCTS, [u,v,w]VelocitySIVBC
  // compute : uVelConvCoefPBLM, vVelConvCoefPBLM, wVelConvCoefPBLM
  //           uVelCoefPBLM, vVelCoefPBLM, wVelCoefPBLM, uVelLinSrcPBLM
  //           vVelLinSrcPBLM, wVelLinSrcPBLM, uVelNonLinSrcPBLM 
  //           vVelNonLinSrcPBLM, wVelNonLinSrcPBLM, presCoefPBLM 
  //           presLinSrcPBLM, presNonLinSrcPBLM
  //sched_buildLinearMatrix(level, sched, new_dw, matrix_dw, delta_t);
  // build the structure and get all the old variables
  sched_buildLinearMatrix(level, sched, old_dw, new_dw, delta_t);

  //residual at the start of linear solve
  // this can be part of linear solver
#if 0
  calculateResidual(level, sched, old_dw, new_dw);
  calculateOrderMagnitude(level, sched, old_dw, new_dw);
#endif

  // Schedule the pressure solve
  // require : pressureIN, presCoefPBLM, presNonLinSrcPBLM
  // compute : presResidualPS, presCoefPS, presNonLinSrcPS, pressurePS
  //d_linearSolver->sched_pressureSolve(level, sched, new_dw, matrix_dw);
  sched_pressureLinearSolve(level, sched, old_dw, new_dw);

  // Schedule Calculation of pressure norm
  // require :
  // compute :
  //sched_normPressure(level, sched, new_dw, matrix_dw);
  
}

//****************************************************************************
// Schedule build of linear matrix
//****************************************************************************
void 
PressureSolver::sched_buildLinearMatrix(const LevelP& level,
					SchedulerP& sched,
					DataWarehouseP& old_dw,
					DataWarehouseP& new_dw,
					double delta_t)
{
  for(Level::const_patchIterator iter=level->patchesBegin();
      iter != level->patchesEnd(); iter++){
    const Patch* patch=*iter;
    {
      Task* tsk = scinew Task("PressureSolver::BuildCoeff",
			   patch, old_dw, new_dw, this,
			   &PressureSolver::buildLinearMatrix, delta_t);

      int numGhostCells = 1;
      int zeroGhostCells = 0;
      int matlIndex = 0;
      int nofStencils = 7;
      // Requires
      // from old_dw for time integration
      // get old_dw from getTop function
      tsk->requires(old_dw, d_lab->d_cellTypeLabel, matlIndex, patch, 
		    Ghost::AroundCells, numGhostCells);
      tsk->requires(old_dw, d_lab->d_densityCPLabel, matlIndex, patch, 
		    Ghost::None, zeroGhostCells);
      tsk->requires(old_dw, d_lab->d_uVelocitySPBCLabel, matlIndex, patch, 
		    Ghost::None, zeroGhostCells);
      tsk->requires(old_dw, d_lab->d_vVelocitySPBCLabel, matlIndex, patch, 
		    Ghost::None, zeroGhostCells);
      tsk->requires(old_dw, d_lab->d_wVelocitySPBCLabel, matlIndex, patch, 
		    Ghost::None, zeroGhostCells);
      // from new_dw
      tsk->requires(new_dw, d_lab->d_pressureINLabel, matlIndex, patch, 
		    Ghost::None, zeroGhostCells);
      tsk->requires(new_dw, d_lab->d_densityINLabel, matlIndex, patch, 
		    Ghost::AroundCells, numGhostCells+1);
      tsk->requires(new_dw, d_lab->d_viscosityINLabel, matlIndex, patch, 
		    Ghost::AroundCells, numGhostCells);
      tsk->requires(new_dw, d_lab->d_uVelocitySIVBCLabel, matlIndex, patch, 
		    Ghost::AroundCells, numGhostCells);
      tsk->requires(new_dw, d_lab->d_vVelocitySIVBCLabel, matlIndex, patch, 
		    Ghost::AroundCells, numGhostCells);
      tsk->requires(new_dw, d_lab->d_wVelocitySIVBCLabel, matlIndex, patch, 
		    Ghost::AroundCells, numGhostCells);

      /// requires convection coeff because of the nodal
      // differencing
      // computes all the components of velocity
      for (int ii = 0; ii < nofStencils; ii++) {
	tsk->computes(new_dw, d_lab->d_uVelCoefPBLMLabel, ii, patch);
	tsk->computes(new_dw, d_lab->d_vVelCoefPBLMLabel, ii, patch);
	tsk->computes(new_dw, d_lab->d_wVelCoefPBLMLabel, ii, patch);
      }
      tsk->computes(new_dw, d_lab->d_uVelLinSrcPBLMLabel, matlIndex, patch);
      tsk->computes(new_dw, d_lab->d_vVelLinSrcPBLMLabel, matlIndex, patch);
      tsk->computes(new_dw, d_lab->d_wVelLinSrcPBLMLabel, matlIndex, patch);
      tsk->computes(new_dw, d_lab->d_uVelNonLinSrcPBLMLabel, matlIndex, patch);
      tsk->computes(new_dw, d_lab->d_vVelNonLinSrcPBLMLabel, matlIndex, patch);
      tsk->computes(new_dw, d_lab->d_wVelNonLinSrcPBLMLabel, matlIndex, patch);
     
      sched->addTask(tsk);
    }
  }
  for(Level::const_patchIterator iter=level->patchesBegin();
      iter != level->patchesEnd(); iter++){
    const Patch* patch=*iter;
    {
      Task* tsk = scinew Task("PressureSolver::BuildCoeffPress",
			   patch, old_dw, new_dw, this,
			   &PressureSolver::buildLinearMatrixPress, delta_t);

      int numGhostCells = 1;
      int zeroGhostCells = 0;
      int matlIndex = 0;
      int nofStencils = 7;
      // Requires
      // from old_dw for time integration
      // get old_dw from getTop function
      tsk->requires(old_dw, d_lab->d_cellTypeLabel, matlIndex, patch, 
		    Ghost::AroundCells, numGhostCells);
      tsk->requires(old_dw, d_lab->d_densityCPLabel, matlIndex, patch, 
		    Ghost::None, zeroGhostCells);
      tsk->requires(old_dw, d_lab->d_uVelocitySPBCLabel, matlIndex, patch, 
		    Ghost::None, zeroGhostCells);
      tsk->requires(old_dw, d_lab->d_vVelocitySPBCLabel, matlIndex, patch, 
		    Ghost::None, zeroGhostCells);
      tsk->requires(old_dw, d_lab->d_wVelocitySPBCLabel, matlIndex, patch, 
		    Ghost::None, zeroGhostCells);
      // from new_dw
      tsk->requires(new_dw, d_lab->d_pressureINLabel, matlIndex, patch, 
		    Ghost::None, zeroGhostCells);
      tsk->requires(new_dw, d_lab->d_densityINLabel, matlIndex, patch, 
		    Ghost::AroundCells, numGhostCells);
      tsk->requires(new_dw, d_lab->d_viscosityINLabel, matlIndex, patch, 
		    Ghost::AroundCells, numGhostCells);
      tsk->requires(new_dw, d_lab->d_uVelocitySIVBCLabel, matlIndex, patch, 
		    Ghost::AroundCells, numGhostCells+1);
      tsk->requires(new_dw, d_lab->d_vVelocitySIVBCLabel, matlIndex, patch, 
		    Ghost::AroundCells, numGhostCells+1);
      tsk->requires(new_dw, d_lab->d_wVelocitySIVBCLabel, matlIndex, patch, 
		    Ghost::AroundCells, numGhostCells+1);

      /// requires convection coeff because of the nodal
      // differencing
      // computes all the components of velocity
      for (int ii = 0; ii < nofStencils; ii++) {
	tsk->requires(new_dw, d_lab->d_uVelCoefPBLMLabel, ii, patch,
		      Ghost::AroundCells, numGhostCells);
	tsk->requires(new_dw, d_lab->d_vVelCoefPBLMLabel, ii, patch,
		      Ghost::AroundCells, numGhostCells);
	tsk->requires(new_dw, d_lab->d_wVelCoefPBLMLabel, ii, patch,
		      Ghost::AroundCells, numGhostCells);
	tsk->computes(new_dw, d_lab->d_presCoefPBLMLabel, ii, patch);
      }
      tsk->requires(new_dw, d_lab->d_uVelLinSrcPBLMLabel, matlIndex, patch,
		    Ghost::AroundCells, numGhostCells);
      tsk->requires(new_dw, d_lab->d_vVelLinSrcPBLMLabel, matlIndex, patch,
		    Ghost::AroundCells, numGhostCells);
      tsk->requires(new_dw, d_lab->d_wVelLinSrcPBLMLabel, matlIndex, patch,
		    Ghost::AroundCells, numGhostCells);
      tsk->requires(new_dw, d_lab->d_uVelNonLinSrcPBLMLabel, matlIndex, patch,
		    Ghost::AroundCells, numGhostCells);
      tsk->requires(new_dw, d_lab->d_vVelNonLinSrcPBLMLabel, matlIndex, patch,
		    Ghost::AroundCells, numGhostCells);
      tsk->requires(new_dw, d_lab->d_wVelNonLinSrcPBLMLabel, matlIndex, patch,
		    Ghost::AroundCells, numGhostCells);
      tsk->computes(new_dw, d_lab->d_presNonLinSrcPBLMLabel, matlIndex, patch);
     
      sched->addTask(tsk);
    }

  }
  }


//****************************************************************************
// Schedule solver for linear matrix
//****************************************************************************
void 
PressureSolver::sched_pressureLinearSolve(const LevelP& level,
					  SchedulerP& sched,
					  DataWarehouseP& old_dw,
					  DataWarehouseP& new_dw)
{
#if 0
  for(Level::const_patchIterator iter=level->patchesBegin();
      iter != level->patchesEnd(); iter++){
    const Patch* patch=*iter;
    {
      Task* tsk = scinew Task("PressureSolver::PressLinearSolve",
			   patch, old_dw, new_dw, this,
			   &PressureSolver::pressureLinearSolve);

      int numGhostCells = 0;
      int matlIndex = 0;
      int nofStencils = 7;
      // Requires
      // coefficient for the variable for which solve is invoked
      tsk->requires(new_dw, d_lab->d_pressureINLabel, matlIndex, patch, 
		    Ghost::None, numGhostCells);
      for (int ii = 0; ii < nofStencils; ii++)
	tsk->requires(new_dw, d_lab->d_presCoefPBLMLabel, ii, patch, 
		      Ghost::None, numGhostCells);
      tsk->requires(new_dw, d_lab->d_presNonLinSrcPBLMLabel, matlIndex, patch, 
		    Ghost::None, numGhostCells);
      // computes global residual
      //      tsk->computes(new_dw, d_lab->d_presResidPSLabel, matlIndex, patch);
      //      tsk->computes(new_dw, d_lab->d_presTruncPSLabel, matlIndex, patch);
      tsk->computes(new_dw, d_lab->d_pressurePSLabel, matlIndex, patch);

      sched->addTask(tsk);
    }
  }
#else
  int numProcessors = d_myworld->size();
  vector<Task*> tasks(numProcessors, (Task*)0);
  LoadBalancer* lb = sched->getLoadBalancer();

  //cerr << "In sched_PressureLinearSolve\n";
  for(Level::const_patchIterator iter=level->patchesBegin();
      iter != level->patchesEnd(); iter++){
    const Patch* patch=*iter;
    {
       int proc = lb->getPatchwiseProcessorAssignment(patch, d_myworld);
       Task* tsk = tasks[proc];
       if(!tsk){
	  tsk = scinew Task("PressureSolver::PressLinearSolve",
			    patch, old_dw, new_dw, this,
			    &PressureSolver::pressureLinearSolve_all,
			    level, sched);
	  tasks[proc]=tsk;
       }

       int numGhostCells = 1;
       int zeroGhostCells = 0;
       int matlIndex = 0;
       int nofStencils = 7;
       // Requires
       // coefficient for the variable for which solve is invoked
       tsk->requires(new_dw, d_lab->d_pressureINLabel, matlIndex, patch, 
		     Ghost::AroundCells, numGhostCells);
       for (int ii = 0; ii < nofStencils; ii++)
	  tsk->requires(new_dw, d_lab->d_presCoefPBLMLabel, ii, patch, 
			Ghost::None, zeroGhostCells);
       tsk->requires(new_dw, d_lab->d_presNonLinSrcPBLMLabel, matlIndex, patch, 
		     Ghost::None, zeroGhostCells);
       // computes global residual
       //      tsk->computes(new_dw, d_lab->d_presResidPSLabel, matlIndex, patch);
       //      tsk->computes(new_dw, d_lab->d_presTruncPSLabel, matlIndex, patch);
       tsk->computes(new_dw, d_lab->d_pressurePSLabel, matlIndex, patch);
#ifdef ARCHES_PRES_DEBUG
       cerr << "Adding computes on patch: " << patch->getID() << '\n';
#endif

    }
  }
  for(int i=0;i<tasks.size();i++)
     if(tasks[i]){
	sched->addTask(tasks[i]);
#ifdef ARCHES_PRES_DEBUG
	cerr << "Adding task: " << *tasks[i] << '\n';
#endif
     }
  sched->releaseLoadBalancer();
#endif
}


//****************************************************************************
// Actually build of linear matrix
//****************************************************************************
void 
PressureSolver::buildLinearMatrix(const ProcessorGroup* pc,
				  const Patch* patch,
				  DataWarehouseP& old_dw,
				  DataWarehouseP& new_dw,
				  double delta_t)
{
  ArchesVariables pressureVars;
  // compute all three componenets of velocity stencil coefficients
  int matlIndex = 0;
  int numGhostCells = 1;
  int zeroGhostCells = 0;
  int nofStencils = 7;
  // Get the reference density
  // Get the required data
  new_dw->get(pressureVars.density, d_lab->d_densityINLabel, 
	      matlIndex, patch, Ghost::AroundCells, numGhostCells+1);
  new_dw->get(pressureVars.viscosity, d_lab->d_viscosityINLabel, 
	      matlIndex, patch, Ghost::AroundCells, numGhostCells);
  new_dw->get(pressureVars.pressure, d_lab->d_pressureINLabel, 
	      matlIndex, patch, Ghost::None, zeroGhostCells);
  sum_vartype den_ref_var;
  old_dw->get(den_ref_var, d_lab->d_refDensity_label);
  pressureVars.den_Ref = den_ref_var;
  //cerr << "getdensity_ref " << pressureVars.den_Ref << endl;
  // Get the PerPatch CellInformation data
  PerPatch<CellInformationP> cellInfoP;
  old_dw->get(cellInfoP, d_lab->d_cellInfoLabel, matlIndex, patch);
  //  old_dw->get(cellInfoP, d_cellInfoLabel, matlIndex, patch);
  //  if (old_dw->exists(d_cellInfoLabel, patch)) 
  //  old_dw->get(cellInfoP, d_cellInfoLabel, matlIndex, patch);
  //else {
  //  cellInfoP.setData(scinew CellInformation(patch));
  //  old_dw->put(cellInfoP, d_cellInfoLabel, matlIndex, patch);
  //}
  CellInformation* cellinfo = cellInfoP.get().get_rep();
  new_dw->get(pressureVars.uVelocity, d_lab->d_uVelocitySIVBCLabel, 
	      matlIndex, patch, Ghost::AroundCells, numGhostCells);
  new_dw->get(pressureVars.vVelocity, d_lab->d_vVelocitySIVBCLabel, 
	      matlIndex, patch, Ghost::AroundCells, numGhostCells);
  new_dw->get(pressureVars.wVelocity, d_lab->d_wVelocitySIVBCLabel, 
	      matlIndex, patch, Ghost::AroundCells, numGhostCells);
  old_dw->get(pressureVars.old_uVelocity, d_lab->d_uVelocitySPBCLabel, 
		matlIndex, patch, Ghost::None, zeroGhostCells);
  old_dw->get(pressureVars.old_vVelocity, d_lab->d_vVelocitySPBCLabel, 
		matlIndex, patch, Ghost::None, zeroGhostCells);
  old_dw->get(pressureVars.old_wVelocity, d_lab->d_wVelocitySPBCLabel, 
		matlIndex, patch, Ghost::None, zeroGhostCells);
  old_dw->get(pressureVars.old_density, d_lab->d_densityINLabel, 
		matlIndex, patch, Ghost::None, zeroGhostCells);
  old_dw->get(pressureVars.cellType, d_lab->d_cellTypeLabel, 
		matlIndex, patch, Ghost::AroundCells, numGhostCells);
  
  for(int index = 1; index <= Arches::NDIM; ++index) {
#if  0
    switch (index) {
    case Arches::XDIR:
      new_dw->allocate(pressureVars.variableCalledDU, d_lab->d_DUPBLMLabel,
		       matlIndex, patch);
      break;
    case Arches::YDIR:
      new_dw->allocate(pressureVars.variableCalledDV, d_lab->d_DVPBLMLabel,
			  matlIndex, patch);
      break;
    case Arches::ZDIR:
      new_dw->allocate(pressureVars.variableCalledDW, d_lab->d_DWPBLMLabel,
			  matlIndex, patch);
      break;
    default:
      throw InvalidValue("invalid index for velocity in PressureSolver"); 
    }
#endif

    for (int ii = 0; ii < nofStencils; ii++) {
      switch(index) {
      case Arches::XDIR:
	new_dw->allocate(pressureVars.uVelocityCoeff[ii], 
			    d_lab->d_uVelCoefPBLMLabel, ii, patch);
	new_dw->allocate(pressureVars.uVelocityConvectCoeff[ii], 
			    d_lab->d_uVelConvCoefPBLMLabel, ii, patch);
	break;
      case Arches::YDIR:
	new_dw->allocate(pressureVars.vVelocityCoeff[ii], 
			    d_lab->d_vVelCoefPBLMLabel, ii, patch);
	new_dw->allocate(pressureVars.vVelocityConvectCoeff[ii],
			    d_lab->d_vVelConvCoefPBLMLabel, ii, patch);
	break;
      case Arches::ZDIR:
	new_dw->allocate(pressureVars.wVelocityCoeff[ii], 
			    d_lab->d_wVelCoefPBLMLabel, ii, patch);
	new_dw->allocate(pressureVars.wVelocityConvectCoeff[ii], 
			    d_lab->d_wVelConvCoefPBLMLabel, ii, patch);
	break;
      default:
	throw InvalidValue("invalid index for velocity in PressureSolver");
      }
    }
    // Calculate Velocity Coeffs :
    //  inputs : [u,v,w]VelocitySIVBC, densityIN, viscosityIN
    //  outputs: [u,v,w]VelCoefPBLM, [u,v,w]VelConvCoefPBLM 
    d_discretize->calculateVelocityCoeff(pc, patch, old_dw, new_dw, 
					 delta_t, index, 
					 Arches::PRESSURE, 
					 cellinfo, &pressureVars);
    // Calculate Velocity source
    //  inputs : [u,v,w]VelocitySIVBC, densityIN, viscosityIN
    //  outputs: [u,v,w]VelLinSrcPBLM, [u,v,w]VelNonLinSrcPBLM
    // get data
    // allocate

    switch(index) {
    case Arches::XDIR:
      new_dw->allocate(pressureVars.uVelLinearSrc, 
			  d_lab->d_uVelLinSrcPBLMLabel, matlIndex, patch);
      new_dw->allocate(pressureVars.uVelNonlinearSrc, 
			  d_lab->d_uVelNonLinSrcPBLMLabel,
			  matlIndex, patch);
      break;
    case Arches::YDIR:
      new_dw->allocate(pressureVars.vVelLinearSrc, 
			  d_lab->d_vVelLinSrcPBLMLabel, matlIndex, patch);
      new_dw->allocate(pressureVars.vVelNonlinearSrc, 
			  d_lab->d_vVelNonLinSrcPBLMLabel, matlIndex, patch);
      break;
    case Arches::ZDIR:
      new_dw->allocate(pressureVars.wVelLinearSrc, 
			  d_lab->d_wVelLinSrcPBLMLabel, matlIndex, patch);
      new_dw->allocate(pressureVars.wVelNonlinearSrc,
			  d_lab->d_wVelNonLinSrcPBLMLabel, matlIndex, patch);
      break;
    default:
      throw InvalidValue("Invalid index in PressureSolver for calcVelSrc");
    }
    d_source->calculateVelocitySource(pc, patch, old_dw, new_dw, 
				      delta_t, index,
				      Arches::PRESSURE,
				      cellinfo, &pressureVars);
    // Calculate the Velocity BCS
    //  inputs : densityCP, [u,v,w]VelocitySIVBC, [u,v,w]VelCoefPBLM
    //           [u,v,w]VelLinSrcPBLM, [u,v,w]VelNonLinSrcPBLM
    //  outputs: [u,v,w]VelCoefPBLM, [u,v,w]VelLinSrcPBLM, 
    //           [u,v,w]VelNonLinSrcPBLM
    
    d_boundaryCondition->velocityBC(pc, patch, old_dw, new_dw, 
				    index,
				    Arches::PRESSURE, cellinfo, &pressureVars);

    // Modify Velocity Mass Source
    //  inputs : [u,v,w]VelocitySIVBC, [u,v,w]VelCoefPBLM, 
    //           [u,v,w]VelConvCoefPBLM, [u,v,w]VelLinSrcPBLM, 
    //           [u,v,w]VelNonLinSrcPBLM
    //  outputs: [u,v,w]VelLinSrcPBLM, [u,v,w]VelNonLinSrcPBLM
    d_source->modifyVelMassSource(pc, patch, old_dw, new_dw, delta_t, index,
				  Arches::PRESSURE, &pressureVars);

    // Calculate Velocity diagonal
    //  inputs : [u,v,w]VelCoefPBLM, [u,v,w]VelLinSrcPBLM
    //  outputs: [u,v,w]VelCoefPBLM
    d_discretize->calculateVelDiagonal(pc, patch, old_dw, new_dw, 
				       index,
				       Arches::PRESSURE, &pressureVars);
#ifdef ARCHES_PRES_DEBUG
    std::cerr << "Done building matrix for press coeff" << endl;
#endif

  }
  // put required vars
  for (int ii = 0; ii < nofStencils; ii++) {
    new_dw->put(pressureVars.uVelocityCoeff[ii], d_lab->d_uVelCoefPBLMLabel, 
		   ii, patch);
    new_dw->put(pressureVars.vVelocityCoeff[ii], d_lab->d_vVelCoefPBLMLabel, 
		   ii, patch);
    new_dw->put(pressureVars.wVelocityCoeff[ii], d_lab->d_wVelCoefPBLMLabel, 
		   ii, patch);
  }
  new_dw->put(pressureVars.uVelNonlinearSrc, 
		 d_lab->d_uVelNonLinSrcPBLMLabel, matlIndex, patch);
  new_dw->put(pressureVars.uVelLinearSrc, 
		 d_lab->d_uVelLinSrcPBLMLabel, matlIndex, patch);
  new_dw->put(pressureVars.vVelNonlinearSrc, 
		 d_lab->d_vVelNonLinSrcPBLMLabel, matlIndex, patch);
  new_dw->put(pressureVars.vVelLinearSrc, 
		 d_lab->d_vVelLinSrcPBLMLabel, matlIndex, patch);
  new_dw->put(pressureVars.wVelNonlinearSrc, 
		 d_lab->d_wVelNonLinSrcPBLMLabel, matlIndex, patch);
  new_dw->put(pressureVars.wVelLinearSrc, 
		 d_lab->d_wVelLinSrcPBLMLabel, matlIndex, patch);
#ifdef ARCHES_PRES_DEBUG
  std::cerr << "Done building matrix for vel coeff for pressure" << endl;
#endif

}

//****************************************************************************
// Actually build of linear matrix
//****************************************************************************
void 
PressureSolver::buildLinearMatrixPress(const ProcessorGroup* pc,
				       const Patch* patch,
				       DataWarehouseP& old_dw,
				       DataWarehouseP& new_dw,
				       double delta_t)
{
   ArchesVariables pressureVars;
  // compute all three componenets of velocity stencil coefficients
  int matlIndex = 0;
  int numGhostCells = 1;
  int zeroGhostCells = 0;
  int nofStencils = 7;
  // Get the reference density
  // Get the required data
  new_dw->get(pressureVars.density, d_lab->d_densityINLabel, 
	      matlIndex, patch, Ghost::AroundCells, numGhostCells);
  new_dw->get(pressureVars.viscosity, d_lab->d_viscosityINLabel, 
	      matlIndex, patch, Ghost::AroundCells, numGhostCells);
  new_dw->get(pressureVars.pressure, d_lab->d_pressureINLabel, 
	      matlIndex, patch, Ghost::None, zeroGhostCells);
  // Get the PerPatch CellInformation data
  PerPatch<CellInformationP> cellInfoP;
  old_dw->get(cellInfoP, d_lab->d_cellInfoLabel, matlIndex, patch);
  //  old_dw->get(cellInfoP, d_cellInfoLabel, matlIndex, patch);
  //  if (old_dw->exists(d_cellInfoLabel, patch)) 
  //  old_dw->get(cellInfoP, d_cellInfoLabel, matlIndex, patch);
  //else {
  //  cellInfoP.setData(scinew CellInformation(patch));
  //  old_dw->put(cellInfoP, d_cellInfoLabel, matlIndex, patch);
  //}
  CellInformation* cellinfo = cellInfoP.get().get_rep();
  new_dw->get(pressureVars.uVelocity, d_lab->d_uVelocitySIVBCLabel, 
	      matlIndex, patch, Ghost::AroundCells, numGhostCells+1);
  new_dw->get(pressureVars.vVelocity, d_lab->d_vVelocitySIVBCLabel, 
	      matlIndex, patch, Ghost::AroundCells, numGhostCells+1);
  new_dw->get(pressureVars.wVelocity, d_lab->d_wVelocitySIVBCLabel, 
	      matlIndex, patch, Ghost::AroundCells, numGhostCells+1);
  old_dw->get(pressureVars.old_density, d_lab->d_densityINLabel, 
		matlIndex, patch, Ghost::None, zeroGhostCells);
  old_dw->get(pressureVars.cellType, d_lab->d_cellTypeLabel, 
		matlIndex, patch, Ghost::AroundCells, numGhostCells);
  
  for (int ii = 0; ii < nofStencils; ii++) {
    new_dw->get(pressureVars.uVelocityCoeff[ii], 
		d_lab->d_uVelCoefPBLMLabel, ii, patch,
		Ghost::AroundCells, numGhostCells);
    new_dw->get(pressureVars.vVelocityCoeff[ii], 
		d_lab->d_vVelCoefPBLMLabel, ii, patch,
		Ghost::AroundCells, numGhostCells);
    new_dw->get(pressureVars.wVelocityCoeff[ii], 
		d_lab->d_wVelCoefPBLMLabel, ii, patch,
		Ghost::AroundCells, numGhostCells);
  }
  new_dw->get(pressureVars.uVelNonlinearSrc, 
	      d_lab->d_uVelNonLinSrcPBLMLabel,
	      matlIndex, patch,
	      Ghost::AroundCells, numGhostCells);
  new_dw->get(pressureVars.vVelNonlinearSrc, 
	      d_lab->d_vVelNonLinSrcPBLMLabel, matlIndex, patch,
	      Ghost::AroundCells, numGhostCells);
  new_dw->get(pressureVars.wVelNonlinearSrc,
	      d_lab->d_wVelNonLinSrcPBLMLabel, matlIndex, patch,
	      Ghost::AroundCells, numGhostCells);
 
  // Calculate Pressure Coeffs
  //  inputs : densityIN, pressureIN, [u,v,w]VelCoefPBLM[Arches::AP]
  //  outputs: presCoefPBLM[Arches::AE..AB] 
  for (int ii = 0; ii < nofStencils; ii++)
    new_dw->allocate(pressureVars.pressCoeff[ii], 
			d_lab->d_presCoefPBLMLabel, ii, patch);
  d_discretize->calculatePressureCoeff(pc, patch, old_dw, new_dw, 
				       delta_t, cellinfo, &pressureVars);

  // Calculate Pressure Source
  //  inputs : pressureSPBC, [u,v,w]VelocitySIVBC, densityCP,
  //           [u,v,w]VelCoefPBLM, [u,v,w]VelNonLinSrcPBLM
  //  outputs: presLinSrcPBLM, presNonLinSrcPBLM
  // Allocate space
  new_dw->allocate(pressureVars.pressLinearSrc, 
		      d_lab->d_presLinSrcPBLMLabel, matlIndex, patch);
  new_dw->allocate(pressureVars.pressNonlinearSrc, 
		      d_lab->d_presNonLinSrcPBLMLabel, matlIndex, patch);

  d_source->calculatePressureSource(pc, patch, old_dw, new_dw, delta_t,
				    cellinfo, &pressureVars);


  // Calculate Pressure BC
  //  inputs : pressureIN, presCoefPBLM
  //  outputs: presCoefPBLM
  d_boundaryCondition->pressureBC(pc, patch, old_dw, new_dw, 
				  cellinfo, &pressureVars);

  // Calculate Pressure Diagonal
  //  inputs : presCoefPBLM, presLinSrcPBLM
  //  outputs: presCoefPBLM 
  d_discretize->calculatePressDiagonal(pc, patch, old_dw, new_dw, 
				       &pressureVars);
  
  // put required vars
  for (int ii = 0; ii < nofStencils; ii++) {
    new_dw->put(pressureVars.pressCoeff[ii], d_lab->d_presCoefPBLMLabel, 
		   ii, patch);
  }
  new_dw->put(pressureVars.pressNonlinearSrc, 
		 d_lab->d_presNonLinSrcPBLMLabel, matlIndex, patch);
#ifdef ARCHES_PRES_DEBUG
  std::cerr << "Done building matrix for press coeff" << endl;
#endif

}


void 
PressureSolver::pressureLinearSolve_all (const ProcessorGroup* pg,
					 const Patch*,
					 DataWarehouseP& old_dw,
					 DataWarehouseP& new_dw,
					 LevelP level, SchedulerP sched)
{
  ArchesVariables pressureVars;
  int me = pg->myrank();
  LoadBalancer* lb = sched->getLoadBalancer();

  // initializeMatrix...
  d_linearSolver->matrixCreate(level, lb);
#ifdef ARCHES_PRES_DEBUG
  cerr << "Finished creating petsc matrix\n";
#endif

  for(Level::const_patchIterator iter=level->patchesBegin();
      iter != level->patchesEnd(); iter++){
    const Patch* patch=*iter;
    {
       int proc = lb->getPatchwiseProcessorAssignment(patch, d_myworld);
       if(proc == me){
	  // Underrelax...

	  // This calls fillRows on linear(petsc) solver
#ifdef ARCHES_PRES_DEBUG
	  cerr << "Calling pressureLinearSolve for patch: " << patch->getID() << '\n';
#endif
	  pressureLinearSolve(pg, patch, old_dw, new_dw, pressureVars);
#ifdef ARCHES_PRES_DEBUG
	  cerr << "Done with pressureLinearSolve for patch: " << patch->getID() << '\n';
#endif
       }
    }
  }
  // MPI_Reduce();
  // solve
#ifdef ARCHES_PRES_DEBUG
  cerr << "Calling pressLinearSolve\n";
#endif
  d_linearSolver->pressLinearSolve();
#ifdef ARCHES_PRES_DEBUG
  cerr << "Done with pressLinearSolve\n";
#endif
  int pressRefProc = -1;
  for(Level::const_patchIterator iter=level->patchesBegin();
      iter != level->patchesEnd(); iter++){
    const Patch* patch=*iter;
    {
       int proc = lb->getPatchwiseProcessorAssignment(patch, d_myworld);
       //int proc = find_processor_assignment(patch);
       if(proc == me){
	 //	  unpack from linear solver.
	 d_linearSolver->copyPressSoln(patch, &pressureVars);
#ifdef ARCHES_PRES_DEBUG
	 cerr << "Calling normPressure for patch: " << patch->getID() << '\n';
#endif
       }
       if (patch->containsCell(d_pressRef)) {
	  pressRefProc = proc;
	  if(pressRefProc == me){
	     pressureVars.press_ref = pressureVars.pressure[d_pressRef];
#ifdef ARCHES_PRES_DEBUG
	     cerr << "press_ref for norm" << pressureVars.press_ref << " " <<
	       pressRefProc << endl;
#endif
	  }
       }
    }
  }
    if(pressRefProc == -1)
       throw InternalError("Patch containing pressure reference point was not found");

    MPI_Bcast(&pressureVars.press_ref, 1, MPI_DOUBLE, pressRefProc, pg->getComm());
    for(Level::const_patchIterator iter=level->patchesBegin();
	iter != level->patchesEnd(); iter++){
     const Patch* patch=*iter;
     {
#ifdef ARCHES_PRES_DEBUG
	 cerr << "After presssoln" << endl;
#endif
       int proc = lb->getPatchwiseProcessorAssignment(patch, d_myworld);
       //int proc = find_processor_assignment(patch);
       if(proc == me){
#ifdef ARCHES_PRES_DEBUG
	 for(CellIterator iter = patch->getCellIterator();
	     !iter.done(); iter++){
	   cerr.width(10);
	   cerr << "press"<<*iter << ": " << pressureVars.pressure[*iter] << "\n" ; 
	 }
#endif
       	 normPressure(pg, patch, &pressureVars);
#ifdef ARCHES_PRES_DEBUG
	 cerr << "Done with normPressure for patch: " 
	      << patch->getID() << '\n';
#endif
	 // put back the results
	 int matlIndex = 0;
	 new_dw->put(pressureVars.pressure, d_lab->d_pressurePSLabel, 
		     matlIndex, patch);
       }
     }
    }

  // destroy matrix
  d_linearSolver->destroyMatrix();
}

// Actual linear solve
void 
PressureSolver::pressureLinearSolve (const ProcessorGroup* pc,
				     const Patch* patch,
				     DataWarehouseP& old_dw,
				     DataWarehouseP& new_dw,
				     ArchesVariables& pressureVars)
{
  int matlIndex = 0;
  int numGhostCells = 1;
  int zeroGhostCells = 0;
  int nofStencils = 7;
  // Get the required data
  new_dw->get(pressureVars.pressure, d_lab->d_pressureINLabel, 
	      matlIndex, patch, Ghost::AroundCells, numGhostCells);
  for (int ii = 0; ii < nofStencils; ii++) 
    new_dw->get(pressureVars.pressCoeff[ii], d_lab->d_presCoefPBLMLabel, 
		   ii, patch, Ghost::None, zeroGhostCells);

  new_dw->get(pressureVars.pressNonlinearSrc, 
		 d_lab->d_presNonLinSrcPBLMLabel, 
		 matlIndex, patch, Ghost::None, zeroGhostCells);

  // compute eqn residual, L1 norm
  new_dw->allocate(pressureVars.residualPressure, d_lab->d_pressureRes,
			  matlIndex, patch);
#if 0
  d_linearSolver->computePressResidual(pc, patch, old_dw, new_dw, 
				       &pressureVars);
#else
  pressureVars.residPress=pressureVars.truncPress=0;
#endif
  new_dw->put(sum_vartype(pressureVars.residPress), d_lab->d_presResidPSLabel);
  new_dw->put(sum_vartype(pressureVars.truncPress), d_lab->d_presTruncPSLabel);
  // apply underelaxation to eqn
  d_linearSolver->computePressUnderrelax(pc, patch, old_dw, new_dw,
					 &pressureVars);
  // put back computed matrix coeffs and nonlinear source terms 
  // modified as a result of underrelaxation 
  // into the matrix datawarehouse
#if 0
  for (int ii = 0; ii < nofStencils; ii++) {
    new_dw->put(pressureVars.pressCoeff[ii], d_lab->d_presCoefPSLabel, ii, patch);
  }
  new_dw->put(pressureVars.pressNonLinSrc, d_lab->d_presNonLinSrcPSLabel, 
	      matlIndex, patch);
#endif

  // for parallel code lisolve will become a recursive task and 
  // will make the following subroutine separate
  // get patch numer ***warning****
  // sets matrix
  d_linearSolver->setPressMatrix(pc, patch, old_dw, new_dw, &pressureVars, d_lab);
  //  d_linearSolver->pressLinearSolve();

}
  
  

//****************************************************************************
// normalize the pressure solution
//****************************************************************************
void 
PressureSolver::normPressure(const ProcessorGroup*,
			     const Patch* patch,
			     ArchesVariables* vars)
{
  IntVector domLo = vars->pressure.getFortLowIndex();
  IntVector domHi = vars->pressure.getFortHighIndex();
  IntVector idxLo = patch->getCellFORTLowIndex();
  IntVector idxHi = patch->getCellFORTHighIndex();
  double pressref = vars->press_ref;
  //  double pressref = 0.0;
  FORT_NORMPRESS(domLo.get_pointer(),domHi.get_pointer(),
		idxLo.get_pointer(), idxHi.get_pointer(),
		vars->pressure.getPointer(), 
		&pressref);

#ifdef ARCHES_PRES_DEBUG
  cerr << " After Pressure Normalization : " << endl;
  for (int ii = idxLo.x(); ii <= idxHi.x(); ii++) {
    cerr << "pressure for ii = " << ii << endl;
    for (int jj = idxLo.y(); jj <= idxHi.y(); jj++) {
      for (int kk = idxLo.z(); kk <= idxHi.z(); kk++) {
	cerr.width(10);
	cerr << vars->pressure[IntVector(ii,jj,kk)] << " " ; 
      }
      cerr << endl;
    }
  }
#endif
}  

//
// $Log$
// Revision 1.63  2000/10/14 17:11:05  sparker
// Changed PerPatch<CellInformation*> to PerPatch<CellInformationP>
// to get rid of memory leak
//
// Revision 1.62  2000/10/12 20:08:33  sparker
// Made multipatch work for several timesteps
// Cleaned up print statements
//
// Revision 1.61  2000/10/10 19:30:57  rawat
// added scalarsolver
//
// Revision 1.60  2000/10/09 17:06:25  rawat
// modified momentum solver for multi-patch
//
// Revision 1.59  2000/10/08 18:56:35  rawat
// fixed the solver for multi
//
// Revision 1.58  2000/10/07 21:40:49  rawat
// fixed pressure norm
//
// Revision 1.57  2000/10/07 05:38:51  sparker
// Changed d_pressureVars into a few different local variables so that
// they will be freed at the end of the task.
//
// Revision 1.56  2000/10/05 16:39:46  rawat
// modified bcs for multi-patch
//
// Revision 1.55  2000/10/04 16:46:23  rawat
// Parallel solver for pressure is working
//
// Revision 1.54  2000/10/02 16:40:25  rawat
// updated cellinformation for multi-patch
//
// Revision 1.53  2000/09/29 20:32:36  rawat
// added underrelax to pressure solver
//
// Revision 1.52  2000/09/26 19:59:17  sparker
// Work on MPI petsc
//
// Revision 1.51  2000/09/25 16:29:23  rawat
// modified requires in PressureSolver for multiple patches
//
// Revision 1.50  2000/09/21 21:45:05  rawat
// added petsc parallel stuff
//
// Revision 1.49  2000/09/20 18:05:33  sparker
// Adding support for Petsc and per-processor tasks
//
// Revision 1.48  2000/09/20 16:56:16  rawat
// added some petsc parallel stuff and fixed some bugs
//
// Revision 1.47  2000/09/14 17:04:54  rawat
// converting arches to multipatch
//
// Revision 1.46  2000/09/12 15:46:23  sparker
// Use petscsolver
//
// Revision 1.45  2000/08/23 06:20:52  bbanerje
// 1) Results now correct for pressure solve.
// 2) Modified BCU, BCV, BCW to add stuff for pressure BC.
// 3) Removed some bugs in BCU, V, W.
// 4) Coefficients for MOM Solve not computed correctly yet.
//
// Revision 1.44  2000/08/11 21:26:36  rawat
// added linear solver for pressure eqn
//
// Revision 1.43  2000/08/10 21:29:09  rawat
// fixed a bug in cellinformation
//
// Revision 1.42  2000/08/01 23:28:43  skumar
// Added residual calculation procedure and modified templates in linear
// solver.  Added template for order-of-magnitude term calculation.
//
// Revision 1.41  2000/08/01 06:18:38  bbanerje
// Made ScalarSolver similar to PressureSolver and MomentumSolver.
//
// Revision 1.40  2000/07/28 02:31:00  rawat
// moved all the labels in ArchesLabel. fixed some bugs and added matrix_dw to store matrix
// coeffecients
//
// Revision 1.37  2000/07/17 22:06:58  rawat
// modified momentum source
//
// Revision 1.36  2000/07/14 03:45:46  rawat
// completed velocity bc and fixed some bugs
//
// Revision 1.35  2000/07/13 06:32:10  bbanerje
// Labels are once more consistent for one iteration.
//
// Revision 1.34  2000/07/12 23:59:21  rawat
// added wall bc for u-velocity
//
// Revision 1.33  2000/07/12 22:15:02  bbanerje
// Added pressure Coef .. will do until Kumar's code is up and running
//
// Revision 1.32  2000/07/12 07:35:46  bbanerje
// Added stuff for mascal : Rawat: Labels and dataWarehouse in velsrc need to be corrected.
//
// Revision 1.31  2000/07/11 15:46:28  rawat
// added setInitialGuess in PicardNonlinearSolver and also added uVelSrc
//
// Revision 1.30  2000/07/08 23:42:55  bbanerje
// Moved all enums to Arches.h and made corresponding changes.
//
// Revision 1.29  2000/07/08 08:03:34  bbanerje
// Readjusted the labels upto uvelcoef, removed bugs in CellInformation,
// made needed changes to uvelcoef.  Changed from StencilMatrix::AE etc
// to Arches::AE .. doesn't like enums in templates apparently.
//
// Revision 1.28  2000/07/07 23:07:45  rawat
// added inlet bc's
//
// Revision 1.27  2000/07/03 05:30:15  bbanerje
// Minor changes for inlbcs dummy code to compile and work. densitySIVBC is no more.
//
// Revision 1.26  2000/06/29 22:56:43  bbanerje
// Changed FCVars to SFC[X,Y,Z]Vars, and added the neceesary getIndex calls.
//
// Revision 1.25  2000/06/22 23:06:35  bbanerje
// Changed velocity related variables to FCVariable type.
// ** NOTE ** We may need 3 types of FCVariables (one for each direction)
//
// Revision 1.24  2000/06/21 07:51:00  bbanerje
// Corrected new_dw, old_dw problems, commented out intermediate dw (for now)
// and made the stuff go through schedule_time_advance.
//
// Revision 1.23  2000/06/21 06:49:21  bbanerje
// Straightened out some of the problems in data location .. still lots to go.
//
// Revision 1.22  2000/06/21 06:12:12  bbanerje
// Added missing VarLabel* mallocs .
//
// Revision 1.21  2000/06/18 01:20:16  bbanerje
// Changed names of varlabels in source to reflect the sequence of tasks.
// Result : Seg Violation in addTask in MomentumSolver
//
// Revision 1.20  2000/06/17 07:06:25  sparker
// Changed ProcessorContext to ProcessorGroup
//
// Revision 1.19  2000/06/16 04:25:40  bbanerje
// Uncommented BoundaryCondition related stuff.
//
// Revision 1.18  2000/06/14 20:40:49  rawat
// modified boundarycondition for physical boundaries and
// added CellInformation class
//
// Revision 1.17  2000/06/07 06:13:55  bbanerje
// Changed CCVariable<Vector> to CCVariable<double> for most cases.
// Some of these variables may not be 3D Vectors .. they may be Stencils
// or more than 3D arrays. Need help here.
//
// Revision 1.15  2000/06/04 23:57:46  bbanerje
// Updated Arches to do ScheduleTimeAdvance.
//
// Revision 1.14  2000/06/04 22:40:14  bbanerje
// Added Cocoon stuff, changed task, require, compute, get, put arguments
// to reflect new declarations. Changed sub.mk to include all the new files.
//
//
