#ifndef SCI_THREAD_THREADTOPOLOGY_H
#define SCI_THREAD_THREADTOPOLOGY_H 1

/**************************************
 
CLASS
   ThreadTopology
   
KEYWORDS
   ThreadTopology
   
DESCRIPTION
   
   Simple interface for defining a complex topology for inter-thread
   communication.  For a set of <tt>nthreads</tt> threads, the
   topology is constructed by connecting one thread to another thread
   with an optional <tt>weight</tt>.  Weights have arbitrary value,
   and their relative magnitude expresses the importantance of keeping
   two threads near each other when mapping the threads to physical
   processors.
PATTERNS


WARNING
   
****************************************/

class ThreadTopology {
public:
    //////////
    // Make a thread topology for some number of threads
    ThreadTopology(int nthreads);

    //////////
    // Destroy the <b>ThreadTopology</b>
    ~ThreadTopology();

    //////////
    // Define a connection between two threads.  <i>from</i> to <i>to</i>
    // range from 0 to <i>nthreads-1</i>.  An optional <i>weight</i>
    // indicates the relative importance of keeping these threads near
    // each other on the physical machine.  A <i>weight</i> of zero
    // disconnects the threads.  <i>connect(from, to)</i> is equivalent
    // to <i>connect(to, from)</i>, and calling connect for the same
    // <i>(from, to)</i> pair will override any previous weight for
    // the same connection.
    void connect(int from, int to, unsigned int weight=10);
};

#endif



