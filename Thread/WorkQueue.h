/*
 * This file was automatically generated by SCC - do NOT edit!
 * You should edit WorkQueue.scc instead 
 */

#ifndef SCI_THREAD_WORKQUEUE_H
#define SCI_THREAD_WORKQUEUE_H 1

/*
 * Doles out work assignment to various worker threads.  Simple
 * attempts are made at evenly distributing the workload.
 * Initially, assignments are relatively large, and will get smaller
 * towards the end in an effort to equalize the total effort.
 */

#include "Mutex.h"
#include "ConditionVariable.h"

class WorkQueue {
    Mutex lock;
    ConditionVariable workdone;
    int current_assignment;
    int current_assignmentsize;
    int decrement;
    int threadcount;
    int nthreads;
    int totalAssignments;
    int granularity;
    int nwaiting;
    bool done;
    bool dynamic;
    const char* name;
    void init() ;
public:
    WorkQueue(const char* name, int totalAssignments, int nthreads,
	      bool dynamic, int granularity=5)
     ;
    WorkQueue(const WorkQueue& copy) ;
    WorkQueue() ;
    WorkQueue& operator=(const WorkQueue& copy) ;
    ~WorkQueue() ;
    bool nextAssignment(int& start, int& end) ;
    void addWork(int nassignments) ;
};

#endif
