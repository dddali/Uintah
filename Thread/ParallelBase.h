/*
 * This file was automatically generated by SCC - do NOT edit!
 * You should edit ParallelBase.scc instead 
 */

#ifndef SCI_THREAD_PARALLELBASE_H
#define SCI_THREAD_PARALLELBASE_H 1

/*
 * Helper class for Parallel class.  This will never be used
 * by a user program.  See <b>Parallel</b> instead.
 */


class ParallelBase {
protected:
    ParallelBase() ;
    virtual ~ParallelBase() ;
    friend class Thread;
    virtual void run(int proc)=0;
public:
};

#endif
