// 
// File:          framework_Component.cc
// Symbol:        framework.Component-v1.0
// Symbol Type:   class
// Babel Version: 0.7.0
// SIDL Created:  20020813 11:07:11 CDT
// Generated:     20020813 11:07:12 CDT
// Description:   Client-side glue code for framework.Component
// 
// WARNING: Automatically generated; changes will be lost
// 
// 

#ifndef included_framework_Component_hh
#include "framework_Component.hh"
#endif
#ifndef included_SIDL_BaseInterface_hh
#include "SIDL_BaseInterface.hh"
#endif
#ifndef included_SIDL_BaseClass_hh
#include "SIDL_BaseClass.hh"
#endif
#include "SIDL_String.h"
#include "babel_config.h"
#ifdef SIDL_DYNAMIC_LIBRARY
#include <stdio.h>
#include <stdlib.h>
#include "SIDL_Loader.hh"
#endif


//////////////////////////////////////////////////
// 
// User Defined Methods
// 


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
framework::Component::addReference(  )
throw ( SIDL::NullIORException ) 

{
  if ( d_self == 0 ) {
    throw SIDL::NullIORException( std::string (
      "Null IOR Pointer in \"framework::Component::addReference()\""
    ));
  }

  if ( !d_weak_reference ) {
    // pack args to dispatch to ior

    // dispatch to ior
    (*(d_self->d_epv->f_addReference))(d_self );
    // unpack results and cleanup

  }
}



/**
 * Decrease by one the intrinsic reference count in the underlying
 * object, and delete the object if the reference is non-positive.
 * Objects in <code>SIDL</code> have an intrinsic reference count.
 * Clients should call this method whenever they remove a
 * reference to an object or interface.
 */
void
framework::Component::deleteReference(  )
throw ( SIDL::NullIORException ) 

{
  if ( d_self == 0 ) {
    throw SIDL::NullIORException( std::string (
      "Null IOR Pointer in \"framework::Component::deleteReference()\""
    ));
  }

  if ( !d_weak_reference ) {
    // pack args to dispatch to ior

    // dispatch to ior
    (*(d_self->d_epv->f_deleteReference))(d_self );
    // unpack results and cleanup

    d_self = 0;
  }
}



/**
 * Return true if and only if <code>obj</code> refers to the same
 * object as this object.
 */
bool
framework::Component::isSame( /*in*/ SIDL::BaseInterface iobj )
throw ( SIDL::NullIORException ) 

{
  if ( d_self == 0 ) {
    throw SIDL::NullIORException( std::string (
      "Null IOR Pointer in \"framework::Component::isSame()\""
    ));
  }
  bool _result;
  // pack args to dispatch to ior
  SIDL_bool _local_result;
  // dispatch to ior
  _local_result = (*(d_self->d_epv->f_isSame))(d_self,
    /* in */ iobj._get_ior() );
  // unpack results and cleanup
  _result = (_local_result == TRUE);
  return _result;
}



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
framework::Component::queryInterface( /*in*/ std::string name )
throw ( SIDL::NullIORException ) 

{
  if ( d_self == 0 ) {
    throw SIDL::NullIORException( std::string (
      "Null IOR Pointer in \"framework::Component::queryInterface()\""
    ));
  }
  SIDL::BaseInterface _result;
  // pack args to dispatch to ior
  char * _local_name = SIDL_String_strdup( name.c_str() );
  // dispatch to ior
  _result = SIDL::BaseInterface( (*(d_self->d_epv->f_queryInterface))(d_self,
    /* in */ _local_name ));
  // unpack results and cleanup
  if (_result._not_nil()) {
    // IOR return and constructor both increment, only need one
    
    (*(_result._get_ior()->d_epv->f_deleteReference))(_result._get_ior(
    )->d_object);
  }
  SIDL_String_free( _local_name );
  return _result;
}



/**
 * Return whether this object is an instance of the specified type.
 * The string name must be the <code>SIDL</code> type name.  This
 * routine will return <code>true</code> if and only if a cast to
 * the string type name would succeed.
 */
bool
framework::Component::isInstanceOf( /*in*/ std::string name )
throw ( SIDL::NullIORException ) 

