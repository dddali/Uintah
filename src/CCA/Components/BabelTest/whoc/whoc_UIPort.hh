// 
// File:          whoc_UIPort.hh
// Symbol:        whoc.UIPort-v1.0
// Symbol Type:   class
// Babel Version: 0.7.0
// SIDL Created:  20020813 11:08:08 CDT
// Generated:     20020813 11:08:09 CDT
// Description:   Client-side glue code for whoc.UIPort
// 
// WARNING: Automatically generated; changes will be lost
// 
// 

#ifndef included_whoc_UIPort_hh
#define included_whoc_UIPort_hh

// declare class before #includes
// (this alleviates circular #include guard problems)[BUG#393]
namespace whoc { 

  class UIPort;
} // end namespace whoc

// Some compilers need to define array template before the specializations
#ifndef included_SIDL_cxx_hh
#include "SIDL_cxx.hh"
#endif
namespace SIDL {
  template<>
  class array<whoc::UIPort>;
}

#ifndef included_SIDL_cxx_hh
#include "SIDL_cxx.hh"
#endif
#ifndef included_whoc_UIPort_IOR_h
#include "whoc_UIPort_IOR.h"
#endif
// 
// Includes for all method dependencies.
// 
#ifndef included_SIDL_BaseInterface_hh
#include "SIDL_BaseInterface.hh"
#endif

namespace whoc { 

  /**
   * Symbol "whoc.UIPort" (version 1.0)
   */
  class UIPort : public SIDL::StubBase {

    //////////////////////////////////////////////////
    // 
    // User Defined Methods
    // 

  public:

    /**
     * <p>
     * Add one to the intrinsic reference count in the underlying object.
     * Object in <code>SIDL</code> have an intrinsic reference count.
     * Objects continue to exist as long as the reference count is
     * positive. Clients should call this method whenever they
     * create another ongoing reference to an object or interface.
     * </p>
     * <p>
     * This does not have a return value because there is no language
     * independent type that can refer to an interface or a
     * class.
     * </p>
     */
    void
    addReference() throw ( SIDL::NullIORException ) 
    ;


    /**
     * Decrease by one the intrinsic reference count in the underlying
     * object, and delete the object if the reference is non-positive.
     * Objects in <code>SIDL</code> have an intrinsic reference count.
     * Clients should call this method whenever they remove a
     * reference to an object or interface.
     */
    void
    deleteReference() throw ( SIDL::NullIORException ) 
    ;


    /**
     * Return true if and only if <code>obj</code> refers to the same
     * object as this object.
     */
    bool
    isSame (
      /*in*/ SIDL::BaseInterface iobj
    )
    throw ( SIDL::NullIORException ) 
    ;



    /**
     * Check whether the object can support the specified interface or
     * class.  If the <code>SIDL</code> type name in <code>name</code>
     * is supported, then a reference to that object is returned with the
     * reference count incremented.  The callee will be responsible for
     * calling <code>deleteReference</code> on the returned object.  If
     * the specified type is not supported, then a null reference is
     * returned.
     */
    SIDL::BaseInterface
    queryInterface (
      /*in*/ std::string name
    )
    throw ( SIDL::NullIORException ) 
    ;



    /**
     * Return whether this object is an instance of the specified type.
     * The string name must be the <code>SIDL</code> type name.  This
     * routine will return <code>true</code> if and only if a cast to
     * the string type name would succeed.
     */
    bool
    isInstanceOf (
      /*in*/ std::string name
    )
    throw ( SIDL::NullIORException ) 
    ;



    /**
     * Execute some encapsulated functionality on the component. 
     * @return 0 if ok, -1 if internal error but component may be used further,
     * -2 if error so severe that component cannot be further used safely.
     */
    int32_t
    ui() throw ( SIDL::NullIORException ) 
    ;


    //////////////////////////////////////////////////
    // 
    // End User Defined Methods
    // (everything else in this file is specific to
    //  Babel's C++ bindings)
    // 

  public:
    typedef struct whoc_UIPort__object ior_t;
    typedef struct whoc_UIPort__external ext_t;
    typedef struct whoc_UIPort__sepv sepv_t;

    // default constructor
    UIPort() : d_self(0), d_weak_reference(false) { }

    // static constructor
    static whoc::UIPort _create();

    // default destructor
    virtual ~UIPort ();

    // copy constructor
    UIPort ( const UIPort& original );

    // assignment operator
    UIPort& operator= ( const UIPort& rhs );

    // conversion from ior to C++ class
    UIPort ( UIPort::ior_t* ior );

    // Alternate constructor: does not call addReference()
    // (sets d_weak_reference=isWeak)
    // For internal use by Impls (fixes bug#275)
    UIPort ( UIPort::ior_t* ior, bool isWeak );

    // conversion from a StubBase
    UIPort ( const SIDL::StubBase& base );

    ior_t* _get_ior() { return d_self; }

    const ior_t* _get_ior() const { return d_self; }

    void _set_ior( ior_t* ptr ) { d_self = ptr; }

