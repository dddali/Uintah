/*
 * The MIT License
 *
 * Copyright (c) 1997-2016 The University of Utah
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

#include <CCA/Components/Schedulers/MPIScheduler.h>
#include <CCA/Components/Schedulers/OnDemandDataWarehouse.h>
#include <CCA/Components/Schedulers/SendState.h>
#include <CCA/Components/Schedulers/DetailedTasks.h>
#include <CCA/Components/Schedulers/TaskGraph.h>
#include <CCA/Ports/LoadBalancer.h>
#include <CCA/Ports/Output.h>

#include <Core/Grid/Variables/ParticleSubset.h>
#include <Core/Grid/Variables/ComputeSet.h>
#include <Core/Malloc/Allocator.h>
#include <Core/Parallel/CommunicationList.hpp>
#include <Core/Parallel/CrowdMonitor.hpp>
#include <Core/Parallel/ProcessorGroup.h>
#include <Core/Parallel/UintahMPI.h>
#include <Core/Util/Time.h>
#include <Core/Util/DebugStream.h>
#include <Core/Util/FancyAssert.h>

#ifdef UINTAH_ENABLE_KOKKOS
#  include <Kokkos_Core.hpp>
#endif //UINTAH_ENABLE_KOKKOS

#include <cstring>
#include <iomanip>
#include <map>
#include <mutex>
#include <sstream>

// Pack data into a buffer before sending -- testing to see if this
// works better and avoids certain problems possible when you allow
// tasks to modify data that may have a pending send.
#define USE_PACKING

using namespace Uintah;


namespace {

struct recv_tag{};
using  recv_monitor = Uintah::CrowdMonitor<recv_tag>;

std::mutex g_lb_mutex;                // load balancer lock
std::mutex g_wait_times_mutex;         // MPI wait times lock

}


// Used to sync cout/cerr so it is readable when output by multiple threads
extern std::mutex coutLock;
extern std::mutex cerrLock;

static DebugStream dbg(          "MPIScheduler_DBG",        false );
static DebugStream dbgst(        "SendTiming",              false );
static DebugStream timeout(      "MPIScheduler_TimingsOut", false );
static DebugStream reductionout( "ReductionTasks",          false );

DebugStream taskorder(     "TaskOrder", false );
DebugStream waitout(       "WaitTimes", false );
DebugStream execout(       "ExecTimes", false );
DebugStream taskdbg(       "TaskDBG",   false );
DebugStream taskLevel_dbg( "TaskLevel", false );
DebugStream mpidbg(        "MPIDBG",    false );

static double CurrentWaitTime = 0;

std::map<std::string, double> waittimes;
std::map<std::string, double> exectimes;

//______________________________________________________________________
//
MPIScheduler::MPIScheduler( const ProcessorGroup * myworld
                          , const Output         * oport
                          ,       MPIScheduler   * parentScheduler
                          )
  : SchedulerCommon(myworld, oport)
  , m_parent_scheduler{parentScheduler}
  , m_oport{oport}
{

#ifdef UINTAH_ENABLE_KOKKOS
  Kokkos::initialize();
#endif //UINTAH_ENABLE_KOKKOS

  m_last_time = Time::currentSeconds();

  reloc_new_posLabel_ = 0;

  if (timeout.active()) {
    char filename[64];
    sprintf(filename, "timingStats.%d", d_myworld->myrank());
    m_timings_stats.open(filename);
    if (d_myworld->myrank() == 0) {
      sprintf(filename, "timingStats.avg");
      m_avg_stats.open(filename);
      sprintf(filename, "timingStats.max");
      m_max_stats.open(filename);
    }
  }

  std::string timeStr("seconds");

  mpi_info_.insert( TotalReduce,    std::string("TotalReduce"),    timeStr, 0 );
  mpi_info_.insert( TotalSend,      std::string("TotalSend"),      timeStr, 0 );
  mpi_info_.insert( TotalRecv,      std::string("TotalRecv"),      timeStr, 0 );
  mpi_info_.insert( TotalTask,      std::string("TotalTask"),      timeStr, 0 );
  mpi_info_.insert( TotalReduceMPI, std::string("TotalReduceMPI"), timeStr, 0 );
  mpi_info_.insert( TotalSendMPI,   std::string("TotalSendMPI"),   timeStr, 0 );
  mpi_info_.insert( TotalRecvMPI,   std::string("TotalRecvMPI"),   timeStr, 0 );
  mpi_info_.insert( TotalTestMPI,   std::string("TotalTestMPI"),   timeStr, 0 );
  mpi_info_.insert( TotalWaitMPI,   std::string("TotalWaitMPI"),   timeStr, 0 );
  mpi_info_.validate( MAX_TIMING_STATS );
}

//______________________________________________________________________
//
void
MPIScheduler::problemSetup( const ProblemSpecP&     prob_spec,
                                  SimulationStateP& state )
{
  SchedulerCommon::problemSetup(prob_spec, state);
}

//______________________________________________________________________
//
MPIScheduler::~MPIScheduler()
{
  if (timeout.active()) {
    m_timings_stats.close();
    if (d_myworld->myrank() == 0) {
      m_avg_stats.close();
      m_max_stats.close();
    }
  }
#ifdef UINTAH_ENABLE_KOKKOS
  Kokkos::finalize();
#endif //UINTAH_ENABLE_KOKKOS
}

//______________________________________________________________________
//
SchedulerP
MPIScheduler::createSubScheduler()
{
  UintahParallelPort * lbp      = getPort("load balancer");
  MPIScheduler       * newsched = scinew MPIScheduler( d_myworld, m_outPort_, this );
  newsched->attachPort( "load balancer", lbp );
  newsched->d_sharedState = d_sharedState;
  return newsched;
}

//______________________________________________________________________
//
void
MPIScheduler::verifyChecksum()
{
#if SCI_ASSERTION_LEVEL >= 3
  if (Uintah::Parallel::usingMPI()) {

    // Compute a simple checksum to make sure that all processes are trying to
    // execute the same graph.  We should do two things in the future:
    //  - make a flag to turn this off
    //  - make the checksum more sophisticated
    int checksum = 0;
    int numSpatialTasks = 0;
    for (unsigned i = 0; i < graphs.size(); i++) {
      checksum += graphs[i]->getTasks().size();

      // This begins addressing the issue of making the global checksum more sophisticated:
      //   check if any tasks were spatially scheduled - TaskType::Spatial, meaning no computes, requires or modifies
      //     e.g. RMCRT radiometer task, which is not scheduled on all patches
      //          these Spatial tasks won't count toward the global checksum
      std::vector<Task*> tasks = graphs[i]->getTasks();
      std::vector<Task*>::const_iterator tasks_iter = tasks.begin();
      for (; tasks_iter != tasks.end(); ++tasks_iter) {
        Task* task = *tasks_iter;
        if (task->getType() == Task::Spatial) {
          numSpatialTasks++;
        }
      }
    }

    // Spatial tasks don't count against the global checksum
    checksum -= numSpatialTasks;

    if (mpidbg.active()) {
      coutLock.lock();
      mpidbg << d_myworld->myrank() << " (Uintah::MPI::Allreduce) Checking checksum of " << checksum << '\n';
      coutLock.unlock();
    }

    int result_checksum;
    Uintah::MPI::Allreduce(&checksum, &result_checksum, 1, MPI_INT, MPI_MIN, d_myworld->getComm());

    if (checksum != result_checksum) {
      std::cerr << "Failed task checksum comparison! Not all processes are executing the same taskgraph\n";
      std::cerr << "  Rank-" << d_myworld->myrank() << " of " << d_myworld->size() - 1 << ": has sum " << checksum
                << "  and global is " << result_checksum << '\n';
      Uintah::MPI::Abort(d_myworld->getComm(), 1);
    }

    if (mpidbg.active()) {
      coutLock.lock();
      mpidbg << d_myworld->myrank() << " (Uintah::MPI::Allreduce) Check succeeded\n";
      coutLock.unlock();
    }
  }
#endif
}

//______________________________________________________________________
//
void MPIScheduler::initiateTask( DetailedTask* dtask,
                                 bool          only_old_recvs,
                                 int           abort_point,
                                 int           iteration )
{
  postMPIRecvs(dtask, only_old_recvs, abort_point, iteration);

  // TODO - is this a mistake? Should be first check in this method APH 07/20/16
  if (only_old_recvs) {
    return;
  }
}

//______________________________________________________________________
//
void
MPIScheduler::initiateReduction( DetailedTask* dtask )
{
  if (reductionout.active() && d_myworld->myrank() == 0) {
    coutLock.lock();
    reductionout << "Running Reduction Task: " << dtask->getName() << std::endl;
    coutLock.unlock();
  }

  double reducestart = Time::currentSeconds();

  runReductionTask(dtask);

  double reduceend = Time::currentSeconds();

  emitNode(dtask, reducestart, reduceend - reducestart, 0);

  mpi_info_[TotalReduce   ] += reduceend - reducestart;
  mpi_info_[TotalReduceMPI] += reduceend - reducestart;
}

//______________________________________________________________________
//
void
MPIScheduler::runTask( DetailedTask* dtask,
                       int           iteration,
                       int           thread_id /*=0*/ )
{
  if (waitout.active()) {
    g_wait_times_mutex.lock();
    waittimes[dtask->getTask()->getName()] += CurrentWaitTime;
    CurrentWaitTime = 0;
    g_wait_times_mutex.unlock();
  }

  double taskstart = Time::currentSeconds();

  if (trackingVarsPrintLocation_ & SchedulerCommon::PRINT_BEFORE_EXEC) {
    printTrackedVars(dtask, SchedulerCommon::PRINT_BEFORE_EXEC);
  }
  std::vector<DataWarehouseP> plain_old_dws(dws.size());
  for (int i = 0; i < (int)dws.size(); i++) {
    plain_old_dws[i] = dws[i].get_rep();
  }

    dtask->doit(d_myworld, dws, plain_old_dws);

  if (trackingVarsPrintLocation_ & SchedulerCommon::PRINT_AFTER_EXEC) {
    printTrackedVars(dtask, SchedulerCommon::PRINT_AFTER_EXEC);
  }

  double total_task_time = Time::currentSeconds() - taskstart;

  g_lb_mutex.lock();
  {
    if (execout.active()) {
      exectimes[dtask->getTask()->getName()] += total_task_time;
    }

    // if I do not have a sub scheduler
    if (!dtask->getTask()->getHasSubScheduler()) {
      //add my task time to the total time
      mpi_info_[TotalTask] += total_task_time;
      if (!d_sharedState->isCopyDataTimestep() && dtask->getTask()->getType() != Task::Output) {
        //add contribution for patchlist
        getLoadBalancer()->addContribution(dtask, total_task_time);
      }
    }
  }
  g_lb_mutex.unlock();

  postMPISends(dtask, iteration, thread_id);

  dtask->done(dws);  // should this be part of task execution time? - APH 09/16/15

  double teststart = Time::currentSeconds();


  //---------------------------------------------------------------------------
  // New way of managing single MPI requests - avoids MPI_Waitsome & MPI_Donesome - APH 07/20/16
  //---------------------------------------------------------------------------
  auto ready_request = [](CommRequest const& r)->bool { return r.test(); };
  CommRequestPool::iterator comm_sends_iter = m_sends.find_any(ready_request);
  if (comm_sends_iter) {
    MPI_Status status;
    comm_sends_iter->finishedCommunication(d_myworld, status);
    m_sends.erase(comm_sends_iter);
  }
  //-----------------------------------


  mpi_info_[TotalTestMPI] += Time::currentSeconds() - teststart;

  // Add subscheduler timings to the parent scheduler and reset subscheduler timings
  if (m_parent_scheduler) {
    for (size_t i = 0; i < mpi_info_.size(); ++i) {
      MPIScheduler::TimingStat e = (MPIScheduler::TimingStat)i;
      m_parent_scheduler->mpi_info_[e] += mpi_info_[e];
    }
    mpi_info_.reset(0);
  }

  emitNode(dtask, taskstart, total_task_time, 0);

}  // end runTask()

