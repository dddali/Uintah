
/*
 * Sphere.h: Sphere objects
 *
 *  Written by:
 *   Steven G. Parker & David Weinstein
 *   Department of Computer Science
 *   University of Utah
 *   April 1994
 *
 *  Copyright (C) 1994 SCI Group
 */

#ifndef SCI_Geom_Sphere_h
#define SCI_Geom_Sphere_h 1

#include <Geom/Geom.h>
#include <Geometry/Point.h>

class GeomSphere : public GeomObj {
public:
    Point cen;
    double rad;
    int nu;
    int nv;

    void adjust();
    void move(const Point&, double, int nu=20, int nv=10);

    GeomSphere(int nu=20, int nv=10);
    GeomSphere(const Point&, double, int nu=20, int nv=10);
    GeomSphere(const GeomSphere&);
    virtual ~GeomSphere();

    virtual GeomObj* clone();
    virtual void get_bounds(BBox&);

#ifdef SCI_OPENGL
    virtual void objdraw(DrawInfoOpenGL*, Material*);
#endif
    virtual void make_prims(Array1<GeomObj*>& free,
			    Array1<GeomObj*>& dontfree);
    virtual void intersect(const Ray& ray, Material*,
			   Hit& hit);
    virtual Vector normal(const Point& p);
};

#endif /* SCI_Geom_Sphere_h */