    bool _is_nil() const { return (d_self==0); }

    bool _not_nil() const { return (d_self!=0); }

    bool operator !() const { return (d_self==0); }

  protected:
    virtual void* _cast(const char* type) const;

  private:
    // Pointer to SIDL's IOR type (one per instance)
    ior_t * d_self;

    // Weak references (used by Impl's only) don't add/deleteRef()
    bool d_weak_reference;

    // Pointer to external (DLL loadable) symbols (shared among instances)
    static const ext_t * s_ext;

  public:
    static const ext_t * _get_ext() throw ( SIDL::NullIORException );

  }; // end class UIPort
} // end namespace whoc

// array specialization
namespace SIDL {
  template<>
  class array<whoc::UIPort> : public array_mixin< struct whoc_UIPort__array,
    struct whoc_UIPort__object*, whoc::UIPort>{
  public:
    /**
     * default constructor
     */
    array() : array_mixin< struct whoc_UIPort__array,
      struct whoc_UIPort__object*, whoc::UIPort>() {}

    /**
     * default destructor
     */
    virtual ~array() { if ( d_array ) { deleteReference(); } }

    /**
     * copy constructor
     */
    array( const array< whoc::UIPort >& original ) {
      d_array = original.d_array;
      if ( d_array ) { addReference(); }
    }

    /**
     * assignment operator
     */
    array< whoc::UIPort >& operator=( const array< whoc::UIPort >& rhs ) {
      if ( d_array != rhs.d_array ) {
        if ( d_array ) { deleteReference(); }
        d_array=rhs.d_array;
        if ( d_array ) { addReference(); }
      }
      return *this;
    }

  /**
   * conversion from ior to C++ class
   * (constructor/casting operator)
   */
  array( struct whoc_UIPort__array* src ) : array_mixin< struct 
    whoc_UIPort__array, struct whoc_UIPort__object*, whoc::UIPort>(src) {}

  /**
   * static constructor: createRow
   */
  static array < whoc::UIPort >
  createRow(int32_t dimen, const int32_t lower[], const int32_t upper[]) {
    array < whoc::UIPort > a;
    a._set_ior( (struct whoc_UIPort__array*)
                SIDL_interface__array_createRow( dimen, lower, upper ) );
    return a;
  }

  /**
   * static constructor: createCol
   */
  static array < whoc::UIPort >
  createCol(int32_t dimen, const int32_t lower[], const int32_t upper[]) {
    array < whoc::UIPort > a;
    a._set_ior( (struct whoc_UIPort__array*)
                SIDL_interface__array_createCol( dimen, lower, upper ) );
    return a;
  }

  /**
   * static constructor: create1d
   */
  static array < whoc::UIPort >
  create1d(int32_t len) {
    array < whoc::UIPort > a;
    a._set_ior( (struct whoc_UIPort__array*)
                SIDL_interface__array_create1d(len));
    return a;
  }

  /**
   * static constructor: create2dCol
   */
  static array < whoc::UIPort >
  create2dCol(int32_t m, int32_t n) {
    array < whoc::UIPort > a;
    a._set_ior( (struct whoc_UIPort__array*)
                SIDL_interface__array_create2dCol(m,n));
    return a;
  }

  /**
   * static constructor: create2dRow
   */
  static array < whoc::UIPort >
  create2dRow(int32_t m, int32_t n) {
    array < whoc::UIPort > a;
    a._set_ior( (struct whoc_UIPort__array*)
                SIDL_interface__array_create2dRow(m,n));
    return a;
  }

  /**
   * constructor: slice
   */
  array < whoc::UIPort >
  slice( int dimen,
         const int32_t newElem[],
         const int32_t *srcStart = 0,
         const int32_t *srcStride = 0,
         const int32_t *newStart = 0) {
    array < whoc::UIPort > a;
    a._set_ior( (struct whoc_UIPort__array*)
                SIDL_interface__array_slice( (struct SIDL_interface__array *) 
      d_array,
                                   dimen, newElem, srcStart, srcStride,
      newStart) );
    return a;
  }

  /**
   * copy
   */
  void copy( const array<whoc::UIPort>& src ) {
    SIDL_interface__array_copy( (const struct SIDL_interface__array*) 
      src._get_ior(),(struct SIDL_interface__array*) _get_ior() );
  }

  /**
   * constructor: smart copy
   */
  void smartCopy() {
    struct SIDL_interface__array * p  = SIDL_interface__array_smartCopy(
      (struct SIDL_interface__array *) _get_ior() );
    if ( _not_nil() ) { deleteReference(); }
    _set_ior( (struct whoc_UIPort__array*) p);
  }

  /**
   * constructor: ensure
   */
  void ensure(int32_t dimen, int ordering ) {
    struct SIDL_interface__array * p  = SIDL_interface__array_ensure(
      (struct SIDL_interface__array *) d_array, dimen, ordering );
    if ( _not_nil() ) { deleteReference(); }
    _set_ior( (struct whoc_UIPort__array*) p);
  }
};
}

#endif