//______________________________________________________________________
//
void
MPIScheduler::runReductionTask( DetailedTask* dtask )
{
  const Task::Dependency* mod = dtask->getTask()->getModifies();
  ASSERT(!mod->m_next);

  OnDemandDataWarehouse* dw = dws[mod->mapDataWarehouse()].get_rep();
  ASSERT(dtask->getTask()->m_comm>=0);
  dw->reduceMPI(mod->m_var, mod->m_reduction_level, mod->m_matls, dtask->getTask()->m_comm);
  dtask->done(dws);
}

//______________________________________________________________________
//
void
MPIScheduler::postMPISends( DetailedTask* dtask,
                            int           iteration,
                            int           thread_id  /*=0*/ )
{
  double sendstart = Time::currentSeconds();
  bool dbg_active = dbg.active();

  int me = d_myworld->myrank();
  if (dbg_active) {
    cerrLock.lock();
    dbg << "Rank-" << me << " postMPISends - task " << *dtask << '\n';
    cerrLock.unlock();
  }

  int numSend = 0;
  int volSend = 0;

  // Send data to dependents
  for (DependencyBatch* batch = dtask->getComputes(); batch != 0; batch = batch->comp_next) {

    // Prepare to send a message
#ifdef USE_PACKING
    PackBufferInfo mpibuff;
#else
    BufferInfo mpibuff;
#endif
    // Create the MPI type
    int to = batch->toTasks.front()->getAssignedResourceIndex();
    ASSERTRANGE(to, 0, d_myworld->size());

    for (DetailedDep* req = batch->head; req != 0; req = req->next) {


      if ((req->condition == DetailedDep::FirstIteration && iteration > 0) || (req->condition == DetailedDep::SubsequentIterations
          && iteration == 0) || (notCopyDataVars_.count(req->req->m_var->getName()) > 0)) {
        // See comment in DetailedDep about CommCondition
        if (dbg_active) {
          cerrLock.lock();
          dbg << "Rank-" << me << "   Ignoring conditional send for " << *req << "\n";
          cerrLock.unlock();
        }
        continue;
      }

      // if we send/recv to an output task, don't send/recv if not an output timestep
      if (req->toTasks.front()->getTask()->getType() == Task::Output && !m_oport->isOutputTimestep()
          && !m_oport->isCheckpointTimestep()) {
        if (dbg_active) {
          cerrLock.lock();
          dbg << "Rank-" << me << "   Ignoring non-output-timestep send for " << *req << "\n";
          cerrLock.unlock();
        }
        continue;
      }

      OnDemandDataWarehouse* dw = dws[req->req->mapDataWarehouse()].get_rep();
      if (dbg_active) {
        cerrLock.lock();
        {
          dbg << "Rank-" << me << " --> sending " << *req << ", ghost type: " << "\""
              << Ghost::getGhostTypeName(req->req->m_gtype) << "\", " << "num req ghost "
              << Ghost::getGhostTypeName(req->req->m_gtype) << ": " << req->req->m_num_ghost_cells
              << ", Ghost::direction: " << Ghost::getGhostTypeDir(req->req->m_gtype)
              << ", from dw " << dw->getID() << '\n';
        }
        cerrLock.unlock();
      }

      // the load balancer is used to determine where data was in the old dw on the prev timestep -
      // pass it in if the particle data is on the old dw
      const VarLabel* posLabel;
      OnDemandDataWarehouse* posDW;
      LoadBalancer* lb = 0;

      if( !reloc_new_posLabel_ && m_parent_scheduler ) {
        posDW = dws[req->req->m_task->mapDataWarehouse(Task::ParentOldDW)].get_rep();
        posLabel = m_parent_scheduler->reloc_new_posLabel_;
      }
      else {
        // on an output task (and only on one) we require particle variables from the NewDW
        if (req->toTasks.front()->getTask()->getType() == Task::Output) {
          posDW = dws[req->req->m_task->mapDataWarehouse(Task::NewDW)].get_rep();
        }
        else {
          posDW = dws[req->req->m_task->mapDataWarehouse(Task::OldDW)].get_rep();
          lb = getLoadBalancer();
        }
        posLabel = reloc_new_posLabel_;
      }

      MPIScheduler* top = this;
      while( top->m_parent_scheduler ) {
        top = top->m_parent_scheduler;
      }

      dw->sendMPI(batch, posLabel, mpibuff, posDW, req, lb);
    }

    // Post the send
    if (mpibuff.count() > 0) {
      ASSERT(batch->messageTag > 0);
      double start = Time::currentSeconds();
      void* buf;
      int count;
      MPI_Datatype datatype;

#ifdef USE_PACKING
      mpibuff.get_type(buf, count, datatype, d_myworld->getComm());
      mpibuff.pack(d_myworld->getComm(), count);
#else
      mpibuff.get_type(buf, count, datatype);
#endif

      if (mpidbg.active()) {
        cerrLock.lock();
        mpidbg << "Rank-" << me << " Posting send for message number " << batch->messageTag << " to   rank-" << to << ", length: " << count
               << " (bytes)\n";
        cerrLock.unlock();
      }

      m_num_messages++;
      numSend++;
      int typeSize;

      Uintah::MPI::Type_size(datatype, &typeSize);
      m_message_volume += count * typeSize;
      volSend += count * typeSize;


      //---------------------------------------------------------------------------
      // New way of managing single MPI requests - avoids MPI_Waitsome & MPI_Donesome - APH 07/20/16
      //---------------------------------------------------------------------------
      CommRequestPool::iterator comm_sends_iter = m_sends.emplace(new SendHandle(mpibuff.takeSendlist()));
      Uintah::MPI::Isend(buf, count, datatype, to, batch->messageTag, d_myworld->getComm(), comm_sends_iter->request());
      comm_sends_iter.clear();
      //---------------------------------------------------------------------------


      mpi_info_[TotalSendMPI] += Time::currentSeconds() - start;

    }
  }  // end for (DependencyBatch* batch = task->getComputes())

  double dsend = Time::currentSeconds() - sendstart;
  mpi_info_[TotalSend] += dsend;

  size_t sends_size = m_sends.size();
  if (dbgst.active() && sends_size > 0) {
    if (d_myworld->myrank() == d_myworld->size() / 2) {
      if (dbgst.active()) {
        cerrLock.lock();
        dbgst << " Rank-" << d_myworld->myrank()
              << " Time: " << Time::currentSeconds()
              << " , NumSend= " << sends_size
              << " , VolSend: " << volSend
              << std::endl;
        cerrLock.unlock();
      }
    }
  }
}  // end postMPISends();

