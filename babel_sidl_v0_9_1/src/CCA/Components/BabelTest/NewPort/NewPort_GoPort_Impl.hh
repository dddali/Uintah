// 
// File:          NewPort_GoPort_Impl.hh
// Symbol:        NewPort.GoPort-v1.0
// Symbol Type:   class
// Babel Version: 0.7.4
// SIDL Created:  20040301 18:37:59 MST
// Generated:     20040301 18:38:04 MST
// Description:   Server-side implementation for NewPort.GoPort
// 
// WARNING: Automatically generated; only changes within splicers preserved
// 
// babel-version = 0.7.4
// source-line   = 6
// source-url    = file:/home/sci/damevski/SCIRun/src/CCA/Components/BabelTest/NewPort/NewPort.sidl
// 

#ifndef included_NewPort_GoPort_Impl_hh
#define included_NewPort_GoPort_Impl_hh

#ifndef included_SIDL_cxx_hh
#include "SIDL_cxx.hh"
#endif
#ifndef included_NewPort_GoPort_IOR_h
#include "NewPort_GoPort_IOR.h"
#endif
// 
// Includes for all method dependencies.
// 
#ifndef included_NewPort_GoPort_hh
#include "NewPort_GoPort.hh"
#endif
#ifndef included_SIDL_BaseInterface_hh
#include "SIDL_BaseInterface.hh"
#endif
#ifndef included_gov_cca_Services_hh
#include "gov_cca_Services.hh"
#endif


// DO-NOT-DELETE splicer.begin(NewPort.GoPort._includes)
// Put additional includes or other arbitrary code here...
// DO-NOT-DELETE splicer.end(NewPort.GoPort._includes)

namespace NewPort { 

  /**
   * Symbol "NewPort.GoPort" (version 1.0)
   */
  class GoPort_impl
  // DO-NOT-DELETE splicer.begin(NewPort.GoPort._inherits)
  // DO-NOT-DELETE splicer.end(NewPort.GoPort._inherits)
  {

  private:
    // Pointer back to IOR.
    // Use this to dispatch back through IOR vtable.
    GoPort self;

    // DO-NOT-DELETE splicer.begin(NewPort.GoPort._implementation)
    gov::cca::Services svc;
    // DO-NOT-DELETE splicer.end(NewPort.GoPort._implementation)

  private:
    // private default constructor (required)
    GoPort_impl() {} 

  public:
    // SIDL constructor (required)
    // Note: alternate Skel constructor doesn't call addref()
    // (fixes bug #275)
    GoPort_impl( struct NewPort_GoPort__object * s ) : self(s,true) { _ctor(); }

    // user defined construction
    void _ctor();

    // virtual destructor (required)
    virtual ~GoPort_impl() { _dtor(); }

    // user defined destruction
    void _dtor();

  public:

    /**
     * user defined non-static method.
     */
    void
    setService (
      /*in*/ ::gov::cca::Services svc
    )
    throw () 
    ;


    /**
     * Execute some encapsulated functionality on the component. 
     * Return 0 if ok, -1 if internal error but component may be 
     * used further, and -2 if error so severe that component cannot
     * be further used safely.
     */
    int32_t
    go() throw () 
    ;
  };  // end class GoPort_impl

} // end namespace NewPort

// DO-NOT-DELETE splicer.begin(NewPort.GoPort._misc)
// Put miscellaneous things here...
// DO-NOT-DELETE splicer.end(NewPort.GoPort._misc)

#endif
