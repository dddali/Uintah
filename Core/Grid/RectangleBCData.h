#ifndef UINTAH_GRID_RectangleBCData_H
#define UINTAH_GRID_RectangleBCData_H

#include <Packages/Uintah/Core/Grid/BoundCondData.h>
#include <Packages/Uintah/Core/Grid/BCDataBase.h>
#include <Core/Geometry/Vector.h>
#include <Core/Geometry/Point.h>
#include <Packages/Uintah/Core/ProblemSpec/ProblemSpecP.h>
#include <sgi_stl_warnings_off.h>
#include <vector>
#include <sgi_stl_warnings_on.h>

namespace Uintah {
using namespace SCIRun;

/**************************************

CLASS
   RectangleBCData
   
   
GENERAL INFORMATION

   RectangleBCData.h

   John A. Schmidt
   Department of Mechanical Engineering
   University of Utah

   Center for the Simulation of Accidental Fires and Explosions (C-SAFE)
  
   Copyright (C) 2000 SCI Group

KEYWORDS
   RectangleBCData

DESCRIPTION
   Long description...
  
WARNING
  
****************************************/

   class RectangleBCData : public BCDataBase  {
   public:
     RectangleBCData();
     RectangleBCData(BCData& bc);
     RectangleBCData(const std::string& type);
     RectangleBCData(ProblemSpecP& ps);
     RectangleBCData(Point& low, Point& up);
     virtual ~RectangleBCData();
     RectangleBCData* clone();
     void addBCData(BCData& bc);
     void getBCData(BCData& bc) const;
     void setBoundaryIterator(std::vector<IntVector>& b);
     void setInteriorIterator(std::vector<IntVector>& i);
     void setSFCXIterator(std::vector<IntVector>& i);
     void setSFCYIterator(std::vector<IntVector>& i);
     void setSFCZIterator(std::vector<IntVector>& i);
     void getBoundaryIterator(std::vector<IntVector>& b) const;
     void getInteriorIterator(std::vector<IntVector>& i) const;
     void getSFCXIterator(std::vector<IntVector>& i) const;
     void getSFCYIterator(std::vector<IntVector>& i) const;
     void getSFCZIterator(std::vector<IntVector>& i) const;
     bool inside(const Point& p) const;

   private:
     BCData d_bc;
     Point d_min,d_max;

     std::vector<IntVector> boundary,interior,sfcx,sfcy,sfcz;
   };

} // End namespace Uintah

#endif




