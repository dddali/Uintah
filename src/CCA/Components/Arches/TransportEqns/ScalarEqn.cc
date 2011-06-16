#include <CCA/Components/Arches/TransportEqns/ScalarEqn.h>
#include <CCA/Components/Arches/SourceTerms/SourceTermFactory.h>
#include <CCA/Components/Arches/SourceTerms/SourceTermBase.h>
#include <CCA/Components/Arches/Directives.h>
#include <CCA/Ports/Scheduler.h>
#include <Core/ProblemSpec/ProblemSpec.h>
#include <Core/Grid/Level.h>
#include <Core/Grid/SimulationState.h>
#include <Core/Grid/Variables/VarTypes.h>
#include <Core/Exceptions/InvalidValue.h>
#include <Core/Parallel/Parallel.h>
#include <Core/Parallel/ProcessorGroup.h>
#include <Core/Grid/DbgOutput.h>

using namespace std;
using namespace Uintah;

static DebugStream dbg("ARCHES", false);

//---------------------------------------------------------------------------
// Builder:
CCScalarEqnBuilder::CCScalarEqnBuilder( ArchesLabel* fieldLabels, 
                                        ExplicitTimeInt* timeIntegrator,
                                        string eqnName ) : 
EqnBuilder( fieldLabels, timeIntegrator, eqnName )
{}
CCScalarEqnBuilder::~CCScalarEqnBuilder(){}

EqnBase*
CCScalarEqnBuilder::build(){
  return scinew ScalarEqn(d_fieldLabels, d_timeIntegrator, d_eqnName);
}
// End Builder
//---------------------------------------------------------------------------

ScalarEqn::ScalarEqn( ArchesLabel* fieldLabels, ExplicitTimeInt* timeIntegrator, string eqnName )
: 
EqnBase( fieldLabels, timeIntegrator, eqnName )
{
 
  std::string varname = eqnName; 
  d_transportVarLabel = VarLabel::create(varname,
            CCVariable<double>::getTypeDescription()); 
  varname = eqnName+"_Fdiff"; 
  d_FdiffLabel = VarLabel::create(varname, 
            CCVariable<double>::getTypeDescription());
  varname = eqnName+"_Fconv"; 
  d_FconvLabel = VarLabel::create(varname, 
            CCVariable<double>::getTypeDescription());
  varname = eqnName+"_RHS";
  d_RHSLabel = VarLabel::create(varname, 
            CCVariable<double>::getTypeDescription());
  varname = eqnName+"_old";
  d_oldtransportVarLabel = VarLabel::create(varname,
            CCVariable<double>::getTypeDescription());
  varname = eqnName+"_scalar_prNo"; 
  d_prNo_label = VarLabel::create( varname, 
            CCVariable<double>::getTypeDescription()); 
  
#ifdef VERIFICATION
  varname = eqnName+"_MMSError";
  d_MMSErrorLabel = VarLabel::create( varname, CCVariable<double>::getTypeDescription() );

  varname = eqnName + "_MMSExact";
  d_MMSExactLabel = VarLabel::create( varname, CCVariable<double>::getTypeDescription() );

  varname = eqnName+"_MMSError_L2Norm";
  d_MMSErrorL2Label = VarLabel::create( varname, sum_vartype::getTypeDescription() );

  varname = eqnName+"_MMSError_LInfNorm";
  d_MMSErrorLInfLabel = VarLabel::create( varname, sum_vartype::getTypeDescription() );
#endif
}

