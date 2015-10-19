/*
 * The MIT License
 *
 * Copyright (c) 1997-2015 The University of Utah
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <CCA/Components/Arches/KokkosSandboxSolver.h>
#include <CCA/Components/Arches/Task/TaskFactoryBase.h>

using namespace Uintah;
typedef std::map<std::string, boost::shared_ptr<TaskFactoryBase> > BFM;
typedef std::vector<std::string> SVec;

KokkosSandboxSolver::KokkosSandboxSolver( SimulationStateP& shared_state,
  const ProcessorGroup* myworld,
  std::map<std::string,
  boost::shared_ptr<TaskFactoryBase> >& task_factory_map)
  : _shared_state(shared_state), _task_factory_map(task_factory_map), NonlinearSolver( myworld )
{}

KokkosSandboxSolver::~KokkosSandboxSolver(){}


void
KokkosSandboxSolver::sched_restartInitialize( const LevelP& level, SchedulerP& sched )
{}

void
KokkosSandboxSolver::sched_restartInitializeTimeAdvance( const LevelP& level, SchedulerP& sched )
{}

void
KokkosSandboxSolver::problemSetup( const ProblemSpecP& input_db,
                                   SimulationStateP& state,
                                   GridP& grid )
{}

int
KokkosSandboxSolver::nonlinearSolve( const LevelP& level,
                                     SchedulerP& sched )
{

  const MaterialSet* matls = _shared_state->allArchesMaterials();

  BFM::iterator i_transport = _task_factory_map.find("transport_factory");
  TaskFactoryBase::TaskMap all_tasks = i_transport->second->retrieve_all_tasks();
  for ( TaskFactoryBase::TaskMap::iterator i = all_tasks.begin(); i != all_tasks.end(); i++){
    i->second->schedule_timestep_init(level, sched, matls);
  }

  //(vel)
  SVec mom_rhs_builders = i_transport->second->retrieve_task_subset("mom_rhs_builders");
  for ( SVec::iterator i = mom_rhs_builders.begin(); i != mom_rhs_builders.end(); i++){
    TaskInterface* tsk = i_transport->second->retrieve_task(*i);
    tsk->schedule_task(level, sched, matls, TaskInterface::STANDARD_TASK, 0);
  }
  
  //(scalars)
  SVec scalar_rhs_builders = i_transport->second->retrieve_task_subset("scalar_rhs_builders");
  for ( SVec::iterator i = scalar_rhs_builders.begin(); i != scalar_rhs_builders.end(); i++){
    TaskInterface* tsk = i_transport->second->retrieve_task(*i);
    tsk->schedule_task(level, sched, matls, TaskInterface::STANDARD_TASK, 0);
  }


  return 0;
}

void
KokkosSandboxSolver::sched_setInitialGuess( SchedulerP&,
                                            const PatchSet* patches,
                                            const MaterialSet* matls)
{}

void
KokkosSandboxSolver::initialize( const LevelP& level, SchedulerP& sched, const bool doing_restart )
{
  const MaterialSet* matls = _shared_state->allArchesMaterials();

  BFM::iterator i_trans_fac = _task_factory_map.find("transport_factory");
  TaskFactoryBase::TaskMap all_tasks = i_trans_fac->second->retrieve_all_tasks();
  for ( TaskFactoryBase::TaskMap::iterator i = all_tasks.begin(); i != all_tasks.end(); i++) {
    i->second->schedule_init(level, sched, matls, doing_restart);
  }
}