//______________________________________________________________________
//
struct CompareDep {
    bool operator()( DependencyBatch* a,
                     DependencyBatch* b )
    {
      return a->messageTag < b->messageTag;
    }
};

//______________________________________________________________________
//
void MPIScheduler::postMPIRecvs( DetailedTask* dtask,
                                 bool          only_old_recvs,
                                 int           abort_point,
                                 int           iteration )
{
  double recvstart = Time::currentSeconds();
  bool dbg_active = dbg.active();

  if (dbg_active) {
    cerrLock.lock();
    dbg << "Rank-" << d_myworld->myrank() << " postMPIRecvs - task " << *dtask << '\n';
    cerrLock.unlock();
  }

  if (trackingVarsPrintLocation_ & SchedulerCommon::PRINT_BEFORE_COMM) {
    printTrackedVars(dtask, SchedulerCommon::PRINT_BEFORE_COMM);
  }

  // sort the requires, so in case there is a particle send we receive it with
  // the right message tag

  std::vector<DependencyBatch*> sorted_reqs;
  std::map<DependencyBatch*, DependencyBatch*>::const_iterator iter = dtask->getRequires().begin();

    for (; iter != dtask->getRequires().end(); iter++) {
      sorted_reqs.push_back(iter->first);
    }

    CompareDep comparator;
    std::sort(sorted_reqs.begin(), sorted_reqs.end(), comparator);
    std::vector<DependencyBatch*>::iterator sorted_iter = sorted_reqs.begin();

  // Receive any of the foreign requires
  {
    recv_monitor recv_lock{ Uintah::CrowdMonitor<recv_tag>::WRITER };

    for (; sorted_iter != sorted_reqs.end(); sorted_iter++) {
      DependencyBatch* batch = *sorted_iter;

      // The first thread that calls this on the batch will return true
      // while subsequent threads calling this will block and wait for
      // that first thread to receive the data.

      dtask->incrementExternalDepCount();
      if (!batch->makeMPIRequest()) {
        if (dbg_active) {
          cerrLock.lock();
          dbg << "Someone else already receiving it\n";
          cerrLock.unlock();
        }
        continue;
      }

      if (only_old_recvs) {
        if (dbg_active) {
          dbg << "abort analysis: " << batch->fromTask->getTask()->getName() << ", so="
              << batch->fromTask->getTask()->getSortedOrder() << ", abort_point=" << abort_point << '\n';
          if (batch->fromTask->getTask()->getSortedOrder() <= abort_point)
            dbg << "posting MPI recv for pre-abort message " << batch->messageTag << '\n';
        }
        if (!(batch->fromTask->getTask()->getSortedOrder() <= abort_point)) {
          continue;
        }
      }

      // Prepare to receive a message
      BatchReceiveHandler* pBatchRecvHandler = scinew BatchReceiveHandler(batch);
      PackBufferInfo* p_mpibuff = 0;

#ifdef USE_PACKING
      p_mpibuff = scinew PackBufferInfo();
      PackBufferInfo& mpibuff = *p_mpibuff;
#else
      BufferInfo mpibuff;
#endif

      // Create the MPI type
      for (DetailedDep* req = batch->head; req != 0; req = req->next) {

        OnDemandDataWarehouse* dw = dws[req->req->mapDataWarehouse()].get_rep();
        if ((req->condition == DetailedDep::FirstIteration && iteration > 0) || (req->condition == DetailedDep::SubsequentIterations
            && iteration == 0) || (notCopyDataVars_.count(req->req->m_var->getName()) > 0)) {

          // See comment in DetailedDep about CommCondition
          if (dbg_active) {
            cerrLock.lock();
            dbg << "Rank-" << d_myworld->myrank() << "   Ignoring conditional receive for " << *req << std::endl;
          }
          continue;
        }
        // if we send/recv to an output task, don't send/recv if not an output timestep
        if (req->toTasks.front()->getTask()->getType() == Task::Output && !m_oport->isOutputTimestep()
            && !m_oport->isCheckpointTimestep()) {
          cerrLock.lock();
          dbg << "Rank-" << d_myworld->myrank() << "   Ignoring non-output-timestep receive for " << *req << std::endl;
          cerrLock.unlock();
          continue;
        }
        if (dbg_active) {
          cerrLock.lock();
          {
            dbg << "Rank-" << d_myworld->myrank() << " <-- receiving " << *req << ", ghost type: " << "\""
                << Ghost::getGhostTypeName(req->req->m_gtype) << "\", " << "num req ghost "
                << Ghost::getGhostTypeName(req->req->m_gtype) << ": " << req->req->m_num_ghost_cells
                << ", Ghost::direction: " << Ghost::getGhostTypeDir(req->req->m_gtype)
                << ", into dw " << dw->getID() << '\n';
          }
          cerrLock.unlock();
        }

        OnDemandDataWarehouse* posDW;

        // the load balancer is used to determine where data was in the old dw on the prev timestep
        // pass it in if the particle data is on the old dw
        LoadBalancer* lb = 0;
        if (!reloc_new_posLabel_ && m_parent_scheduler) {
          posDW = dws[req->req->m_task->mapDataWarehouse(Task::ParentOldDW)].get_rep();
        }
        else {
          // on an output task (and only on one) we require particle variables from the NewDW
          if (req->toTasks.front()->getTask()->getType() == Task::Output) {
            posDW = dws[req->req->m_task->mapDataWarehouse(Task::NewDW)].get_rep();
          }
          else {
            posDW = dws[req->req->m_task->mapDataWarehouse(Task::OldDW)].get_rep();
            lb = getLoadBalancer();
          }
        }

        MPIScheduler* top = this;
        while (top->m_parent_scheduler) {
          top = top->m_parent_scheduler;
        }

        dw->recvMPI(batch, mpibuff, posDW, req, lb);

        if (!req->isNonDataDependency()) {
          graphs[currentTG_]->getDetailedTasks()->setScrubCount(req->req, req->matl, req->fromPatch, dws);
        }
      }

      // Post the receive
      if (mpibuff.count() > 0) {

        ASSERT(batch->messageTag > 0);
        double start = Time::currentSeconds();
        void* buf;
        int count;
        MPI_Datatype datatype;

#ifdef USE_PACKING
        mpibuff.get_type(buf, count, datatype, d_myworld->getComm());
#else
        mpibuff.get_type(buf, count, datatype);
#endif

        int from = batch->fromTask->getAssignedResourceIndex();
        ASSERTRANGE(from, 0, d_myworld->size());

        if (mpidbg.active()) {
        cerrLock.lock();
        mpidbg << "Rank-" << d_myworld->myrank() << " Posting recv for message number " << batch->messageTag << " from rank-" << from
               << ", length: " << count << " (bytes)\n";
        cerrLock.unlock();
        }


        //---------------------------------------------------------------------------
        // New way of managing single MPI requests - avoids MPI_Waitsome & MPI_Donesome - APH 07/20/16
        //---------------------------------------------------------------------------
        CommRequestPool::iterator comm_recvs_iter = m_recvs.emplace(new RecvHandle(p_mpibuff, pBatchRecvHandler));
        Uintah::MPI::Irecv(buf, count, datatype, from, batch->messageTag, d_myworld->getComm(), comm_recvs_iter->request());
        comm_recvs_iter.clear();
        //---------------------------------------------------------------------------


        mpi_info_[TotalRecvMPI] += Time::currentSeconds() - start;
      }
      else {
        // Nothing really need to be received, but let everyone else know
        // that it has what is needed (nothing).
        batch->received(d_myworld);
#ifdef USE_PACKING
        // otherwise, these will be deleted after it receives and unpacks the data.
        delete p_mpibuff;
        delete pBatchRecvHandler;
#endif
      }
    }  // end for loop over requires
  }  // end recv_lock{ Uintah::CrowdMonitor<recv_tag>::WRITER }

  double drecv = Time::currentSeconds() - recvstart;
  mpi_info_[TotalRecv] += drecv;

}  // end postMPIRecvs()