ScalarEqn::~ScalarEqn()
{
  VarLabel::destroy(d_FdiffLabel);
  VarLabel::destroy(d_FconvLabel); 
  VarLabel::destroy(d_RHSLabel);
  VarLabel::destroy(d_transportVarLabel);
  VarLabel::destroy(d_oldtransportVarLabel);
  VarLabel::destroy(d_prNo_label); 

#ifdef VERIFICATION
  VarLabel::destroy(d_MMSErrorLabel);
  VarLabel::destroy(d_MMSExactLabel);
  VarLabel::destroy(d_MMSErrorL2Label);
  VarLabel::destroy(d_MMSErrorLInfLabel);
#endif
}
//---------------------------------------------------------------------------
// Method: Problem Setup 
//---------------------------------------------------------------------------
void
ScalarEqn::problemSetup(const ProblemSpecP& inputdb)
{
  ProblemSpecP db = inputdb; 

  db->getWithDefault("turbulentPrandtlNumber",d_turbPrNo,0.4);

  if (db->findBlock("use_laminar_pr")) {
    d_laminar_pr = true; 
  } else {
    d_laminar_pr = false; 
  }
 
  // Discretization information
  db->getWithDefault( "conv_scheme", d_convScheme, "upwind");
  db->getWithDefault( "doConv", d_doConv, false);
  db->getWithDefault( "doDiff", d_doDiff, false);
  db->getWithDefault( "addSources", d_addSources, true); 
  db->getWithDefault( "timestepMultiplier", d_timestepMultiplier, 1.0);
  
  // algorithmic knobs
  //Keep USE_DENSITY_GUESS set to true until the algorithmic details are settled. - Jeremy 
  d_use_density_guess = true; // use the density guess rather than the new density from the table...implies that the equation is updated BEFORE properties are computed. 

  // Clipping:
  d_doClipping = false; 
  ProblemSpecP db_clipping = db->findBlock("Clipping");

  if (db_clipping) {
    //This seems like a *safe* number to assume 
    double clip_default = -999999;

    d_doLowClip = false; 
    d_doHighClip = false; 
    d_doClipping = true;
    
    db_clipping->getWithDefault("low", d_lowClip,  clip_default);
    db_clipping->getWithDefault("high",d_highClip, clip_default);

    if ( d_lowClip != clip_default ) 
      d_doLowClip = true; 

    if ( d_highClip != clip_default ) 
      d_doHighClip = true; 

    if ( !d_doHighClip && !d_doLowClip ) 
      throw InvalidValue("A low or high clipping must be specified if the <Clipping> section is activated!", __FILE__, __LINE__);
  } 

  // Scaling information
  db->getWithDefault( "scaling_const", d_scalingConstant, 1.0 ); 

  // Initialization function
  ProblemSpecP db_initialValue = db->findBlock("initialization");
  if (db_initialValue) {

    db_initialValue->getAttribute("type", d_initFunction); 

    if (d_initFunction == "constant") {
      db_initialValue->require("constant", d_constant_init); 

    } else if (d_initFunction == "step") {
      db_initialValue->require("step_direction", d_step_dir); 
      db_initialValue->require("step_value", d_step_value); 

      if( db_initialValue->findBlock("step_start") ) {
        b_stepUsesPhysicalLocation = true;
        db_initialValue->require("step_start", d_step_start); 
        db_initialValue->require("step_end"  , d_step_end); 

      } else if ( db_initialValue->findBlock("step_cellstart") ) {
        b_stepUsesCellLocation = true;
        db_initialValue->require("step_cellstart", d_step_cellstart);
        db_initialValue->require("step_cellend", d_step_cellend);
      }

    } else if (d_initFunction == "mms_x") {

    } else if (d_initFunction == "mms_y") {

    } else if (d_initFunction == "mms_z") {

    } else if (d_initFunction == "mms_xyz") {

    } else if (d_initFunction == "geometry_fill") {

      db_initialValue->require("constant", d_constant_init); // going to full with this constant 

      ProblemSpecP the_geometry = db_initialValue->findBlock("geom_object"); 
      if (the_geometry) {
        GeometryPieceFactory::create(the_geometry, d_initGeom); 
      } else {
        throw ProblemSetupException("You are missing the geometry specification (<geom_object>) for the transport eqn. initialization!", __FILE__, __LINE__); 
      }
    }
  }
}

void
ScalarEqn::problemSetupSources( const ProblemSpecP& db )
{
  // Source terms:
  SourceTermFactory& srcFactory = SourceTermFactory::self();
  for( ProblemSpecP eqn_db  = db->findBlock("Eqn"); eqn_db != 0; eqn_db=eqn_db->findNextBlock("Eqn")) {
    std::string eqn_name;
    eqn_db->getAttribute("label", eqn_name);
    if( eqn_name == d_eqnName ) {
      if( eqn_db->findBlock("src") ) {
        string srcname; 
        for (ProblemSpecP src_db = eqn_db->findBlock("src"); src_db != 0; src_db = src_db->findNextBlock("src")){
          src_db->getAttribute("label", srcname);
          //which sources are turned on for this equation
          d_sources.push_back( srcFactory.retrieve_source_term(srcname).getSrcLabel() ); 
        }
      }
    }
  }
}


//---------------------------------------------------------------------------
// Method: Schedule clean up. 
//---------------------------------------------------------------------------
void 
ScalarEqn::sched_cleanUp( const LevelP& level, SchedulerP& sched )
{
  string taskname = "ScalarEqn::cleanUp";
  Task* tsk = scinew Task(taskname, this, &ScalarEqn::cleanUp);

  sched->addTask(tsk, level->eachPatch(), d_fieldLabels->d_sharedState->allArchesMaterials());
}


//---------------------------------------------------------------------------
// Method: Actually clean up. 
//---------------------------------------------------------------------------
void ScalarEqn::cleanUp( const ProcessorGroup* pc, 
                         const PatchSubset* patches, 
                         const MaterialSubset* matls, 
                         DataWarehouse* old_dw, 
                         DataWarehouse* new_dw )
{
  if( d_addSources ) {
    //Set the initialization flag for the source label to false.
    SourceTermFactory& factory = SourceTermFactory::self(); 
    for( vector<const VarLabel*>::iterator iLabel = d_sources.begin(); iLabel != d_sources.end(); ++iLabel ) {
      SourceTermBase& temp_src = factory.retrieve_source_term( (*iLabel)->getName() );
      temp_src.reinitializeLabel(); 
    }
  }
}


