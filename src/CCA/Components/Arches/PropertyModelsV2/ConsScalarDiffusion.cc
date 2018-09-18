#include <CCA/Components/Arches/PropertyModelsV2/ConsScalarDiffusion.h>
#include <CCA/Components/Arches/UPSHelper.h>

using namespace Uintah;
using namespace ArchesCore;

//--------------------------------------------------------------------------------------------------
ConsScalarDiffusion::ConsScalarDiffusion( std::string task_name, int matl_index ) :
TaskInterface( task_name, matl_index ){}

//--------------------------------------------------------------------------------------------------
ConsScalarDiffusion::~ConsScalarDiffusion(){}

//--------------------------------------------------------------------------------------------------
TaskAssignedExecutionSpace ConsScalarDiffusion::loadTaskComputeBCsFunctionPointers()
{
  return create_portable_arches_tasks<TaskInterface::BC>( this
                                     , &ConsScalarDiffusion::compute_bcs<UINTAH_CPU_TAG>     // Task supports non-Kokkos builds
                                     , &ConsScalarDiffusion::compute_bcs<KOKKOS_OPENMP_TAG>  // Task supports Kokkos::OpenMP builds
                                     , &ConsScalarDiffusion::compute_bcs<KOKKOS_CUDA_TAG>    // Task supports Kokkos::Cuda builds
                                     );
}

//--------------------------------------------------------------------------------------------------
TaskAssignedExecutionSpace ConsScalarDiffusion::loadTaskInitializeFunctionPointers()
{
  return create_portable_arches_tasks<TaskInterface::INITIALIZE>( this
                                     , &ConsScalarDiffusion::initialize<UINTAH_CPU_TAG>     // Task supports non-Kokkos builds
                                     , &ConsScalarDiffusion::initialize<KOKKOS_OPENMP_TAG>  // Task supports Kokkos::OpenMP builds
                                     , &ConsScalarDiffusion::initialize<KOKKOS_CUDA_TAG>    // Task supports Kokkos::Cuda builds
                                     );
}

//--------------------------------------------------------------------------------------------------
TaskAssignedExecutionSpace ConsScalarDiffusion::loadTaskEvalFunctionPointers()
{
  return create_portable_arches_tasks<TaskInterface::TIMESTEP_EVAL>( this
                                     , &ConsScalarDiffusion::eval<UINTAH_CPU_TAG>     // Task supports non-Kokkos builds
                                     , &ConsScalarDiffusion::eval<KOKKOS_OPENMP_TAG>  // Task supports Kokkos::OpenMP builds
                                     , &ConsScalarDiffusion::eval<KOKKOS_CUDA_TAG>    // Task supports Kokkos::Cuda builds
                                     );
}

TaskAssignedExecutionSpace ConsScalarDiffusion::loadTaskTimestepInitFunctionPointers()
{
  return create_portable_arches_tasks<TaskInterface::TIMESTEP_INITIALIZE>( this
                                     , &ConsScalarDiffusion::timestep_init<UINTAH_CPU_TAG>     // Task supports non-Kokkos builds
                                     , &ConsScalarDiffusion::timestep_init<KOKKOS_OPENMP_TAG>  // Task supports Kokkos::OpenMP builds
                                     , &ConsScalarDiffusion::timestep_init<KOKKOS_CUDA_TAG>  // Task supports Kokkos::OpenMP builds
                                     );
}

TaskAssignedExecutionSpace ConsScalarDiffusion::loadTaskRestartInitFunctionPointers()
{
  return  TaskAssignedExecutionSpace::NONE_EXECUTION_SPACE;
}

//--------------------------------------------------------------------------------------------------
void ConsScalarDiffusion::problemSetup( ProblemSpecP& db ){

  m_density_name        = parse_ups_for_role( DENSITY, db, "density" );
  m_turb_viscosity_name = "turb_viscosity";
  m_gamma_name          = _task_name;
  db->require("D_mol", m_Diffusivity);
  db->getWithDefault("turbulentPrandtlNumber", m_Pr, 0.4);
}

