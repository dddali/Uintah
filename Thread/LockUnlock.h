/*
 * This file was automatically generated by SCC - do NOT edit!
 * You should edit LockUnlock.scc instead 
 */

#ifndef SCI_THREAD_LOCKUNLOCK_H
#define SCI_THREAD_LOCKUNLOCK_H 1

/*
 * Utility class to lock and unlock a <b>Mutex</b> or a <b>CrowdMonitor</b>.
 * The constructor of the <b>LockUnlock</b> object will lock the mutex
 * (or <b>CrowdMonitor</b>), and the destructor will unlock it.
 * <p>
 * This would be used like this:
 * <blockquote><pre>
 * {
 * <blockquote>LockUnlock mlock(&mutex);  // Acquire the mutex
 *     ... critical section ...</blockquote>
 * } // mutex is released when mlock goes out of scope
 * </pre></blockquote>
 */
class CrowdMonitor;
class Mutex;


class LockUnlock {
    Mutex* mutex;
    CrowdMonitor* monitor;
public:
    LockUnlock(Mutex* mutex);
    enum Which {
	Read,
	Write
    };
    LockUnlock(CrowdMonitor* crowdMonitor, Which action) ;
    ~LockUnlock() ;
private:
    Which action;
};

#endif