//______________________________________________________________________
//
void MPIScheduler::processMPIRecvs(int how_much)
{
  if (m_recvs.size() == 0) {
    return;
  }

  double start = Time::currentSeconds();

  //---------------------------------------------------------------------------
  // New way of managing single MPI requests - avoids MPI_Waitsome & MPI_Donesome - APH 07/20/16
  //---------------------------------------------------------------------------
  auto test_request    = [](CommRequest const& n)->bool { return n.test(); };
  auto wait_request    = [](CommRequest const& n)->bool { return n.wait(); };

  CommRequestPool::iterator comm_iter;

  {
    recv_monitor recv_lock{ Uintah::CrowdMonitor<recv_tag>::WRITER };

    switch (how_much) {

      case TEST :
        comm_iter = m_recvs.find_any(test_request);
        if (comm_iter) {
          MPI_Status status;
          comm_iter->finishedCommunication(d_myworld, status);
          m_recvs.erase(comm_iter);
        }
        break;

      case WAIT_ONCE :
        comm_iter = m_recvs.find_any(wait_request);
        if (comm_iter) {
          MPI_Status status;
          comm_iter->finishedCommunication(d_myworld, status);
          m_recvs.erase(comm_iter);
        }
        break;

      case WAIT_ALL :
        while (m_recvs.size() != 0u) {
          comm_iter = m_recvs.find_any(wait_request);
          if (comm_iter) {
            MPI_Status status;
            comm_iter->finishedCommunication(d_myworld, status);
            m_recvs.erase(comm_iter);
          }
        }
        break;

    }  // end switch

  }

  mpi_info_[TotalWaitMPI] += Time::currentSeconds() - start;
  CurrentWaitTime += Time::currentSeconds() - start;

}  // end processMPIRecvs()

