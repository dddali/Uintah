//  MPMMaterial.cc
//

#include "MPMMaterial.h"
#include "ConstitutiveModel.h"
#include <SCICore/Geometry/IntVector.h>
#include <Uintah/Grid/Region.h>
#include <Uintah/Grid/CellIterator.h>
#include <Uintah/Components/MPM/GeometrySpecification/GeometryPieceFactory.h>
#include <Uintah/Components/MPM/GeometrySpecification/UnionGeometryPiece.h>
#include <Uintah/Components/MPM/GeometrySpecification/GeometryObject.h>
#include <Uintah/Exceptions/ParameterNotFound.h>
#include <iostream>
#include "ConstitutiveModelFactory.h"
using namespace std;
using namespace Uintah::MPM;
using namespace Uintah;
using namespace SCICore::Geometry;


MPMMaterial::MPMMaterial(ProblemSpecP& ps)
{
   // Constructor

  // Follow the layout of the input file
  // Steps:
  // 1.  Determine the type of constitutive model and create it.
  // 2.  Get the general properties of the material such as
  //     density, toughness, thermal_conductivity, specific_heat.
  // 3.  Loop through all of the geometry pieces that make up a single
  //     geometry object.
  // 4.  Within the geometry object, assign the boundary conditions
  //     to the object.
  // 5.  Assign the velocity field.

  // Step 1 -- create the constitutive gmodel.

   d_cm = ConstitutiveModelFactory::create(ps);
   if(!d_cm)
      throw ParameterNotFound("No constitutive model");
   std::cerr << "works here after cm factory" << std::endl;

   // Step 2 -- get the general material properties

   ps->require("density",d_density);
   ps->require("toughness",d_toughness);
   ps->require("thermal_conductivity",d_thermal_cond);
   ps->require("specific_heat",d_spec_heat);
   ps->require("temperature",d_temp);

   // Step 3 -- Loop through all of the pieces in this geometry object

   int piece_num = 0;
   for (ProblemSpecP geom_obj_ps = ps->findBlock("geom_object");
	geom_obj_ps != 0; 
	geom_obj_ps = geom_obj_ps->findNextBlock("geom_object") ) {

      vector<GeometryPiece*> pieces;
      GeometryPieceFactory::create(geom_obj_ps, pieces);

      GeometryPiece* mainpiece;
      if(pieces.size() == 0){
	 throw ParameterNotFound("No piece specified in geom_object");
      } else if(pieces.size() > 1){
	 mainpiece = new UnionGeometryPiece(pieces);
      } else {
	 mainpiece = pieces[0];
      }

      piece_num++;
      cerr << "piece: " << piece_num << '\n';
      IntVector res;
      geom_obj_ps->require("res",res);
      cerr << piece_num << ": res: " << res << '\n';
      d_geom_objs.push_back(new GeometryObject(mainpiece, res));

      // Step 4 -- Assign the boundary conditions to the object

      
      // Step 5 -- Assign the velocity field
      int vf;
      ps->require("velocity_field",vf);
      setVFIndex(vf);
      
	
            
   }


}

MPMMaterial::~MPMMaterial()
{
  // Destructor
}

ConstitutiveModel * MPMMaterial::getConstitutiveModel()
{
  // Return the pointer to the constitutive model associated
  // with this material

  return d_cm;
}

particleIndex MPMMaterial::countParticles(const Region* region) const
{
   particleIndex sum = 0;
   for(int i=0; i<d_geom_objs.size(); i++)
      sum+= countParticles(d_geom_objs[i], region);
   return sum;
}

void MPMMaterial::createParticles(ParticleVariable<Point>& position,
				  const Region* region)
{
   particleIndex start = 0;
   for(int i=0; i<d_geom_objs.size(); i++)
      start += createParticles(d_geom_objs[i], start, position, region);
}

particleIndex MPMMaterial::countParticles(GeometryObject* obj,
					  const Region* region) const
{
   GeometryPiece* piece = obj->getPiece();
   Box b1 = piece->getBoundingBox();
   Box b2 = region->getBox();
   Box b = b1.intersect(b2);
   if(b.degenerate())
      return 0;

   IntVector ppc = obj->getNumParticlesPerCell();
   Vector dxpp = region->dCell()*obj->getNumParticlesPerCell();
   Vector dcorner = dxpp*0.5;

   particleIndex count = 0;
   for(CellIterator iter = region->getCellIterator(b); !iter.done(); iter++){
      Point lower = region->nodePosition(*iter) + dcorner;
      for(int ix=0;ix < ppc.x(); ix++){
	 for(int iy=0;iy < ppc.y(); iy++){
	    for(int iz=0;iz < ppc.z(); iz++){
	       IntVector idx(ix, iy, iz);
	       Point p = lower + dxpp*idx;
	       if(piece->inside(p))
		  count++;
	    }
	 }
      }
   }
   cerr << "Count1 for obj: " << count << '\n';
   return count;
}


particleIndex MPMMaterial::createParticles(GeometryObject* obj,
					   particleIndex start,
					   ParticleVariable<Point>& position,
					   const Region* region)
{
   GeometryPiece* piece = obj->getPiece();
   Box b1 = piece->getBoundingBox();
   Box b2 = region->getBox();
   Box b = b1.intersect(b2);
   if(b.degenerate())
      return 0;

   IntVector ppc = obj->getNumParticlesPerCell();
   Vector dxpp = region->dCell()*obj->getNumParticlesPerCell();
   Vector dcorner = dxpp*0.5;

   particleIndex count = 0;
   for(CellIterator iter = region->getCellIterator(b); !iter.done(); iter++){
      Point lower = region->nodePosition(*iter) + dcorner;
      for(int ix=0;ix < ppc.x(); ix++){
	 for(int iy=0;iy < ppc.y(); iy++){
	    for(int iz=0;iz < ppc.z(); iz++){
	       IntVector idx(ix, iy, iz);
	       Point p = lower + dxpp*idx;
	       if(piece->inside(p))
		  position[start+count++]=p;
	    }
	 }
      }
   }
   cerr << "Count2 for obj: " << count << '\n';
   return count;
}

