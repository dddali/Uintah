
/*
 *  VectorFieldRG.h: Vector Fields defined on a Regular grid
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   March 1994
 *
 *  Copyright (C) 1994 SCI Group
 */

#include <Datatypes/VectorFieldRG.h>
#include <Classlib/String.h>
#include <Malloc/Allocator.h>
#include <iostream.h>

static Persistent* maker()
{
    return scinew VectorFieldRG;
}

PersistentTypeID VectorFieldRG::type_id("VectorFieldRG", "VectorField", maker);

VectorFieldRG::VectorFieldRG()
: VectorField(RegularGrid), nx(0), ny(0), nz(0)
{
}

VectorFieldRG::~VectorFieldRG()
{
}

Point VectorFieldRG::get_point(int i, int j, int k)
{
    double x=bmin.x()+diagonal.x()*double(i)/double(nx-1);
    double y=bmin.y()+diagonal.y()*double(j)/double(ny-1);
    double z=bmin.z()+diagonal.z()*double(k)/double(nz-1);
    return Point(x,y,z);
}

void VectorFieldRG::locate(const Point& p, int& ix, int& iy, int& iz)
{
    Vector pn=p-bmin;
    double dx=diagonal.x();
    double dy=diagonal.y();
    double dz=diagonal.z();
    double x=pn.x()*(nx-1)/dx;
    double y=pn.y()*(ny-1)/dy;
    double z=pn.z()*(nz-1)/dz;
    ix=(int)x;
    iy=(int)y;
    iz=(int)z;
}

#define VectorFIELDRG_VERSION 1

void VectorFieldRG::io(Piostream& stream)
{
    /*int version=*/stream.begin_class("VectorFieldRG", VectorFIELDRG_VERSION);
    // Do the base class first...
    VectorField::io(stream);

    // Save these since the VectorField doesn't
    Pio(stream, bmin);
    Pio(stream, bmax);
    if(stream.reading()){
	have_bounds=1;
	diagonal=bmax-bmin;
    }

    // Save the rest..
    Pio(stream, nx);
    Pio(stream, ny);
    Pio(stream, nz);
    Pio(stream, grid);
    stream.end_class();
}	

void VectorFieldRG::compute_bounds()
{
    // Nothing to do - we store the bounds in the base class...
}

int VectorFieldRG::interpolate(const Point& p, Vector& value)
{
    Vector pn=p-bmin;
    double dx=diagonal.x();
    double dy=diagonal.y();
    double dz=diagonal.z();
    double x=pn.x()*(nx-1)/dx;
    double y=pn.y()*(ny-1)/dy;
    double z=pn.z()*(nz-1)/dz;
    int ix=(int)x;
    int iy=(int)y;
    int iz=(int)z;
    int ix1=ix+1;
    int iy1=iy+1;
    int iz1=iz+1;
    if(ix<0 || ix1>=nx)return 0;
    if(iy<0 || iy1>=ny)return 0;
    if(iz<0 || iz1>=nz)return 0;
    double fx=x-ix;
    double fy=y-iy;
    double fz=z-iz;
    Vector x00=Interpolate(grid(ix, iy, iz), grid(ix1, iy, iz), fx);
    Vector x01=Interpolate(grid(ix, iy, iz1), grid(ix1, iy, iz1), fx);
    Vector x10=Interpolate(grid(ix, iy1, iz), grid(ix1, iy1, iz), fx);
    Vector x11=Interpolate(grid(ix, iy1, iz1), grid(ix1, iy1, iz1), fx);
    Vector y0=Interpolate(x00, x10, fy);
    Vector y1=Interpolate(x01, x11, fy);
    value=Interpolate(y0, y1, fz);
    return 1;
}

void VectorFieldRG::resize(int _nx, int _ny, int _nz)
{
    nx=_nx;
    ny=_ny;
    nz=_nz;
    grid.newsize(nx, ny, nz);
}

void VectorFieldRG::set_minmax(const Point& min,
			       const Point& max)
{
    bmin=min;
    bmax=max;
    have_bounds=1;
    diagonal=bmax-bmin;
}

VectorField* VectorFieldRG::clone()
{
    return scinew VectorFieldRG(*this);
}
