#include <Core/ProblemSpec/ProblemSpec.h>
#include <CCA/Ports/Scheduler.h>
#include <Core/Grid/SimulationState.h>
#include <Core/Grid/Variables/VarTypes.h>
#include <Core/Grid/Variables/CCVariable.h>
#include <CCA/Components/Arches/SourceTerms/UnweightedSrcTerm.h>
#include <CCA/Components/Arches/TransportEqns/DQMOMEqnFactory.h>
#include <CCA/Components/Arches/TransportEqns/EqnBase.h>
#include <CCA/Components/Arches/TransportEqns/DQMOMEqn.h>
#include <CCA/Components/Arches/ArchesLabel.h>

//===========================================================================

using namespace std;
using namespace Uintah; 

//---------------------------------------------------------------------------
// Builder:
UnweightedSrcTermBuilder::UnweightedSrcTermBuilder(std::string srcName, 
                                         vector<std::string> reqLabelNames, 
                                         SimulationStateP& sharedState)
: SourceTermBuilder(srcName, reqLabelNames, sharedState)
{}

UnweightedSrcTermBuilder::~UnweightedSrcTermBuilder(){}

SourceTermBase*
UnweightedSrcTermBuilder::build(){
  return scinew UnweightedSrcTerm( d_srcName, d_sharedState, d_requiredLabels );
}
// End Builder
//---------------------------------------------------------------------------

UnweightedSrcTerm::UnweightedSrcTerm( std::string srcName, SimulationStateP& sharedState,
                            vector<std::string> reqLabelNames ) 
: SourceTermBase(srcName, sharedState, reqLabelNames)
{}

UnweightedSrcTerm::~UnweightedSrcTerm()
{}
//---------------------------------------------------------------------------
// Method: Problem Setup
//---------------------------------------------------------------------------
void 
UnweightedSrcTerm::problemSetup(const ProblemSpecP& inputdb)
{

  ProblemSpecP db = inputdb; 

}
//---------------------------------------------------------------------------
// Method: Schedule the calculation of the source term 
//---------------------------------------------------------------------------
void 
UnweightedSrcTerm::sched_computeSource( const LevelP& level, SchedulerP& sched, int timeSubStep )
{
  std::string taskname = "UnweightedSrcTerm::eval";
  Task* tsk = scinew Task(taskname, this, &UnweightedSrcTerm::computeSource, timeSubStep);

  if (timeSubStep == 0 && !d_labelSchedInit) {
    // Every source term needs to set this flag after the varLabel is computed. 
    // transportEqn.cleanUp should reinitialize this flag at the end of the time step. 
    d_labelSchedInit = true;

    tsk->computes(d_srcLabel);
  } else {
    tsk->modifies(d_srcLabel); 
  }

  DQMOMEqnFactory& dqmomFactory  = DQMOMEqnFactory::self();

  for (vector<std::string>::iterator iter = d_requiredLabels.begin(); 
       iter != d_requiredLabels.end(); iter++) { 

    std::string label_name = (*iter);
    EqnBase& eqn = dqmomFactory.retrieve_scalar_eqn( label_name );

    const VarLabel* unwaLabel = eqn.getTransportEqnLabel();
    tsk->requires( Task::OldDW, unwaLabel, Ghost::None, 0 );

    //DQMOMEqnFactory::EqnMap& dqmom_eqns = dqmomFactory.retrieve_all_eqns();

    //for (DQMOMEqnFactory::EqnMap::iterator ieqn=dqmom_eqns.begin();
    //     ieqn != dqmom_eqns.end(); ieqn++){
          
    DQMOMEqn* dqmom_eqn = dynamic_cast<DQMOMEqn*>(&eqn);
    int d_quadNode = dqmom_eqn->getQuadNode(); 

    // require particle velocity
    string partVel_name = "vel_qn";
    std::string node;
    std::stringstream out;
    out << d_quadNode;
    node = out.str();
    partVel_name += node;

    const VarLabel* partVelLabel = VarLabel::find( partVel_name );
    tsk->requires( Task::NewDW, partVelLabel, Ghost::AroundCells, 1 );

  }

  sched->addTask(tsk, level->eachPatch(), d_sharedState->allArchesMaterials()); 

}
//---------------------------------------------------------------------------
// Method: Actually compute the source term 
//---------------------------------------------------------------------------
void
UnweightedSrcTerm::computeSource( const ProcessorGroup* pc, 
                   const PatchSubset* patches, 
                   const MaterialSubset* matls, 
                   DataWarehouse* old_dw, 
                   DataWarehouse* new_dw, 
                   int timeSubStep )
{
  //patch loop
  for (int p=0; p < patches->size(); p++){

    const Patch* patch = patches->get(p);
    int archIndex = 0;
    int matlIndex = d_sharedState->getArchesMaterial(archIndex)->getDWIndex(); 

    CCVariable<double> constSrc; 
    if ( new_dw->exists(d_srcLabel, matlIndex, patch ) ){
      new_dw->getModifiable( constSrc, d_srcLabel, matlIndex, patch ); 
      constSrc.initialize(0.0);
    } else {
      new_dw->allocateAndPut( constSrc, d_srcLabel, matlIndex, patch );
      constSrc.initialize(0.0);
    } 

    DQMOMEqnFactory& dqmomFactory  = DQMOMEqnFactory::self();
    constCCVariable<double> unwa;
    constCCVariable<Vector> partVel;
    std::string label_name;

    for (vector<std::string>::iterator iter = d_requiredLabels.begin(); 
         iter != d_requiredLabels.end(); iter++) { 
   
      label_name = (*iter);
      EqnBase& eqn = dqmomFactory.retrieve_scalar_eqn( label_name );

      const VarLabel* unwaLabel = eqn.getTransportEqnLabel();
      old_dw->get( unwa, unwaLabel, matlIndex, patch, Ghost::None, 0 );

      DQMOMEqn* dqmom_eqn = dynamic_cast<DQMOMEqn*>(&eqn);
      int d_quadNode = dqmom_eqn->getQuadNode();

      //ArchesLabel::PartVelMap::const_iterator iter = d_fieldLabels->partVel.find(d_quadNode);
      //old_dw->get(partVel, iter->second, matlIndex, patch, Ghost::None, 0 );
 
      string partVel_name = "vel_qn";
      std::string node;
      std::stringstream out;
      out << d_quadNode;
      node = out.str();
      partVel_name += node;

      const VarLabel* partVelLabel = VarLabel::find( partVel_name );
      new_dw->get(partVel, partVelLabel, matlIndex, patch, Ghost::AroundCells, 1 );
    }


    Vector Dx = patch->dCell();

    for (CellIterator iter=patch->getCellIterator(); !iter.done(); iter++){
      IntVector c = *iter;
      IntVector cxm = c - IntVector(1,0,0);
      IntVector cxp = c + IntVector(1,0,0);
      IntVector cym = c - IntVector(0,1,0);
      IntVector cyp = c + IntVector(0,1,0);
      IntVector czm = c - IntVector(0,0,1);
      IntVector czp = c + IntVector(0,0,1);
 
      constSrc[c] += unwa[c]*( (partVel[cxp].x()-partVel[cxm].x())/(2*Dx.x()) +
                               (partVel[cyp].y()-partVel[cym].y())/(2*Dx.y()) +
                               (partVel[czp].z()-partVel[czm].z())/(2*Dx.z()) );
      //cout << "label_name " << label_name << endl;
      //cout << "unwa " << unwa[c] << endl;
      //cout << "partvel " << partVel[cxp].x() << " " << partVel[cxm].x() << endl;
      //cout << "Dx " << Dx.x() << endl; 
    }
  }
}