//---------------------------------------------------------------------------
// Method: Schedule the intialization of the variables. 
//---------------------------------------------------------------------------
void 
ScalarEqn::sched_initializeVariables( const LevelP& level, SchedulerP& sched )
{
  string taskname = "ScalarEqn::initializeVariables";

  Task* tsk = scinew Task(taskname, this, &ScalarEqn::initializeVariables);

  printSchedule(level, dbg, taskname);

  Ghost::GhostType gn = Ghost::None;
  
  // New DW
  tsk->computes(d_transportVarLabel);
  tsk->computes(d_oldtransportVarLabel); // for rk sub stepping 
  tsk->computes(d_RHSLabel); 
  tsk->computes(d_FconvLabel);
  tsk->computes(d_FdiffLabel);
  tsk->computes(d_prNo_label); 

#ifdef VERIFICATION
  tsk->computes(d_MMSErrorLabel);
  tsk->computes(d_MMSExactLabel);
  tsk->computes(d_MMSErrorL2Label);
  tsk->computes(d_MMSErrorLInfLabel);
#endif

  //Old DW
  tsk->requires(Task::OldDW, d_transportVarLabel, gn, 0);
  if (d_laminar_pr){
    // This requires that the LaminarPrNo model is activated
    const VarLabel* pr_label = VarLabel::find("laminar_pr"); 
    tsk->requires(Task::OldDW, pr_label, gn, 0); 
  }

  sched->addTask(tsk, level->eachPatch(), d_fieldLabels->d_sharedState->allArchesMaterials());
}

/** @details
Actually initialize the variables used in the solution of the
scalar transport equations. This is performed each timestep.

@seealso EqnFactory
*/
void ScalarEqn::initializeVariables( const ProcessorGroup* pc, 
                                     const PatchSubset* patches, 
                                     const MaterialSubset* matls, 
                                     DataWarehouse* old_dw, 
                                     DataWarehouse* new_dw )
{

  //patch loop
  for (int p=0; p < patches->size(); p++){

    Ghost::GhostType  gn  = Ghost::None;

    const Patch* patch = patches->get(p);
    int archIndex = 0;
    int matlIndex = d_fieldLabels->d_sharedState->getArchesMaterial(archIndex)->getDWIndex(); 

    CCVariable<double> newVar;
    CCVariable<double> rkoldVar; 
    constCCVariable<double> oldVar; 
    new_dw->allocateAndPut( newVar, d_transportVarLabel, matlIndex, patch );
    new_dw->allocateAndPut( rkoldVar, d_oldtransportVarLabel, matlIndex, patch ); 
    old_dw->get(oldVar, d_transportVarLabel, matlIndex, patch, gn, 0);

    newVar.initialize(0.0);
    rkoldVar.initialize(0.0);
    // copy old into new
    newVar.copyData(oldVar);
    rkoldVar.copyData(oldVar); 

    CCVariable<double> Fdiff; 
    CCVariable<double> Fconv; 
    CCVariable<double> RHS; 

    new_dw->allocateAndPut( Fdiff, d_FdiffLabel, matlIndex, patch );
    new_dw->allocateAndPut( Fconv, d_FconvLabel, matlIndex, patch );
    new_dw->allocateAndPut( RHS, d_RHSLabel, matlIndex, patch ); 
    
    Fdiff.initialize(0.0);
    Fconv.initialize(0.0);
    RHS.initialize(0.0);

    CCVariable<double> prNumber; 
    new_dw->allocateAndPut( prNumber, d_prNo_label, matlIndex, patch ); 

    if ( d_laminar_pr ) { 
      constCCVariable<double> laminarPrNumber; 
      const VarLabel* prLabel = VarLabel::find("laminar_pr"); 
      old_dw->get( laminarPrNumber, prLabel, matlIndex, patch, gn, 0 ); 
      prNumber.copyData( laminarPrNumber ); 
    } else { 
      prNumber.initialize( d_turbPrNo ); 
    }
    curr_time = d_fieldLabels->d_sharedState->getElapsedTime(); 
    curr_ssp_time = curr_time; 

#ifdef VERIFICATION
    CCVariable<double> MMSError;
    new_dw->allocateAndPut( MMSError, d_MMSErrorLabel, matlIndex, patch );
    MMSError.initialize(0.0);

    CCVariable<double> MMSExact;
    new_dw->allocateAndPut( MMSExact, d_MMSExactLabel, matlIndex, patch );
    MMSExact.initialize(0.0);

    new_dw->put( sum_vartype(0.0), d_MMSErrorL2Label );
    new_dw->put( sum_vartype(0.0), d_MMSErrorLInfLabel );
#endif

  }
}
//---------------------------------------------------------------------------
// Method: Schedule compute the sources. 
//--------------------------------------------------------------------------- 
void 
ScalarEqn::sched_computeSources( const LevelP& level, 
                                 SchedulerP& sched, 
                                 int timeSubStep )
{
  if( d_addSources ) {
    SourceTermFactory& factory = SourceTermFactory::self(); 
    for( vector<const VarLabel*>::iterator iLabel = d_sources.begin(); iLabel != d_sources.end(); ++iLabel ) {
      SourceTermBase& temp_src = factory.retrieve_source_term( (*iLabel)->getName() );
      temp_src.sched_computeSource( level, sched, timeSubStep ); 
    }
  }
}

