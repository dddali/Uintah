
// $Id$

/*
 *  ConditionVariable.h: Condition variable primitive
 *
 *  Written by:
 *   Author: Steve Parker
 *   Department of Computer Science
 *   University of Utah
 *   Date: June 1997
 *
 *  Copyright (C) 1997 SCI Group
 */

#ifndef SCICore_Thread_ConditionVariable_h
#define SCICore_Thread_ConditionVariable_h


/**************************************
 
CLASS
   ConditionVariable
   
KEYWORDS
   ConditionVariable
   
DESCRIPTION
   Condition variable primitive.  When a thread calls the
   <i>wait</i> method,which will block until another thread calls
   the <i>conditionSignal</i> or <i>conditionBroadcast</i> methods.  When
   there are multiple threads waiting, <i>conditionBroadcast</i> will unblock
   all of them, while <i>conditionSignal</i> will unblock only one (an
   arbitrary one) of them.  This primitive is used to allow a thread
   to wait for some condition to exist, such as an available resource.
   The thread waits for that condition, until it is unblocked by another
   thread that caused the condition to exist (<i>i.e.</i> freed the
   resource).
PATTERNS


WARNING
   
****************************************/

#include <SCICore/Thread/Semaphore.h>
#include <SCICore/Thread/Mutex.h>

namespace SCICore {
    namespace Thread {
	class ConditionVariable {
	    const char* d_name;
	    int d_num_waiters;
	    Mutex d_mutex;
	    Semaphore d_semaphore;
	public:
	    //////////
	    // Create a condition variable. <i>name</i> should be a static
	    // string which describes the primitive for debugging purposes.
	    ConditionVariable(const char* name);
    
	    //////////
	    // Destroy the condition variable
	    ~ConditionVariable();
    
	    //////////
	    // Wait for a condition.  This method atomically unlocks
	    // <b>mutex</b>, and blocks.  The <b>mutex</b> is typically
	    // used to guard access to the resource that the thread is
	    // waiting for.
	    void wait(Mutex& m);
    
	    //////////
	    // Signal a condition.  This will unblock one of the waiting
	    // threads. No guarantee is made as to which thread will be
	    // unblocked, but thread implementations typically give
	    // preference to the thread that has waited the longest.
	    void conditionSignal();

	    //////////
	    // Signal a condition.  This will unblock all of the waiting
	    // threads. Note that only the number of waiting threads will
	    // be unblocked. No guarantee is made that these are the same
	    // N threads that were blocked at the time of the broadcast.
	    void conditionBroadcast();
	};
    }
}

#endif

//
// $Log$
// Revision 1.4  1999/08/25 02:37:55  sparker
// Added namespaces
// General cleanups to prepare for integration with SCIRun
//
//