//--------------------------------------------------------------------------------------------------
void ConsScalarDiffusion::create_local_labels(){

  register_new_variable<CCVariable<double> >( m_gamma_name);

}

//--------------------------------------------------------------------------------------------------
void ConsScalarDiffusion::register_initialize( AVarInfo& variable_registry , const bool pack_tasks){

  typedef ArchesFieldContainer AFC;

  register_variable( m_gamma_name,          AFC::COMPUTES, variable_registry, _task_name );
  //register_variable( m_turb_viscosity_name, AFC::REQUIRES, 0, AFC::NEWDW, variable_registry ,_task_name );
  //register_variable( m_density_name,        AFC::REQUIRES, 0, AFC::NEWDW, variable_registry ,_task_name );

}

//--------------------------------------------------------------------------------------------------
template<typename ExecutionSpace, typename MemSpace>
void ConsScalarDiffusion::initialize( const Patch* patch, ArchesTaskInfoManager* tsk_info, ExecutionObject<ExecutionSpace, MemSpace>& executionObject ){


  CCVariable<double>& gamma = tsk_info->get_uintah_field_add<CCVariable<double> >(m_gamma_name);
  //constCCVariable<double>& mu_t    = tsk_info->get_const_uintah_field_add<constCCVariable<double> >(m_turb_viscosity_name);
  //constCCVariable<double>& density = tsk_info->get_const_uintah_field_add<constCCVariable<double> >(m_density_name);

  gamma.initialize(0.0);

  //Uintah::BlockRange range( patch->getCellLowIndex(), patch->getCellHighIndex() );
  //Uintah::parallel_for( range, [&](int i, int j, int k){
   //gamma(i,j,k) = density(i,j,k)*m_Diffusivity + mu_t(i,j,k)/m_Pr;
  //});
}

//--------------------------------------------------------------------------------------------------
void ConsScalarDiffusion::register_timestep_init( AVarInfo& variable_registry , const bool pack_tasks){


}

//--------------------------------------------------------------------------------------------------
template<typename ExecutionSpace, typename MemSpace> void
ConsScalarDiffusion::timestep_init( const Patch* patch, ArchesTaskInfoManager* tsk_info, ExecutionObject<ExecutionSpace, MemSpace>& executionObject ){


}

//--------------------------------------------------------------------------------------------------
void ConsScalarDiffusion::register_timestep_eval( VIVec& variable_registry, const int time_substep , const bool packed_tasks){

  typedef ArchesFieldContainer AFC;

  register_variable( m_gamma_name,          AFC::COMPUTES, variable_registry, _task_name );
  register_variable( m_turb_viscosity_name, AFC::REQUIRES, 0, AFC::NEWDW, variable_registry, time_substep ,_task_name );
  register_variable( m_density_name,        AFC::REQUIRES, 0, AFC::NEWDW, variable_registry, time_substep ,_task_name );

}

//--------------------------------------------------------------------------------------------------
template<typename ExecutionSpace, typename MemSpace>
void ConsScalarDiffusion::eval( const Patch* patch, ArchesTaskInfoManager* tsk_info, ExecutionObject<ExecutionSpace, MemSpace>& exObj ){

  auto gamma = tsk_info->get_uintah_field_add<CCVariable<double>, double, MemSpace >(m_gamma_name);
  auto  mu_t    = tsk_info->get_const_uintah_field_add<constCCVariable<double>,const double , MemSpace >(m_turb_viscosity_name);
  auto  density = tsk_info->get_const_uintah_field_add<constCCVariable<double>,const double , MemSpace >(m_density_name);

  parallel_initialize(exObj, 0.0, gamma);
  const double  PrNo= m_Pr;
  const double molecular_diffusivity =m_Diffusivity;
  Uintah::BlockRange range( patch->getCellLowIndex(), patch->getCellHighIndex() );
  Uintah::parallel_for( exObj, range, KOKKOS_LAMBDA (int i, int j, int k){
   gamma(i,j,k) = density(i,j,k)*molecular_diffusivity + mu_t(i,j,k)/PrNo;
  });

}