//---------------------------------------------------------------------------
// Method: Schedule build the transport equation. 
//---------------------------------------------------------------------------
void
ScalarEqn::sched_buildTransportEqn( const LevelP& level, SchedulerP& sched, int timeSubStep )
{
  string taskname = "ScalarEqn::buildTransportEqn"; 

  Task* tsk = scinew Task(taskname, this, &ScalarEqn::buildTransportEqn, timeSubStep);

  printSchedule(level, dbg, taskname);

  //----NEW----
  // note that rho and U are copied into new DW in ExplicitSolver::setInitialGuess
  tsk->requires(Task::NewDW, d_fieldLabels->d_densityCPLabel, Ghost::AroundCells, 1); 
  tsk->requires(Task::NewDW, d_fieldLabels->d_viscosityCTSLabel, Ghost::AroundCells, 1);
  tsk->requires(Task::NewDW, d_fieldLabels->d_uVelocitySPBCLabel, Ghost::AroundCells, 1);   
#ifdef YDIM
  tsk->requires(Task::NewDW, d_fieldLabels->d_vVelocitySPBCLabel, Ghost::AroundCells, 1); 
#endif
#ifdef ZDIM
  tsk->requires(Task::NewDW, d_fieldLabels->d_wVelocitySPBCLabel, Ghost::AroundCells, 1); 
#endif
  tsk->modifies(d_FdiffLabel);
  tsk->modifies(d_FconvLabel);
  tsk->modifies(d_RHSLabel);
  tsk->requires(Task::NewDW, d_oldtransportVarLabel, Ghost::AroundCells, 2);
  tsk->requires(Task::NewDW, d_prNo_label, Ghost::None, 0); 

  // extra srcs
  if (d_addSources) {
    for( vector<const VarLabel*>::iterator iter = d_sources.begin(); iter != d_sources.end(); ++iter ) {
      tsk->requires( Task::NewDW, (*iter), Ghost::None, 0 );
    }
  }

  tsk->requires(Task::OldDW, d_fieldLabels->d_areaFractionLabel, Ghost::AroundCells, 2); 

  sched->addTask(tsk, level->eachPatch(), d_fieldLabels->d_sharedState->allArchesMaterials());
}

//---------------------------------------------------------------------------
// Method: Actually build the transport equation. 
//---------------------------------------------------------------------------
void 
ScalarEqn::buildTransportEqn( const ProcessorGroup* pc, 
                              const PatchSubset* patches, 
                              const MaterialSubset* matls, 
                              DataWarehouse* old_dw, 
                              DataWarehouse* new_dw,
                              int timeSubStep )
{
  //patch loop
  for (int p=0; p < patches->size(); p++){

    //Ghost::GhostType  gaf = Ghost::AroundFaces;
    Ghost::GhostType  gac = Ghost::AroundCells;
    Ghost::GhostType  gn  = Ghost::None;

    const Patch* patch = patches->get(p);
    int archIndex = 0;
    int matlIndex = d_fieldLabels->d_sharedState->getArchesMaterial(archIndex)->getDWIndex(); 

    Vector Dx = patch->dCell(); 

    constCCVariable<double> oldPhi;
    constCCVariable<double> den;
    constCCVariable<double> mu_t;
    constSFCXVariable<double> uVel; 
    constSFCYVariable<double> vVel; 
    constSFCZVariable<double> wVel; 
    constCCVariable<double> extra_src; // Any additional source (eg, mms or unweighted abscissa src)  
    constCCVariable<Vector> areaFraction; 
    constCCVariable<double> prNumber; 

    CCVariable<double> Fdiff; 
    CCVariable<double> Fconv; 
    CCVariable<double> RHS; 

    new_dw->get(oldPhi,       d_oldtransportVarLabel, matlIndex, patch, gac, 2);
    new_dw->get(den,          d_fieldLabels->d_densityCPLabel, matlIndex, patch, gac, 1); 
    new_dw->get(mu_t,         d_fieldLabels->d_viscosityCTSLabel, matlIndex, patch, gac, 1); 
    new_dw->get(uVel,         d_fieldLabels->d_uVelocitySPBCLabel, matlIndex, patch, gac, 1); 
    new_dw->get(prNumber,     d_prNo_label, matlIndex, patch, gn, 0); 
    old_dw->get(areaFraction, d_fieldLabels->d_areaFractionLabel, matlIndex, patch, gac, 2); 

    double vol = Dx.x();
#ifdef YDIM
    new_dw->get(vVel,   d_fieldLabels->d_vVelocitySPBCLabel, matlIndex, patch, gac, 1); 
    vol *= Dx.y();
#endif
#ifdef ZDIM
    new_dw->get(wVel,   d_fieldLabels->d_wVelocitySPBCLabel, matlIndex, patch, gac, 1); 
    vol *= Dx.z();
#endif

    new_dw->getModifiable(Fdiff, d_FdiffLabel, matlIndex, patch);
    new_dw->getModifiable(Fconv, d_FconvLabel, matlIndex, patch); 
    new_dw->getModifiable(RHS, d_RHSLabel, matlIndex, patch);


    RHS.initialize(0.0); 
    Fconv.initialize(0.0); 
    Fdiff.initialize(0.0);

    //----CONVECTION
    if (d_doConv)
      d_disc->computeConv( patch, Fconv, oldPhi, uVel, vVel, wVel, den, areaFraction, d_convScheme ); 
  
    //----DIFFUSION
    if (d_doDiff)
      d_disc->computeDiff( patch, Fdiff, oldPhi, mu_t, areaFraction, prNumber, matlIndex, d_eqnName );
 
    //----STORE SOURCES
    vector< constCCVariable<double>* > sourceVars; 
    if( d_addSources ) {
      int z = 0;
      for( vector<const VarLabel*>::iterator src_iter = d_sources.begin(); src_iter != d_sources.end(); ++src_iter, ++z ) {
        sourceVars.push_back( scinew constCCVariable<double> );
        new_dw->get( *sourceVars[z], (*src_iter), matlIndex, patch, gn, 0 );
      }
    }

    //----SUM UP RHS
    for (CellIterator iter=patch->getCellIterator(); !iter.done(); iter++){
      IntVector c = *iter; 

      RHS[c] += Fdiff[c] - Fconv[c];

      //-----SUM UP EXTRA SOURCES
      double source_sum = 0;
      if( d_addSources ) {
        for( vector< constCCVariable<double>* >::iterator iS = sourceVars.begin(); iS != sourceVars.end(); ++iS ) {
          source_sum += (**iS)[c];
        }
      }
      RHS[c] += source_sum*vol;

    }//end cells

    if( d_addSources ) {
      for( vector< constCCVariable<double>* >::iterator iS = sourceVars.begin(); iS != sourceVars.end(); ++iS ) {
        delete *iS;
      }
    }

  }//end patches
}

