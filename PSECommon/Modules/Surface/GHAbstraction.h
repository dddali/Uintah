#ifndef _GHABSTRACTION_H_
#define _GHABSTRACTION_H_

/*
 *  GHAbstraction.h: ?
 *
 *  Written by:
 *   Author: ?
 *   Department of Computer Science
 *   University of Utah
 *   Date: ?
 *
 *  Copyright (C) 199? SCI Group
 */

#include <Dataflow/Module.h>
#include <CoreDatatypes/TriSurface.h>

class Model;

namespace PSECommon {
namespace Modules {

using SCICore::CoreDatatypes::TriSurface;
using PSECommon::Dataflow::Module;

class GHAbstraction {
public:
  GHAbstraction(TriSurface* surf); // init with a surface...

  void Simplify(int nfaces); // reduces model to nfaces...
  void DumpSurface(TriSurface* surf); // dumps reduced surface...

  void RDumpSurface(); // does the real thing...

  void AddPoint(double x, double y, double z);
  void AddFace(int, int, int);

  void SAddPoint(double x, double y, double z);
  void SAddFace(int,int,int); // adds it to TriSurface
  
  void InitAdd(); // inits model...
  void FinishAdd();
  // data for this guy...

  Model *M0;
  TriSurface *orig; // original surface...
  TriSurface *work; // working surface...

  Module *owner;
};

} // End namespace Modules
} // End namespace PSECommon

//
// $Log$
// Revision 1.1  1999/07/27 16:57:56  mcq
// Initial commit
//
// Revision 1.4  1999/05/06 20:17:12  dav
// added back PSECommon .h files
//
// Revision 1.2  1999/04/29 03:19:26  dav
// updates
//
// Revision 1.1.1.1  1999/04/24 23:12:31  dav
// Import sources
//
//

#endif