{
  if ( d_self == 0 ) {
    throw SIDL::NullIORException( std::string (
      "Null IOR Pointer in \"framework::Component::isInstanceOf()\""
    ));
  }
  bool _result;
  // pack args to dispatch to ior
  SIDL_bool _local_result;
  char * _local_name = SIDL_String_strdup( name.c_str() );
  // dispatch to ior
  _local_result = (*(d_self->d_epv->f_isInstanceOf))(d_self,
    /* in */ _local_name );
  // unpack results and cleanup
  _result = (_local_result == TRUE);
  SIDL_String_free( _local_name );
  return _result;
}



/**
 * Obtain Services handle, through which the component communicates with the
 * framework. This is the one method that every CCA Component must implement. 
 */
void
framework::Component::setServices( /*in*/ govcca::Services svc )
throw ( SIDL::NullIORException ) 

{
  if ( d_self == 0 ) {
    throw SIDL::NullIORException( std::string (
      "Null IOR Pointer in \"framework::Component::setServices()\""
    ));
  }

  // pack args to dispatch to ior

  // dispatch to ior
  (*(d_self->d_epv->f_setServices))(d_self, /* in */ svc._get_ior() );
  // unpack results and cleanup

}



//////////////////////////////////////////////////
// 
// End User Defined Methods
// (everything else in this file is specific to
//  Babel's C++ bindings)
// 

// static constructor
framework::Component
framework::Component::_create() {
  framework::Component self( (*_get_ext()->createObject)() );
  // NOTE: reference count == 2. 
  //   (1 from createObject, 1 from IOR->C++)
  // Decrement this count back down to one.
  (*(self.d_self->d_epv->f_deleteReference))(self.d_self);
  return self;
}

// default destructor
framework::Component::~Component () {
  if ( d_self != 0 ) {
    deleteReference();
  }
}

// copy constructor
framework::Component::Component ( const framework::Component& original ) {
  d_self = const_cast<ior_t*>(original.d_self);
  d_weak_reference = original.d_weak_reference;
  if (d_self != 0 ) {
    addReference();
  }
}

// assignment operator
framework::Component&
framework::Component::operator=( const framework::Component& rhs ) {
  if ( d_self != rhs.d_self ) {
    if ( d_self != 0 ) {
      deleteReference();
    }
    d_self = const_cast<ior_t*>(rhs.d_self);
    d_weak_reference = rhs.d_weak_reference;
    if ( d_self != 0 ) {
      addReference();
    }
  }
  return *this;
}

// conversion from ior to C++ class
framework::Component::Component ( framework::Component::ior_t* ior ) 
    : d_self( ior ), d_weak_reference(false) {
  if ( d_self != 0 ) {
    addReference();
  }
}

// Alternate constructor: does not call addReference()
// (sets d_weak_reference=isWeak)
// For internal use by Impls (fixes bug#275)
framework::Component::Component ( framework::Component::ior_t* ior,
  bool isWeak ) 
    : d_self( ior ), d_weak_reference(isWeak) { 
}

// conversion from a StubBase
framework::Component::Component ( const SIDL::StubBase& base )
{
  d_self = reinterpret_cast<ior_t*>(base._cast("framework.Component"));
  d_weak_reference = false;
  if (d_self != 0) {
    addReference();
  }
}

// protected method that implements casting
void* framework::Component::_cast(const char* type) const
{
  void* ptr = 0;
  if ( d_self != 0 ) {
    ptr = reinterpret_cast<void*>((*d_self->d_epv->f__cast)(d_self, type));
  }
  return ptr;
}

// Static data type
const framework::Component::ext_t * framework::Component::s_ext;

// private static method to get static data type
const framework::Component::ext_t *
framework::Component::_get_ext()
  throw (SIDL::NullIORException)
{
  if (! s_ext ) {
#ifdef SIDL_STATIC_LIBRARY
    s_ext = framework_Component__externals();
#else
    const ext_t *(*dll_f)(void) =
      (const ext_t *(*)(void)) SIDL::Loader::lookupSymbol(
        "framework_Component__externals");
    s_ext = (dll_f ? (*dll_f)() : NULL);
    if (!s_ext) {
      throw SIDL::NullIORException( std::string (
        "cannot find implementation for framework.Component; please set SIDL_DLL_PATH"
      ));
    }
#endif
  }
  return s_ext;
}