//---------------------------------------------------------------------------
// Method: Schedule solve the transport equation. 
//---------------------------------------------------------------------------
void
ScalarEqn::sched_solveTransportEqn( const LevelP& level,
                                    SchedulerP& sched, 
                                    int timeSubStep, 
                                    bool lastTimeSubstep )
{
  string taskname = "ScalarEqn::solveTransportEqn";

  Task* tsk = scinew Task(taskname, this, &ScalarEqn::solveTransportEqn, timeSubStep, lastTimeSubstep );

  printSchedule(level,dbg, taskname);

  tsk->modifies(d_transportVarLabel);
  tsk->modifies(d_oldtransportVarLabel); 
  tsk->requires(Task::NewDW, d_RHSLabel, Ghost::None, 0);
  tsk->requires(Task::NewDW, d_fieldLabels->d_densityGuessLabel, Ghost::None, 0);
  tsk->requires(Task::NewDW, d_fieldLabels->d_densityCPLabel, Ghost::None, 0);

  tsk->requires(Task::OldDW, d_fieldLabels->d_sharedState->get_delt_label(), Ghost::None, 0);

  sched->addTask(tsk, level->eachPatch(), d_fieldLabels->d_sharedState->allArchesMaterials());
}

//---------------------------------------------------------------------------
// Method: Actually solve the transport equation. 
//---------------------------------------------------------------------------
void 
ScalarEqn::solveTransportEqn( const ProcessorGroup* pc, 
                              const PatchSubset* patches, 
                              const MaterialSubset* matls, 
                              DataWarehouse* old_dw, 
                              DataWarehouse* new_dw,
                              int timeSubStep,
                              bool lastTimeSubstep )
{
  //patch loop
  for (int p=0; p < patches->size(); p++){

    Ghost::GhostType  gn  = Ghost::None;

    const Patch* patch = patches->get(p);
    int archIndex = 0;
    int matlIndex = d_fieldLabels->d_sharedState->getArchesMaterial(archIndex)->getDWIndex(); 

    delt_vartype DT;
    old_dw->get(DT, d_fieldLabels->d_sharedState->get_delt_label());
    double dt = DT; 

    // Here, j is the rk step and n is the time step.  
    //
    CCVariable<double> phi_at_jp1;   // phi^{(j+1)}
//    CCVariable<double> phi_at_j;     // phi^{(j)}
    constCCVariable<double> rk1_phi; // phi^{n}
    constCCVariable<double> RHS; 
    constCCVariable<double> old_den; 
    constCCVariable<double> new_den; 

    new_dw->getModifiable(phi_at_jp1, d_transportVarLabel,    matlIndex, patch);
//    new_dw->getModifiable(phi_at_j,   d_oldtransportVarLabel, matlIndex, patch);
    old_dw->get(rk1_phi, d_transportVarLabel, matlIndex, patch, gn, 0);
    new_dw->get(RHS, d_RHSLabel, matlIndex, patch, gn, 0);

    new_dw->get(new_den, d_fieldLabels->d_densityGuessLabel, matlIndex, patch, gn, 0); 
    new_dw->get(old_den, d_fieldLabels->d_densityCPLabel, matlIndex, patch, gn, 0);

    // update to get phi^{(j+1)}
    d_timeIntegrator->singlePatchFEUpdate( patch, phi_at_jp1, old_den, new_den, RHS, dt, curr_ssp_time, d_eqnName);

    if (d_doClipping) 
      clipPhi( patch, phi_at_jp1 ); 
//    // Time averaging will occur separately, and later
//    // (see ExplicitSolver::nonLinearSolve)
//
//    if( lastTimeSubstep ) {
//      // The procedure looks like this:
//      // 1. Compute the error between the last RK substeps
//      // 2. Use error to estimate new minimum timeestep
//
//      double new_min_delta_t = 1e16;
//
//      for( CellIterator iter = patch->getCellIterator(); !iter.done(); ++iter ) {
//        IntVector c = *iter;
//        
//        // Step 1: Compute error
//        // error = ( phi^{(1)}-phi^{n} )/delta_t - RHS(\phi^{(1)})
//        double error = (phi_at_j[c] - rk1_phi[c])/dt - RHS[c];
//        double deltat = fabs( phi_at_jp1[c]/(error+TINY) );
//
//        // Step 2: Estimate new min. timestep
//        // min_delta_t_stable = phi^{j+1} / error [=] phi/(phi/time) [=] time
//        if( fabs(error) > TINY ) {
//          new_min_delta_t = min( deltat, new_min_delta_t);
//        }
//        
//      }//end cells
//      
//      new_min_delta_t *= d_timestepMultiplier;
//      EqnFactory& eqnFactory  = EqnFactory::self(); 
//
//      //cout << "Hi from equation " << d_eqnName << ", about to set minimum timestep var to " << new_min_delta_t << endl;
//      if( new_min_delta_t != 0.0 ) {
//        eqnFactory.setMinTimestepVar( d_eqnName, new_min_delta_t );
//      }
//
//    }

  }
}