//---------------------------------------------------------------------------
// Method: Schedule dummy initialization
//---------------------------------------------------------------------------
void
UnweightedSrcTerm::sched_dummyInit( const LevelP& level, SchedulerP& sched )
{
  string taskname = "UnweightedSrcTerm::dummyInit"; 

  Task* tsk = scinew Task(taskname, this, &UnweightedSrcTerm::dummyInit);

  tsk->computes(d_srcLabel);

  for (std::vector<const VarLabel*>::iterator iter = d_extraLocalLabels.begin(); iter != d_extraLocalLabels.end(); iter++){
    tsk->computes(*iter); 
  }

  sched->addTask(tsk, level->eachPatch(), d_sharedState->allArchesMaterials());

}
void 
UnweightedSrcTerm::dummyInit( const ProcessorGroup* pc, 
                      const PatchSubset* patches, 
                      const MaterialSubset* matls, 
                      DataWarehouse* old_dw, 
                      DataWarehouse* new_dw )
{
  //patch loop
  for (int p=0; p < patches->size(); p++){

    const Patch* patch = patches->get(p);
    int archIndex = 0;
    int matlIndex = d_sharedState->getArchesMaterial(archIndex)->getDWIndex(); 

    CCVariable<double> src;

    new_dw->allocateAndPut( src, d_srcLabel, matlIndex, patch ); 

    src.initialize(0.0); 

    for (std::vector<const VarLabel*>::iterator iter = d_extraLocalLabels.begin(); iter != d_extraLocalLabels.end(); iter++){
      CCVariable<double> tempVar; 
      new_dw->allocateAndPut(tempVar, *iter, matlIndex, patch ); 
    }
  }
}

