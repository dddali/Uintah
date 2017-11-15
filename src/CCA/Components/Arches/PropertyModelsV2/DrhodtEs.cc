#include <CCA/Components/Arches/PropertyModelsV2/DrhodtEs.h>
#include <CCA/Components/Arches/KokkosTools.h>
#include <CCA/Components/Arches/UPSHelper.h>


namespace Uintah{

//--------------------------------------------------------------------------------------------------
DrhodtEs::DrhodtEs( std::string task_name, int matl_index ) :
TaskInterface( task_name, matl_index ) {
}

//--------------------------------------------------------------------------------------------------
DrhodtEs::~DrhodtEs(){
}

//--------------------------------------------------------------------------------------------------
void
DrhodtEs::problemSetup( ProblemSpecP& db ){

  //db->findBlock("KMomentum")->findBlock("drhodt")->getAttribute("label",m_label_drhodt);
  m_label_drhodt = "drhodt";
  using namespace ArchesCore;
  m_label_density = parse_ups_for_role( DENSITY, db, "density" );

}

//--------------------------------------------------------------------------------------------------
void
DrhodtEs::create_local_labels(){
  register_new_variable<CCVariable<double> >( m_label_drhodt );

}

//--------------------------------------------------------------------------------------------------
void
DrhodtEs::register_initialize( std::vector<ArchesFieldContainer::VariableInformation>&
                                       variable_registry, const bool packed_tasks ){
  register_variable( m_label_drhodt , ArchesFieldContainer::COMPUTES, variable_registry );
}

//--------------------------------------------------------------------------------------------------
void
DrhodtEs::initialize( const Patch* patch, ArchesTaskInfoManager* tsk_info ){

  CCVariable<double>& drhodt = tsk_info->get_uintah_field_add<CCVariable<double> >( m_label_drhodt );
  drhodt.initialize(0.0);
}

//--------------------------------------------------------------------------------------------------
void
DrhodtEs::register_timestep_init( std::vector<ArchesFieldContainer::VariableInformation>&
                                          variable_registry, const bool packed_tasks ){

}

//--------------------------------------------------------------------------------------------------
void
DrhodtEs::timestep_init( const Patch* patch, ArchesTaskInfoManager* tsk_info ){

}

//--------------------------------------------------------------------------------------------------
void
DrhodtEs::register_timestep_eval( std::vector<ArchesFieldContainer::VariableInformation>&
                                          variable_registry, const int time_substep,
                                          const bool packed_tasks ){
  register_variable( m_label_density , ArchesFieldContainer::REQUIRES, 0, ArchesFieldContainer::NEWDW, variable_registry, time_substep );
  register_variable( m_label_density , ArchesFieldContainer::REQUIRES, 0, ArchesFieldContainer::OLDDW, variable_registry, time_substep );
  register_variable( m_label_drhodt , ArchesFieldContainer::COMPUTES,  variable_registry, time_substep );

}

//--------------------------------------------------------------------------------------------------
void
DrhodtEs::eval( const Patch* patch, ArchesTaskInfoManager* tsk_info ){

  constCCVariable<double>& rho = tsk_info->get_const_uintah_field_add<constCCVariable<double> >( m_label_density );
  constCCVariable<double>& old_rho = tsk_info->get_const_uintah_field_add<constCCVariable<double> >( m_label_density, ArchesFieldContainer::OLDDW);
  
  CCVariable<double>& drhodt = tsk_info->get_uintah_field_add<CCVariable<double> >( m_label_drhodt );
  drhodt.initialize(0.0); 
  const double dt = tsk_info->get_dt();
  Uintah::BlockRange range(patch->getExtraCellLowIndex(), patch->getExtraCellHighIndex() );
  Uintah::parallel_for( range, [&](int i, int j, int k){
    drhodt(i,j,k)   = (rho(i,j,k) - old_rho(i,j,k))/dt;
  });
}
//--------------------------------------------------------------------------------------------------

} //namespace Uintah