//---------------------------------------------------------------------------
// Method: Schedule Time averaging 
//---------------------------------------------------------------------------
void
ScalarEqn::sched_timeAveraging( const LevelP& level, 
                                SchedulerP& sched, 
                                int timeSubStep, 
                                bool lastTimeSubstep )
{
  string taskname = "ScalarEqn::timeAveraging";

  Task* tsk = scinew Task(taskname, this, &ScalarEqn::timeAveraging, timeSubStep, lastTimeSubstep);

  grid = level->getGrid();

  //New
  tsk->modifies(d_transportVarLabel);
  tsk->modifies(d_oldtransportVarLabel);
  tsk->requires(Task::NewDW, d_fieldLabels->d_densityCPLabel, Ghost::None, 0);

#ifdef VERIFICATION
  if( lastTimeSubstep ) {
    tsk->modifies(d_MMSErrorLabel);
    tsk->modifies(d_MMSExactLabel);
    tsk->modifies(d_MMSErrorL2Label);
    tsk->modifies(d_MMSErrorLInfLabel);
  }
#endif

  //Old
  tsk->requires(Task::OldDW, d_transportVarLabel, Ghost::None, 0);
  tsk->requires(Task::OldDW, d_fieldLabels->d_densityCPLabel, Ghost::None, 0); 
  tsk->requires(Task::OldDW, d_fieldLabels->d_sharedState->get_delt_label(), Ghost::None, 0);

  sched->addTask(tsk, level->eachPatch(), d_fieldLabels->d_sharedState->allArchesMaterials());
}
//---------------------------------------------------------------------------
// Method: Time averaging 
//---------------------------------------------------------------------------
void 
ScalarEqn::timeAveraging( const ProcessorGroup* pc, 
                          const PatchSubset* patches, 
                          const MaterialSubset* matls, 
                          DataWarehouse* old_dw, 
                          DataWarehouse* new_dw,
                          int timeSubStep,
                          bool lastTimeSubstep )
{
  Vector Ltot = Vector(0.0,0.0,0.0);
  grid->getLength( Ltot );

  //patch loop
  for (int p=0; p < patches->size(); p++){
    Ghost::GhostType  gn  = Ghost::None;

    const Patch* patch = patches->get(p);
    int archIndex = 0;
    int matlIndex = d_fieldLabels->d_sharedState->getArchesMaterial(archIndex)->getDWIndex(); 

    delt_vartype DT;
    old_dw->get(DT, d_fieldLabels->d_sharedState->get_delt_label());
    double dt = DT; 

    // Compute the current RK time. 
    double factor = d_timeIntegrator->time_factor[timeSubStep]; 
    curr_ssp_time = curr_time + factor * dt;

    // j is the rk step and n is the time step.  
    CCVariable<double> new_phi; 
    CCVariable<double> last_rk_phi; 
    constCCVariable<double> old_phi;
    constCCVariable<double> new_den; 
    constCCVariable<double> old_den; 

    new_dw->getModifiable( new_phi, d_transportVarLabel, matlIndex, patch ); 
    new_dw->getModifiable( last_rk_phi, d_oldtransportVarLabel, matlIndex, patch ); 
    old_dw->get( old_phi, d_transportVarLabel, matlIndex, patch, gn, 0 ); 
    new_dw->get( new_den, d_fieldLabels->d_densityCPLabel, matlIndex, patch, gn, 0); 
    old_dw->get( old_den, d_fieldLabels->d_densityCPLabel, matlIndex, patch, gn, 0); 

#ifdef VERIFICATION
    if( lastTimeSubstep ) {
      computeMMSError( patches, patch, 
                       matlIndex, 
                       old_dw, new_dw, 
                       &phi_at_jp1,
                       timeSubStep, 
                       Ltot );
    }
    
    //phi_at_jp1.copyData(phi_at_j);
#endif

    //----Time averaging done here. 
    d_timeIntegrator->timeAvePhi( patch, new_phi, old_phi, new_den, old_den, timeSubStep, curr_ssp_time ); 

    //----BOUNDARY CONDITIONS
    //    must update BCs for next substep
    computeBCs( patch, d_eqnName, new_phi );

    if (d_doClipping) 
      clipPhi( patch, new_phi ); 

    //----COPY averaged phi into oldphi
    last_rk_phi.copyData(new_phi); 

  }

#ifdef VERIFICATION
  // L2 norm of error should be normalized
  // by number of total cells
  int num_cells=0;
  const Patch* patch = patches->get(0);
  num_cells = patch->getNumCells();
  num_cells *= pc->size();
  printMMSError( new_dw, lastTimeSubstep, num_cells );
#endif
}