//______________________________________________________________________
//

void
MPIScheduler::execute( int tgnum     /* = 0 */,
                       int iteration /* = 0 */ )
{
  ASSERTRANGE(tgnum, 0, (int )graphs.size());
  TaskGraph* tg = graphs[tgnum];
  tg->setIteration(iteration);
  currentTG_ = tgnum;

  if (graphs.size() > 1) {
    // tg model is the multi TG model, where each graph is going to need to
    // have its dwmap reset here (even with the same tgnum)
    tg->remapTaskDWs(dwmap);
  }

  DetailedTasks* dts = tg->getDetailedTasks();

  if (dts == 0) {
    if (d_myworld->myrank() == 0) {
      cerrLock.lock();
      std::cerr << "MPIScheduler skipping execute, no tasks\n";
      cerrLock.unlock();
    }
    return;
  }

  int ntasks = dts->numLocalTasks();
  dts->initializeScrubs(dws, dwmap);
  dts->initTimestep();

  for (int i = 0; i < ntasks; i++) {
    dts->localTask(i)->resetDependencyCounts();
  }

  if (timeout.active()) {
    m_labels.clear();
    m_times.clear();
    //emitTime("time since last execute");
  }

  int me = d_myworld->myrank();

  // TODO determine exactly what this does and at what cost/benefit (APH 01/22/15)
  makeTaskGraphDoc(dts, me);

  //if(timeout.active())
  //emitTime("taskGraph output");

  mpi_info_.reset( 0 );

  int numTasksDone = 0;

  if (dbg.active()) {
    cerrLock.lock();
    dbg << me << " Executing " << dts->numTasks() << " tasks (" << ntasks << " local)\n";
    cerrLock.unlock();
  }

  bool abort = false;
  int abort_point = 987654;

  if (reloc_new_posLabel_ && dws[dwmap[Task::OldDW]] != 0) {
    dws[dwmap[Task::OldDW]]->exchangeParticleQuantities(dts, getLoadBalancer(), reloc_new_posLabel_, iteration);
  }

  int i = 0;
  while (numTasksDone < ntasks) {
    i++;

    //
    // The following checkMemoryUse() is commented out to allow for
    // maintaining the same functionality as before this commit...
    // In other words, so that memory highwater checking is only done
    // at the end of a timestep, and not between tasks... Once the
    // RT settles down we will uncomment this section and then
    // memory use checks will occur before every task.
    //
    // Note, the results (memuse, highwater, maxMemUse) from the following
    // checkMemoryUse call are not used... the call, however, records
    // the maxMemUse for future reference, and that is why we are calling
    // it.
    //
    //unsigned long memuse, highwater, maxMemUse;
    //checkMemoryUse( memuse, highwater, maxMemUse );

    DetailedTask * task = dts->getNextInternalReadyTask();

    numTasksDone++;

    if (taskorder.active()) {
      taskorder << d_myworld->myrank() << " Running task static order: " << task->getStaticOrder() << " , scheduled order: "
                << numTasksDone << std::endl;
    }

    if (taskdbg.active()) {
      taskdbg << me << " Initiating task:  \t";
      printTask(taskdbg, task);
      taskdbg << '\n';
    }

    if (task->getTask()->getType() == Task::Reduction) {
      if (!abort) {
        initiateReduction(task);
      }
    }
    else {
      initiateTask(task, abort, abort_point, iteration);
      processMPIRecvs(WAIT_ALL);
      ASSERT(m_recvs.size() == 0);
      runTask(task, iteration);

      if (taskdbg.active()) {
        taskdbg << d_myworld->myrank() << " Completed task:  \t";
        printTask(taskdbg, task);
        taskdbg << '\n';
        printTaskLevels(d_myworld, taskLevel_dbg, task);
      }
    }

    if(!abort && dws[dws.size()-1] && dws[dws.size()-1]->timestepAborted()){
      abort = true;
      abort_point = task->getTask()->getSortedOrder();
      dbg << "Aborting timestep after task: " << *task->getTask() << '\n';
    }
  } // end while( numTasksDone < ntasks )

  if (timeout.active()) {
    emitTime("MPI send time", mpi_info_[TotalSendMPI]);
    emitTime("MPI Testsome time", mpi_info_[TotalTestMPI]);
    emitTime("Total send time", mpi_info_[TotalSend] - mpi_info_[TotalSendMPI] - mpi_info_[TotalTestMPI]);
    emitTime("MPI recv time", mpi_info_[TotalRecvMPI]);
    emitTime("MPI wait time", mpi_info_[TotalWaitMPI]);
    emitTime("Total recv time", mpi_info_[TotalRecv] - mpi_info_[TotalRecvMPI] - mpi_info_[TotalWaitMPI]);
    emitTime("Total task time", mpi_info_[TotalTask]);
    emitTime("Total MPI reduce time", mpi_info_[TotalReduceMPI]);
    emitTime("Total reduction time", mpi_info_[TotalReduce] - mpi_info_[TotalReduceMPI]);
    emitTime("Total comm time", mpi_info_[TotalRecv] + mpi_info_[TotalSend] + mpi_info_[TotalReduce]);

    double time = Time::currentSeconds();
    double totalexec = time - m_last_time;

    m_last_time = time;

    emitTime("Other execution time", totalexec - mpi_info_[TotalSend] - mpi_info_[TotalRecv] - mpi_info_[TotalTask] - mpi_info_[TotalReduce]);
  }

  if( !m_parent_scheduler ) { // If this scheduler is the root scheduler...    
    computeNetRunTimeStats(d_sharedState->d_runTimeStats);
  }


  //---------------------------------------------------------------------------
  // New way of managing single MPI requests - avoids MPI_Waitsome & MPI_Donesome - APH 07/20/16
  //---------------------------------------------------------------------------
  // wait on all pending requests
  auto ready_request = [](CommRequest const& r)->bool { return r.wait(); };
  CommRequestPool::handle find_handle;
  while ( m_sends.size() != 0u ) {
    CommRequestPool::iterator comm_sends_iter;
    if ( (comm_sends_iter = m_sends.find_any(find_handle, ready_request)) ) {
      find_handle = comm_sends_iter;
      m_sends.erase(comm_sends_iter);
    } else {
      // TODO - make this a sleep? APH 07/20/16
    }
  }
  //---------------------------------------------------------------------------

  ASSERT(m_sends.size() == 0);
  ASSERT(m_recvs.size() == 0);


//  if(timeout.active()) {
//    emitTime("final wait");
//  }

  if (restartable && tgnum == (int)graphs.size() - 1) {
    // Copy the restart flag to all processors
    int myrestart = dws[dws.size() - 1]->timestepRestarted();
    int netrestart;
    Uintah::MPI::Allreduce(&myrestart, &netrestart, 1, MPI_INT, MPI_LOR, d_myworld->getComm());
    if (netrestart) {
      dws[dws.size() - 1]->restartTimestep();
      if (dws[0]) {
        dws[0]->setRestarted();
      }
    }
  }

  finalizeTimestep();

  if ( !m_parent_scheduler && (execout.active() || timeout.active() || waitout.active()) ) {  // only do on toplevel scheduler
    outputTimingStats("MPIScheduler");
  }

  if (dbg.active()) {
    coutLock.lock();
    dbg << me << " MPIScheduler finished\n";
    coutLock.unlock();
  }
}

