
/*
 *  PartToGeom.cc:  Convert a Particle Set into geoemtry
 *
 *  Written 
 *   Steve Parker
 *   Department of Computer Science
 *   University of Utah
 *   July 1994
 *
 *  Copyright (C) 1994 SCI Group
 */

#include <Classlib/NotFinished.h>
#include <Dataflow/Module.h>
#include <Datatypes/GeometryPort.h>
#include <Geom/Color.h>
#include <Geom/Geom.h>
#include <Geom/Group.h>
#include <Geom/Material.h>
#include <Geom/Pt.h>
#include <Geom/Sphere.h>
#include <Datatypes/ParticleSetPort.h>
#include <Datatypes/ParticleSet.h>
#include <Datatypes/ColorMap.h>
#include <Datatypes/ColorMapPort.h>
#include <Malloc/Allocator.h>
#include <TCL/TCLvar.h>

class PartToGeom : public Module {
    ParticleSetIPort* iPart;
    ColorMapIPort *iCmap;
    GeometryOPort* ogeom;
    TCLdouble current_time;
    TCLdouble radius;
    TCLint drawspheres;
    int last_idx;
    int last_generation;
public:
    PartToGeom(const clString& id);
    PartToGeom(const PartToGeom&, int deep);
    virtual ~PartToGeom();
    virtual Module* clone(int deep);
    virtual void execute();
};

extern "C" {
Module* make_PartToGeom(const clString& id)
{
    return scinew PartToGeom(id);
}
}

PartToGeom::PartToGeom(const clString& id)
: Module("PartToGeom", id, Filter), current_time("current_time", id, this),
  radius("radius", id, this), drawspheres("drawspheres", id, this)
{
    // Create the input port
    iPart=scinew ParticleSetIPort(this, "Particles", ParticleSetIPort::Atomic);
    add_iport(iPart);
    iCmap=scinew ColorMapIPort(this, "ColorMap", ColorMapIPort::Atomic);
    add_iport(iCmap);
    ogeom=scinew GeometryOPort(this, "Geometry", GeometryIPort::Atomic);
    add_oport(ogeom);
    last_idx=-1;
    last_generation=-1;
    drawspheres.set(1);
    radius.set(0.05);
}

PartToGeom::PartToGeom(const PartToGeom&copy, int deep)
: Module(copy, deep), current_time("current_time", id, this),
  radius("radius", id, this), drawspheres("drawspheres", id, this)
{
    NOT_FINISHED("PartToGeom::PartToGeom");
}

PartToGeom::~PartToGeom()
{
}

Module* PartToGeom::clone(int deep)
{
    return scinew PartToGeom(*this, deep);
}

void PartToGeom::execute()
{
    ParticleSetHandle part;
    
    if (!iPart->get(part)){
	last_idx=-1;
	return;
    }

    //double time=current_time.get();
    Array1<double> timesteps;
    part->list_natural_times(timesteps);
    if(timesteps.size()==0){
	ogeom->delAll();
	last_idx=-1;
	return;
    }

    int timestep=0;
    /*    while(time>timesteps[timestep] && timestep<timesteps.size()-1)
	timestep++;

    if(timestep == last_idx && part->generation == last_generation)
	return;
    
    last_idx=timestep;
    last_generation=part->generation;
    */
    int posid=part->position_vector();
    Array1<Vector> pos;
    part->get(timestep, posid, pos);
    
    Array1<double> scalars;
    part->get(timestep, 0, scalars);

    // grap the color map from the input port
    ColorMapHandle cmh;
    ColorMap *cmap;
    if( iCmap->get( cmh ) )
      cmap = cmh.get_rep();
    else {
      // create a default colormap
      Array1<Color> rgb;
      Array1<float> rgbT;
      Array1<float> alphas;
      Array1<float> alphaT;
      rgb.add( Color(1,0,0) );
      rgb.add( Color(0,0,1) );
      rgbT.add(0.0);
      rgbT.add(1.0);
      alphas.add(1.0);
      alphas.add(1.0);
      alphaT.add(1.0);
      alphaT.add(1.0);
      
      cmap = new ColorMap(rgb,rgbT,alphas,alphaT,2);
    }

    int i;
    // find max scalar
    double max = -1e30;
    double min = 1e30;
    for( i = 0; i < scalars.size(); i++ ) {
      max = ( scalars[i] > max ) ? scalars[i] : max;
      min = ( scalars[i] < min ) ? scalars[i] : min;
    }
    
    if( drawspheres.get() == 1 ) {
      GeomGroup *obj = scinew GeomGroup;
      for (int i=0; i<pos.size();i++) {
	GeomSphere *sp = scinew GeomSphere(pos[i].asPoint(),radius.get());
	int index = cmap->rcolors.size() * ( (scalars[i] - min) /
					     (max - min + 1) ) ;
	obj->add( scinew GeomMaterial(sp,scinew Material( Color(1,1,1),
							  cmap->rcolors[index],
							  cmap->rcolors[index],
							  20 )));
      }
      ogeom->delAll();
      ogeom->addObj(obj, "Particles");      
    } else {
      GeomPts *obj = scinew GeomPts(pos.size());
      for(int i=0;i<pos.size();i++)
	obj->add(pos[i].asPoint(), scalars[i]);

      ogeom->delAll();
      ogeom->addObj(obj, "Particles");      
    }
    //    GeomMaterial* mat=scinew GeomMaterial(obj,
    //					  scinew Material(Color(0,0,0),
    //							  Color(0,.6,0), 
    //							  Color(.5,.5,.5),20));
    
    //    ogeom->delAll();
    //    ogeom->addObj(matl, "Particles");
}