//---------------------------------------------------------------------------
// Method: Compute the boundary conditions. 
//---------------------------------------------------------------------------
template<class phiType> void
ScalarEqn::computeBCs( const Patch* patch, 
                       string varName,
                       phiType& phi )
{
  d_boundaryCond->setScalarValueBC( 0, patch, phi, varName ); 
}
//---------------------------------------------------------------------------
// Method: Clip the scalar 
//---------------------------------------------------------------------------
template<class phiType> void
ScalarEqn::clipPhi( const Patch* p, 
                       phiType& phi )
{
  // probably should put these "if"s outside the loop   
  for (CellIterator iter=p->getCellIterator(0); !iter.done(); iter++){

    IntVector c = *iter; 

    if (d_doLowClip) {
      if (phi[c] < d_lowClip) 
        phi[c] = d_lowClip; 
    }

    if (d_doHighClip) { 
      if (phi[c] > d_highClip) 
        phi[c] = d_highClip; 
    } 
  }
}

//---------------------------------------------------------------------------
// Method: Schedule dummy initialization
//---------------------------------------------------------------------------
void
ScalarEqn::sched_dummyInit( const LevelP& level, SchedulerP& sched )
{
  string taskname = "ScalarEqn::dummyInit"; 

  Task* tsk = scinew Task(taskname, this, &ScalarEqn::dummyInit);

  printSchedule(level,dbg, taskname);

  Ghost::GhostType  gn = Ghost::None;

  tsk->requires(Task::OldDW, d_transportVarLabel, gn, 0); 
  tsk->computes(d_transportVarLabel);
  tsk->computes(d_oldtransportVarLabel); 
  tsk->computes(d_FconvLabel); 
  tsk->computes(d_FdiffLabel); 
  tsk->computes(d_RHSLabel); 
  tsk->computes(d_prNo_label); 

#ifdef VERIFICATION
  tsk->computes(d_MMSErrorLabel);
  tsk->computes(d_MMSExactLabel);
#endif

  sched->addTask(tsk, level->eachPatch(), d_fieldLabels->d_sharedState->allArchesMaterials());

}

void 
ScalarEqn::dummyInit( const ProcessorGroup* pc, 
                     const PatchSubset* patches, 
                     const MaterialSubset* matls, 
                     DataWarehouse* old_dw, 
                     DataWarehouse* new_dw )
{
  //patch loop
  for (int p=0; p < patches->size(); p++){

    const Patch* patch = patches->get(p);
    int archIndex = 0;
    int matlIndex = d_fieldLabels->d_sharedState->getArchesMaterial(archIndex)->getDWIndex(); 

    CCVariable<double> phi; 
    CCVariable<double> rkold_phi;
    CCVariable<double> RHS; 
    CCVariable<double> Fconv; 
    CCVariable<double> Fdiff; 
    CCVariable<double> PrNumber; 
    constCCVariable<double> old_phi; 

    new_dw->allocateAndPut( phi, d_transportVarLabel, matlIndex, patch ); 
    new_dw->allocateAndPut( rkold_phi, d_oldtransportVarLabel, matlIndex, patch ); 
    new_dw->allocateAndPut( RHS, d_RHSLabel, matlIndex, patch); 
    new_dw->allocateAndPut( Fconv, d_FconvLabel, matlIndex, patch); 
    new_dw->allocateAndPut( Fdiff, d_FdiffLabel, matlIndex, patch); 
    new_dw->allocateAndPut( PrNumber, d_prNo_label, matlIndex, patch); 

    old_dw->get( old_phi, d_transportVarLabel, matlIndex, patch, Ghost::None, 0); 

    Fconv.initialize(0.0); 
    Fdiff.initialize(0.0); 
    RHS.initialize(0.0);
    phi.initialize(0.0); 
    rkold_phi.initialize(0.0); 
    PrNumber.initialize(0.0);

    phi.copyData(old_phi);

#ifdef VERIFICATION
    CCVariable<double> MMSError;
    new_dw->allocateAndPut( MMSError, d_MMSErrorLabel, matlIndex, patch );
    MMSError.initialize(0.0);

    CCVariable<double> MMSExact;
    new_dw->allocateAndPut( MMSExact, d_MMSExactLabel, matlIndex, patch );
    MMSExact.initialize(0.0);
#endif

  }
}

#ifdef VERIFICATION

