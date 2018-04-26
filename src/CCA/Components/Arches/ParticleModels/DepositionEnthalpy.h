#ifndef Uintah_Component_Arches_DepositionEnthalpy_h
#define Uintah_Component_Arches_DepositionEnthalpy_h

#include <CCA/Components/Arches/Task/TaskInterface.h>
#include <Core/Grid/SimulationState.h>

namespace Uintah{

  class Operators;
  class DepositionEnthalpy : public TaskInterface {

public:

    DepositionEnthalpy( std::string task_name, int matl_index, const int N, SimulationStateP shared_state );
    ~DepositionEnthalpy();

    TaskAssignedExecutionSpace loadTaskEvalFunctionPointers();

    void problemSetup( ProblemSpecP& db );

    void register_initialize( std::vector<ArchesFieldContainer::VariableInformation>& variable_registry , const bool packed_tasks);

    void register_timestep_init( std::vector<ArchesFieldContainer::VariableInformation>& variable_registry , const bool packed_tasks){}

    void register_timestep_eval( std::vector<ArchesFieldContainer::VariableInformation>& variable_registry, const int time_substep , const bool packed_tasks);

    void register_compute_bcs( std::vector<ArchesFieldContainer::VariableInformation>& variable_registry, const int time_substep , const bool packed_tasks){};

    void compute_bcs( const Patch* patch, ArchesTaskInfoManager* tsk_info ){}

    void initialize( const Patch* patch, ArchesTaskInfoManager* tsk_info );

    void timestep_init( const Patch* patch, ArchesTaskInfoManager* tsk_info ){}

    template <typename EXECUTION_SPACE, typename MEMORY_SPACE>
    void eval( const Patch* patch, ArchesTaskInfoManager* tsk_info, void* stream );

    void create_local_labels();

    const std::string get_env_name( const int i, const std::string base_name ){
      std::stringstream out;
      std::string env;
      out << i;
      env = out.str();
      return base_name + "_" + env;
    }


    //Build instructions for this (DepositionEnthalpy) class.
    class Builder : public TaskInterface::TaskBuilder {

      public:

      Builder( std::string task_name, int matl_index, const int N, SimulationStateP shared_state ) : _task_name(task_name), _matl_index(matl_index), _Nenv(N), _shared_state(shared_state){}
      ~Builder(){}

      DepositionEnthalpy* build()
      { return scinew DepositionEnthalpy( _task_name, _matl_index, _Nenv, _shared_state ); }

      private:

      std::string _task_name;
      int _matl_index;
      int _Nenv;
      SimulationStateP _shared_state;

    };

private:
      int _Nenv;
      SimulationStateP _shared_state;
      std::vector<IntVector> _d;
      std::vector<IntVector> _fd;
      std::string _cellType_name;
      std::string _ratedepx_name;
      std::string _ratedepy_name;
      std::string _ratedepz_name;
      std::string _ash_enthalpy_src;
      std::vector<double> _mass_ash;
      double _Ha0;
      std::string _diameter_base_name;
      std::string _temperature_base_name;
      std::string _gasT_name;
      std::string _density_base_name;

  };
}
#endif