//______________________________________________________________________
//
void
MPIScheduler::emitTime( const char* label )
{
   double time = Time::currentSeconds();
   emitTime(label, time-m_last_time);
   m_last_time = time;
}

//______________________________________________________________________
//
void
MPIScheduler::emitTime( const char*  label,
                              double dt )
{
   m_labels.push_back(label);
   m_times.push_back(dt);
}

//______________________________________________________________________
//
void
MPIScheduler::outputTimingStats(const char* label)
{
  // add number of cells, patches, and particles
  int numCells = 0, numParticles = 0;
  OnDemandDataWarehouseP dw = dws[dws.size() - 1];
  const GridP grid(const_cast<Grid*>(dw->getGrid()));
  const PatchSubset* myPatches = getLoadBalancer()->getPerProcessorPatchSet(grid)->getSubset(d_myworld->myrank());
  for (int p = 0; p < myPatches->size(); p++) {
    const Patch* patch = myPatches->get(p);
    IntVector range = patch->getExtraCellHighIndex() - patch->getExtraCellLowIndex();
    numCells += range.x() * range.y() * range.z();

    // go through all materials since getting an MPMMaterial correctly would depend on MPM
    for (int m = 0; m < d_sharedState->getNumMatls(); m++) {
      if (dw->haveParticleSubset(m, patch))
        numParticles += dw->getParticleSubset(m, patch)->numParticles();
    }
  }

  emitTime("NumPatches", myPatches->size());
  emitTime("NumCells", numCells);
  emitTime("NumParticles", numParticles);
  std::vector<double> d_totaltimes(m_times.size());
  std::vector<double> d_maxtimes(m_times.size());
  std::vector<double> d_avgtimes(m_times.size());
  double avgTask = -1, maxTask = -1;
  double avgComm = -1, maxComm = -1;
  double avgCell = -1, maxCell = -1;

  MPI_Comm comm = d_myworld->getComm();
  Uintah::MPI::Reduce(&m_times[0], &d_totaltimes[0], static_cast<int>(m_times.size()), MPI_DOUBLE, MPI_SUM, 0, comm);
  Uintah::MPI::Reduce(&m_times[0], &d_maxtimes[0],   static_cast<int>(m_times.size()), MPI_DOUBLE, MPI_MAX, 0, comm);

  double total = 0, avgTotal = 0, maxTotal = 0;
  for (int i = 0; i < (int)d_totaltimes.size(); i++) {
    d_avgtimes[i] = d_totaltimes[i] / d_myworld->size();
    if (strcmp(m_labels[i], "Total task time") == 0) {
      avgTask = d_avgtimes[i];
      maxTask = d_maxtimes[i];
    }
    else if (strcmp(m_labels[i], "Total comm time") == 0) {
      avgComm = d_avgtimes[i];
      maxComm = d_maxtimes[i];
    }
    else if (strncmp(m_labels[i], "Num", 3) == 0) {
      if (strcmp(m_labels[i], "NumCells") == 0) {
        avgCell = d_avgtimes[i];
        maxCell = d_maxtimes[i];
      }
      // these are independent stats - not to be summed
      continue;
    }

    total    += m_times[i];
    avgTotal += d_avgtimes[i];
    maxTotal += d_maxtimes[i];
  }

  // to not duplicate the code
  std::vector<std::ofstream*> files;
  std::vector<std::vector<double>*> data;
  files.push_back(&m_timings_stats);
  data.push_back(&m_times);

  int me = d_myworld->myrank();

  if (me == 0) {
    files.push_back(&m_avg_stats);
    files.push_back(&m_max_stats);
    data.push_back(&d_avgtimes);
    data.push_back(&d_maxtimes);
  }

  for (unsigned file = 0; file < files.size(); file++) {
    std::ofstream& out = *files[file];
    out << "Timestep " << d_sharedState->getCurrentTopLevelTimeStep() << std::endl;
    for (int i = 0; i < (int)(*data[file]).size(); i++) {
      out << label << ": " << m_labels[i] << ": ";
      int len = static_cast<int>(strlen(m_labels[i]) + strlen("MPIScheduler: ") + strlen(": "));
      for (int j = len; j < 55; j++)
        out << ' ';
      double percent;
      if (strncmp(m_labels[i], "Num", 3) == 0) {
        percent = d_totaltimes[i] == 0 ? 100 : (*data[file])[i] / d_totaltimes[i] * 100;
      }
      else {
        percent = (*data[file])[i] / total * 100;
      }
      out << (*data[file])[i] << " (" << percent << "%)\n";
    }
    out << std::endl << std::endl;
  }

  if (me == 0) {
    timeout << "  Avg. exec: " << avgTask << ", max exec: " << maxTask << " = " << (1 - avgTask / maxTask) * 100 << " load imbalance (exec)%\n";
    timeout << "  Avg. comm: " << avgComm << ", max comm: " << maxComm << " = " << (1 - avgComm / maxComm) * 100 << " load imbalance (comm)%\n";
    timeout << "  Avg.  vol: " << avgCell << ", max  vol: " << maxCell << " = " << (1 - avgCell / maxCell) * 100 << " load imbalance (theoretical)%\n";
  }

  double time = Time::currentSeconds();
  m_last_time = time;

  if (execout.active()) {
    static int count = 0;

    // only output the exec times every 10 timesteps
    if (++count % 10 == 0) {
      std::ofstream fout;
      char filename[100];
      sprintf(filename, "exectimes.%d.%d", d_myworld->size(), d_myworld->myrank());
      fout.open(filename);

      // Report which timesteps TaskExecTime values have been accumulated over
      fout << "Reported values are cumulative over 10 timesteps ("
           << d_sharedState->getCurrentTopLevelTimeStep()-9
           << " through "
           << d_sharedState->getCurrentTopLevelTimeStep()
           << ")" << std::endl;

      for (std::map<std::string, double>::iterator iter = exectimes.begin(); iter != exectimes.end(); iter++) {
        fout << std::fixed << d_myworld->myrank() << ": TaskExecTime(s): " << iter->second << " Task:" << iter->first << std::endl;
      }
      fout.close();
      exectimes.clear();
    }
  }

  if (waitout.active()) {
    static int count = 0;

    // only output the exec times every 10 timesteps
    if (++count % 10 == 0) {

      if (d_myworld->myrank() == 0 || d_myworld->myrank() == d_myworld->size() / 2 || d_myworld->myrank() == d_myworld->size() - 1) {

        std::ofstream wout;
        char fname[100];
        sprintf(fname, "waittimes.%d.%d", d_myworld->size(), d_myworld->myrank());
        wout.open(fname);

        for (std::map<std::string, double>::iterator iter = waittimes.begin(); iter != waittimes.end(); iter++) {
          wout << std::fixed << d_myworld->myrank() << ":   TaskWaitTime(TO): " << iter->second << " Task:" << iter->first << std::endl;
        }

        for (std::map<std::string, double>::iterator iter = DependencyBatch::waittimes.begin(); iter != DependencyBatch::waittimes.end();
            iter++) {
          wout << std::fixed << d_myworld->myrank() << ": TaskWaitTime(FROM): " << iter->second << " Task:" << iter->first << std::endl;
        }
        wout.close();
      }

      waittimes.clear();
      DependencyBatch::waittimes.clear();
    }
  }
}

//______________________________________________________________________
//  Take the various timers and compute the net results
void MPIScheduler::computeNetRunTimeStats(InfoMapper< SimulationState::RunTimeStat, double >& runTimeStats)
{
    runTimeStats[SimulationState::TaskExecTime]       += mpi_info_[TotalTask] - runTimeStats[SimulationState::OutputFileIOTime]  // don't count output time or bytes
                                                                              - runTimeStats[SimulationState::OutputFileIORate];
     
    runTimeStats[SimulationState::TaskLocalCommTime]  += mpi_info_[TotalRecv] + mpi_info_[TotalSend];
    runTimeStats[SimulationState::TaskWaitCommTime]   += mpi_info_[TotalWaitMPI];
    runTimeStats[SimulationState::TaskGlobalCommTime] += mpi_info_[TotalReduce];
}