/** @details
This method computes the MMS error associated with the computed solution.
It calculates the \f$L_{\infty}\f$ and \f$L_2\f$ norm of the error.

The error in the computed solution of a field \f$ \phi \f$ is defined as:

\f[
\epsilon = \phi_{e} - \phi_{M}
\f] 

where \f$ \phi_{e} \f$ is the exact solution and \f$ \phi_{M} \f$ is the 
computational model's approximation of the solution.

The error is a vector, with one value for each cell.

The norms are defined as:

\f[
L_{\infty} = \mbox{max} ( \boldsymbol{\epsilon} )
\f]

and 

\f[
L_{2} = \left( \sum_{i=1}^{N_{cells}} \epsilon_{i}^{2} \right)^{\frac{1}{2}}
\f]

*/
void 
ScalarEqn::computeMMSError( const PatchSubset* patches, 
                            const Patch* patch,
                            const int matlIndex, 
                            DataWarehouse* old_dw, 
                            DataWarehouse* new_dw,
                            CCVariable<double>* phi_at_jp1,
                            int timeSubStep,
                            Vector domain_size )
{
  CCVariable<double> abs_error;
  new_dw->getModifiable( abs_error, d_MMSErrorLabel, matlIndex, patch );

  CCVariable<double> exact;
  new_dw->getModifiable( exact, d_MMSExactLabel, matlIndex, patch );

  sum_vartype LInfErrNorm;
  new_dw->get(LInfErrNorm, d_MMSErrorLInfLabel);
  double LInfErrorNorm = LInfErrNorm;

  sum_vartype L2ErrNorm;
  new_dw->get(L2ErrNorm, d_MMSErrorL2Label);
  double L2ErrorNorm = L2ErrNorm;

  if( d_initFunction == "mms_x" || d_initFunction == "MMS_X" ) {
    for (CellIterator iter=patch->getCellIterator(); !iter.done(); iter++){
      double x,y,z,computed;
      IntVector c = *iter; 
      Vector Dx = patch->dCell(); 
      x = c[0]*Dx.x() + Dx.x()/2.; 
      y = c[1]*Dx.y() + Dx.y()/2.; 
      z = c[2]*Dx.z() + Dx.z()/2.; 

      exact[c] = MMS_X::evaluate_MMS( x, y, z, domain_size );
      computed = (*phi_at_jp1)[c];
      abs_error[c] = fabs( exact[c] - computed );

      LInfErrorNorm = max( LInfErrorNorm, abs_error[c] );
      L2ErrorNorm += abs_error[c]*abs_error[c];
    }
  }

  if( d_initFunction == "mms_y" || d_initFunction == "MMS_Y" ) {
    for (CellIterator iter=patch->getCellIterator(); !iter.done(); iter++){
      double x,y,z,computed;
      IntVector c = *iter; 
      Vector Dx = patch->dCell(); 
      x = c[0]*Dx.x() + Dx.x()/2.; 
      y = c[1]*Dx.y() + Dx.y()/2.; 
      z = c[2]*Dx.z() + Dx.z()/2.; 

      exact[c] = MMS_Y::evaluate_MMS( x, y, z, domain_size );
      computed = (*phi_at_jp1)[c];
      abs_error[c] = fabs( exact[c] - computed );

      LInfErrorNorm = max( LInfErrorNorm, abs_error[c] );
      L2ErrorNorm += abs_error[c]*abs_error[c];
    }
  }

  if( d_initFunction == "mms_z" || d_initFunction == "MMS_Z" ) {
    for (CellIterator iter=patch->getCellIterator(); !iter.done(); iter++){
      double x,y,z,computed;
      IntVector c = *iter; 
      Vector Dx = patch->dCell(); 
      x = c[0]*Dx.x() + Dx.x()/2.; 
      y = c[1]*Dx.y() + Dx.y()/2.; 
      z = c[2]*Dx.z() + Dx.z()/2.; 

      exact[c] = MMS_Z::evaluate_MMS( x, y, z, domain_size );
      computed = (*phi_at_jp1)[c];
      abs_error[c] = fabs( exact[c] - computed );

      LInfErrorNorm = max( LInfErrorNorm, abs_error[c] );
      L2ErrorNorm += abs_error[c]*abs_error[c];
    }
  }

  if( d_initFunction == "mms_xyz" || d_initFunction == "MMS_XYZ" ) {
    for (CellIterator iter=patch->getCellIterator(); !iter.done(); iter++){
      double x,y,z,computed;
      IntVector c = *iter; 
      Vector Dx = patch->dCell(); 
      x = c[0]*Dx.x() + Dx.x()/2.; 
      y = c[1]*Dx.y() + Dx.y()/2.; 
      z = c[2]*Dx.z() + Dx.z()/2.;

      exact[c] = MMS_XYZ::evaluate_MMS( x, y, z, domain_size ); 
      computed = (*phi_at_jp1)[c];
      abs_error[c] = fabs( exact[c] - computed );

      LInfErrorNorm = max( LInfErrorNorm, abs_error[c] );
      L2ErrorNorm += abs_error[c]*abs_error[c];
    }
  }

  L2ErrorNorm = sqrt(L2ErrorNorm);

  new_dw->put( sum_vartype(LInfErrorNorm), d_MMSErrorLInfLabel );
  new_dw->put( sum_vartype(L2ErrorNorm),   d_MMSErrorL2Label );
}

void
ScalarEqn::printMMSError( DataWarehouse* new_dw,
                          bool lastTimeSubstep,
                          int num_cells )
{
  if( lastTimeSubstep ) {
    sum_vartype LInfErrNorm;
    new_dw->get(LInfErrNorm, d_MMSErrorLInfLabel);
    sum_vartype L2ErrNorm;
    new_dw->get(L2ErrNorm, d_MMSErrorL2Label);

    double L2ErrNorm_Normalized = L2ErrNorm;
    L2ErrNorm_Normalized /= num_cells;
    new_dw->put(sum_vartype(L2ErrNorm_Normalized), d_MMSErrorL2Label);

    proc0cout << endl;
    proc0cout << "***********************************************" << endl;
    proc0cout << "MMS Error Calculation: " << d_initFunction << endl << endl;
    proc0cout << "Absolute error = [exact solution] - [computed solution]" << endl;
    proc0cout << "L-inf norm of absolute error: \t" << LInfErrNorm << endl;
    proc0cout << "L-2 norm of absolute error: \t" << L2ErrNorm_Normalized << endl;
    proc0cout << "Num cells = " << num_cells << endl;
    proc0cout << "***********************************************" << endl;
    proc0cout << endl;
  }
}

#endif

