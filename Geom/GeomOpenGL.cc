
/*
 *  GeomOpenGL.cc: Rendering for OpenGL windows
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   April 1994
 *
 *  Copyright (C) 1994 SCI Group
 */

#include <Geom/GeomOpenGL.h>
#include <Classlib/NotFinished.h>
#include <Geom/Cone.h>
#include <Geom/Cylinder.h>
#include <Geom/Disc.h>
#include <Geom/Geom.h>
#include <Geom/Group.h>
#include <Geom/HeadLight.h>
#include <Geom/Light.h>
#include <Geom/Line.h>
#include <Geom/Material.h>
#include <Geom/Pick.h>
#include <Geom/PointLight.h>
#include <Geom/Polyline.h>
#include <Geom/RenderMode.h>
#include <Geom/Sphere.h>
#include <Geom/Switch.h>
#include <Geom/Tetra.h>
#include <Geom/Torus.h>
#include <Geom/Tri.h>
#include <Geom/Tube.h>
#include <Geom/TriStrip.h>
#include <Geom/View.h>
#include <Geom/VCTri.h>
#include <Geom/VCTriStrip.h>
#include <Math/TrigTable.h>
#include <Math/Trig.h>
#include <Geometry/Plane.h>
#include <GL/gl.h>
#include <GL/glu.h>

#define MAX_MATL_STACK 100

void GeomObj::pre_draw(DrawInfoOpenGL* di, Material* matl, int lit)
{
    if(lit && di->lighting && !di->currently_lit){
	di->currently_lit=1;
	glEnable(GL_LIGHTING);
    }
    if((!lit || !di->lighting) && di->currently_lit){
	di->currently_lit=0;
	glDisable(GL_LIGHTING);
    }
    di->set_matl(matl);
}

DrawInfoOpenGL::DrawInfoOpenGL()
: current_matl(0)
{
    qobj=gluNewQuadric();
}

void DrawInfoOpenGL::reset()
{
    polycount=0;
    current_matl=0;
}

DrawInfoOpenGL::~DrawInfoOpenGL()
{
    gluDeleteQuadric(qobj);
}

void DrawInfoOpenGL::set_drawtype(DrawType dt)
{
    drawtype=dt;
    switch(drawtype){
    case DrawInfoOpenGL::WireFrame:
	gluQuadricNormals(qobj, GLU_NONE);
	gluQuadricDrawStyle(qobj, GLU_LINE);
	break;
    case DrawInfoOpenGL::Flat:
	gluQuadricNormals(qobj, GLU_NONE);
	gluQuadricDrawStyle(qobj, GLU_FILL);
	break;
    case DrawInfoOpenGL::Gouraud:
	gluQuadricNormals(qobj, GLU_FLAT);
	gluQuadricDrawStyle(qobj, GLU_FILL);
	break;
    case DrawInfoOpenGL::Phong:
	gluQuadricNormals(qobj, GLU_SMOOTH);
	gluQuadricDrawStyle(qobj, GLU_FILL);
	break;
    }
    
}

void DrawInfoOpenGL::set_matl(Material* matl)
{
    if(matl==current_matl)
	return;
    current_matl=matl;
    float color[4];
    matl->ambient.get_color(color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
    matl->diffuse.get_color(color);
    glColor4fv(color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
    matl->specular.get_color(color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
    matl->emission.get_color(color);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, matl->shininess);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color);
}

void GeomCone::draw(DrawInfoOpenGL* di, Material* matl)
{
    if(height < 1.e-6)return;
    pre_draw(di, matl, 1);
    glPushMatrix();
    glTranslated(bottom.x(), bottom.y(), bottom.z());
    glRotated(RtoD(zrotangle), zrotaxis.x(), zrotaxis.y(), zrotaxis.z());
    di->polycount+=2*(nu-1)*(nv-1);
    gluCylinder(di->qobj, bot_rad, top_rad, height, nu, nv);
    glPopMatrix();
}

void GeomContainer::draw(DrawInfoOpenGL* di, Material* matl)
{
    child->draw(di, matl);
}

void GeomCylinder::draw(DrawInfoOpenGL* di, Material* matl)
{
    if(height < 1.e-6)return;
    pre_draw(di, matl, 1);
    glPushMatrix();
    glTranslated(bottom.x(), bottom.y(), bottom.z());
    glRotated(RtoD(zrotangle), zrotaxis.x(), zrotaxis.y(), zrotaxis.z());
    di->polycount+=2*(nu-1)*(nv-1);
    gluCylinder(di->qobj, rad, rad, height, nu, nv);
    glPopMatrix();
}

void GeomDisc::draw(DrawInfoOpenGL* di, Material* matl)
{
    pre_draw(di, matl, 1);
    glPushMatrix();
    glTranslated(cen.x(), cen.y(), cen.z());
    glRotated(RtoD(zrotangle), zrotaxis.x(), zrotaxis.y(), zrotaxis.z());
    di->polycount+=2*(nu-1)*(nv-1);
    gluDisk(di->qobj, 0, rad, nu, nv);
    glPopMatrix();
}

void GeomGroup::draw(DrawInfoOpenGL* di, Material* matl)
{
    for (int i=0; i<objs.size(); i++)
	objs[i]->draw(di, matl);
}

void GeomLine::draw(DrawInfoOpenGL* di, Material* matl)
{
    pre_draw(di, matl, 0);
    di->polycount++;
    glBegin(GL_LINE_STRIP);
    glVertex3d(p1.x(), p1.y(), p1.z());
    glVertex3d(p2.x(), p2.y(), p2.z());
    glEnd();
}

void GeomMaterial::draw(DrawInfoOpenGL* di, Material* /* old_matl */)
{
    child->draw(di, matl.get_rep());
}

void GeomPick::draw(DrawInfoOpenGL* di, Material* matl)
{
    if(di->pickmode)
	glPushName((GLuint)this);
    child->draw(di, matl);
    if(di->pickmode)
	glPopName();
}

void GeomPolyline::draw(DrawInfoOpenGL* di, Material* matl)
{
    pre_draw(di, matl, 0);
    di->polycount+=pts.size()-1;
    glBegin(GL_LINE_STRIP);
    for(int i=0;i<pts.size();i++){
	Point p(pts[i]);
	glVertex3d(p.x(), p.y(), p.z());
    }
    glEnd();
}

// --------------------------------------------------

void GeomTube::draw(DrawInfoOpenGL* di, Material* matl)
{
    pre_draw(di, matl, 1);
    Array1<Point> c1; 
    Array1<Point> c2; 
    Vector n1, n2; 
    Point  p1, p2; 
    for(int i=0; i<pts.size(); i++) {
       c1 = make_circle(pts[i], rad[i], normal[i]); 
       c2 = make_circle(pts[i+1], rad[i+1], normal[i+1]); 

       // draw triangle strips
            glBegin(GL_TRIANGLE_STRIP);
	    for(int j=0;j<c1.size();j++){

		n1 = c1[j]-pts[i];
		glNormal3d(n1.x(), n1.y(), n1.z());
		p1 = c1[j];
		glVertex3d(p1.x(), p1.y(), p1.z());

		n2 = c2[j]-pts[i+1];
		glNormal3d(n2.x(), n2.y(), n2.z());
		p2 = c2[j];
		glVertex3d(p2.x(), p2.y(), p2.z());
	    }
		n1 = c1[0]-pts[i];
		glNormal3d(n1.x(), n1.y(), n1.z());
		p1 = c1[0];
		glVertex3d(p1.x(), p1.y(), p1.z());

		n2 = c2[0]-pts[i+1];
		glNormal3d(n2.x(), n2.y(), n2.z());
		p2 = c2[0];
		glVertex3d(p2.x(), p2.y(), p2.z());

	    glEnd();
     }
  }

// --------------------------------------------------

void GeomRenderMode::draw(DrawInfoOpenGL* di, Material* matl)
{
//    int save_lighting=di->lighting;
    NOT_FINISHED("GeomRenderMode");
    if(child){
	child->draw(di, matl);
	// We don't put things back if no children...
	
    }
}    

void GeomSphere::draw(DrawInfoOpenGL* di, Material* matl)
{
    pre_draw(di, matl, 1);
    glPushMatrix();
    glTranslated(cen.x(), cen.y(), cen.z());
    di->polycount+=2*(nu-1)*(nv-1);
    gluSphere(di->qobj, rad, nu, nv);
    glPopMatrix();
}

void GeomSwitch::draw(DrawInfoOpenGL* di, Material* matl)
{
   if(state)
      child->draw(di, matl);
}

void GeomTetra::draw(DrawInfoOpenGL* di, Material* matl)
{
    pre_draw(di, matl, 1);
    di->polycount+=4;
    switch(di->get_drawtype()){
    case DrawInfoOpenGL::WireFrame:
	glBegin(GL_LINE_STRIP);
	glVertex3d(p1.x(), p1.y(), p1.z());
	glVertex3d(p2.x(), p2.y(), p2.z());
	glVertex3d(p3.x(), p3.y(), p3.z());
	glVertex3d(p1.x(), p1.y(), p1.z());
	glVertex3d(p4.x(), p4.y(), p4.z());
	glVertex3d(p2.x(), p2.y(), p2.z());
	glVertex3d(p3.x(), p3.y(), p3.z());
	glVertex3d(p4.x(), p4.y(), p4.z());
	glEnd();
	break;
    case DrawInfoOpenGL::Flat:
	// this should be made into a tri-strip, but I couldn;t remember how...
	glBegin(GL_TRIANGLES);
	glVertex3d(p1.x(), p1.y(), p1.z());
	glVertex3d(p2.x(), p2.y(), p2.z());
	glVertex3d(p3.x(), p3.y(), p3.z());
	glEnd();
	glBegin(GL_TRIANGLES);
	glVertex3d(p1.x(), p1.y(), p1.z());
	glVertex3d(p2.x(), p2.y(), p2.z());
	glVertex3d(p4.x(), p4.y(), p4.z());
	glEnd();
	glBegin(GL_TRIANGLES);
	glVertex3d(p4.x(), p4.y(), p4.z());
	glVertex3d(p2.x(), p2.y(), p2.z());
	glVertex3d(p3.x(), p3.y(), p3.z());
	glEnd();
	glBegin(GL_TRIANGLES);
	glVertex3d(p1.x(), p1.y(), p1.z());
	glVertex3d(p4.x(), p4.y(), p4.z());
	glVertex3d(p3.x(), p3.y(), p3.z());
	glEnd();
	break;
    case DrawInfoOpenGL::Gouraud:
    case DrawInfoOpenGL::Phong:
	// this should be made into a tri-strip, but I couldn;t remember how...
	{
	    Vector n1(Plane(p1, p2, p3).normal());
	    glBegin(GL_TRIANGLES);
	    glNormal3d(n1.x(), n1.y(), n1.z());
	    glVertex3d(p1.x(), p1.y(), p1.z());
	    glVertex3d(p2.x(), p2.y(), p2.z());
	    glVertex3d(p3.x(), p3.y(), p3.z());
	    glEnd();
	    Vector n2(Plane(p1, p2, p4).normal());
	    glBegin(GL_TRIANGLES);
	    glNormal3d(n2.x(), n2.y(), n2.z());
	    glVertex3d(p1.x(), p1.y(), p1.z());
	    glVertex3d(p2.x(), p2.y(), p2.z());
	    glVertex3d(p4.x(), p4.y(), p4.z());
	    glEnd();
	    Vector n3(Plane(p4, p2, p3).normal());
	    glBegin(GL_TRIANGLES);
	    glNormal3d(n3.x(), n3.y(), n3.z());
	    glVertex3d(p4.x(), p4.y(), p4.z());
	    glVertex3d(p2.x(), p2.y(), p2.z());
	    glVertex3d(p3.x(), p3.y(), p3.z());
	    glEnd();
	    Vector n4(Plane(p1, p4, p3).normal());
	    glBegin(GL_TRIANGLES);
	    glNormal3d(n4.x(), n4.y(), n4.z());
	    glVertex3d(p1.x(), p1.y(), p1.z());
	    glVertex3d(p4.x(), p4.y(), p4.z());
	    glVertex3d(p3.x(), p3.y(), p3.z());
	    glEnd();
	    break;
	}
    }
}

void GeomTorus::draw(DrawInfoOpenGL* di, Material* matl)
{
    pre_draw(di, matl, 1);
    glPushMatrix();
    glTranslated(cen.x(), cen.y(), cen.z());
    glRotated(RtoD(zrotangle), zrotaxis.x(), zrotaxis.y(), zrotaxis.z());
    di->polycount+=2*(nu-1)*(nv-1);

    // Draw the torus
    SinCosTable tab1(nu, 0, 2*Pi);
    SinCosTable tab2(nv, 0, 2*Pi, rad2);
    SinCosTable tab2n(nv, 0, 2*Pi, rad2);
    int u,v;
    switch(di->get_drawtype()){

	
    case DrawInfoOpenGL::WireFrame:
	for(u=0;u<nu;u++){
	    double rx=tab1.sin(u);
	    double ry=tab1.cos(u);
	    glBegin(GL_LINE_LOOP);
	    for(v=1;v<nv;v++){
		double z=tab2.cos(v);
		double rad=rad1+tab2.sin(v);
		double x=rx*rad;
		double y=ry*rad;
		glVertex3d(x, y, z);
	    }
	    glEnd();
	}
	for(v=1;v<nv;v++){
	    double z=tab2.cos(v);
	    double rr=tab2.sin(v);
	    glBegin(GL_LINE_LOOP);
	    for(u=1;u<nu;u++){
		double rad=rad1+rr;
		double x=tab1.sin(u)*rad;
		double y=tab1.cos(u)*rad;
		glVertex3d(x, y, z);
	    }
	    glEnd();
	}
	break;
    case DrawInfoOpenGL::Flat:
	for(v=0;v<nv-1;v++){
	    double z1=tab2.cos(v);
	    double rr1=tab2.sin(v);
	    double z2=tab2.cos(v+1);
	    double rr2=tab2.sin(v+1);
	    glBegin(GL_TRIANGLE_STRIP);
	    for(u=0;u<nu;u++){
		double r1=rad1+rr1;
		double r2=rad1+rr2;
		double xx=tab1.sin(u);
		double yy=tab1.cos(u);
		double x1=xx*r1;
		double y1=yy*r1;
		double x2=xx*r2;
		double y2=yy*r2;
		glVertex3d(x1, y1, z1);
		glVertex3d(x2, y2, z2);
	    }
	    glEnd();
	}
	break;
    case DrawInfoOpenGL::Gouraud:
	for(v=0;v<nv-1;v++){
	    double z1=tab2.cos(v);
	    double rr1=tab2.sin(v);
	    double z2=tab2.cos(v+1);
	    double rr2=tab2.sin(v+1);
	    double nz=tab2n.cos(v);
	    glBegin(GL_TRIANGLE_STRIP);
	    for(u=0;u<nu;u++){
		double r1=rad1+rr1;
		double r2=rad1+rr2;
		double xx=tab1.sin(u);
		double yy=tab1.cos(u);
		double x1=xx*r1;
		double y1=yy*r1;
		double x2=xx*r2;
		double y2=yy*r2;
		glNormal3d(xx, yy, nz);
		glVertex3d(x1, y1, z1);
		glVertex3d(x2, y2, z2);
	    }
	    glEnd();
	}
	break;
    case DrawInfoOpenGL::Phong:
	for(v=0;v<nv-1;v++){
	    double z1=tab2.cos(v);
	    double rr1=tab2.sin(v);
	    double z2=tab2.cos(v+1);
	    double rr2=tab2.sin(v+1);
	    double nz1=tab2n.cos(v);
	    double nz2=tab2n.cos(v+1);
	    glBegin(GL_TRIANGLE_STRIP);
	    for(u=0;u<nu;u++){
		double r1=rad1+rr1;
		double r2=rad1+rr2;
		double xx=tab1.sin(u);
		double yy=tab1.cos(u);
		double x1=xx*r1;
		double y1=yy*r1;
		double x2=xx*r2;
		double y2=yy*r2;
		glNormal3d(xx, yy, nz1);
		glVertex3d(x1, y1, z1);
		glNormal3d(xx, yy, nz2);
		glVertex3d(x2, y2, z2);
	    }
	    glEnd();
	}
	break;	
    }
    glPopMatrix();
}

void GeomTri::draw(DrawInfoOpenGL* di, Material* matl)
{
    pre_draw(di, matl, 1);
    di->polycount++;
    switch(di->get_drawtype()){
    case DrawInfoOpenGL::WireFrame:
	glBegin(GL_LINE_LOOP);
	glVertex3d(p1.x(), p1.y(), p1.z());
	glVertex3d(p2.x(), p2.y(), p2.z());
	glVertex3d(p3.x(), p3.y(), p3.z());
	glEnd();
	break;
    case DrawInfoOpenGL::Flat:
	glBegin(GL_TRIANGLES);
	glVertex3d(p1.x(), p1.y(), p1.z());
	glVertex3d(p2.x(), p2.y(), p2.z());
	glVertex3d(p3.x(), p3.y(), p3.z());
	glEnd();
	break;
    case DrawInfoOpenGL::Gouraud:
    case DrawInfoOpenGL::Phong:
	glBegin(GL_TRIANGLES);
	glNormal3d(-n.x(), -n.y(), -n.z());
	glVertex3d(p1.x(), p1.y(), p1.z());
	glVertex3d(p2.x(), p2.y(), p2.z());
	glVertex3d(p3.x(), p3.y(), p3.z());
	glEnd();
	break;
    }
}

void GeomTriStrip::draw(DrawInfoOpenGL* di, Material* matl)
{
    pre_draw(di, matl, 1);
    if(pts.size() <= 2)
	return;
    di->polycount+=pts.size()-2;
    switch(di->get_drawtype()){
    case DrawInfoOpenGL::WireFrame:
	{
	    glBegin(GL_LINES);
	    Point p1(pts[0]);
	    Point p2(pts[1]);
	    glVertex3d(p1.x(), p1.y(), p1.z());
	    glVertex3d(p2.x(), p2.y(), p2.z());
	    for(int i=2;i<pts.size();i+=2){
		Point p3(pts[i]);
		Point p4(pts[i+1]);
		glVertex3d(p3.x(), p3.y(), p3.z());
		glVertex3d(p1.x(), p1.y(), p1.z());
		glVertex3d(p3.x(), p3.y(), p3.z());
		glVertex3d(p2.x(), p2.y(), p2.z());
		glVertex3d(p4.x(), p4.y(), p4.z());
		glVertex3d(p3.x(), p3.y(), p3.z());
		glVertex3d(p4.x(), p4.y(), p4.z());
		glVertex3d(p2.x(), p2.y(), p2.z());
		p1=p3;
		p2=p4;
	    }
	    glEnd();
	}
	break;
    case DrawInfoOpenGL::Flat:
	{
	    glBegin(GL_TRIANGLE_STRIP);
	    for(int i=0;i<pts.size();i++){
		Point p(pts[i]);
		glVertex3d(p.x(), p.y(), p.z());
	    }
	    glEnd();
	}
	break;
    case DrawInfoOpenGL::Gouraud:
    case DrawInfoOpenGL::Phong:
	{
	    glBegin(GL_TRIANGLE_STRIP);
	    for(int i=0;i<pts.size();i++){
		Vector n(norms[i]);
		glNormal3d(n.x(), n.y(), n.z());
		Point p(pts[i]);
		glVertex3d(p.x(), p.y(), p.z());
	    }
	    glEnd();
	}
	break;
    }
}

void GeomVCTriStrip::draw(DrawInfoOpenGL* di, Material* matl)
{
    if(pts.size() <= 2)
	return;
    di->polycount+=pts.size()-2;
    pre_draw(di, matl, 1);
    switch(di->get_drawtype()){
    case DrawInfoOpenGL::WireFrame:
	{
	    glBegin(GL_LINES);
	    Point p1(pts[0]);
	    Point p2(pts[1]);
	    glVertex3d(p1.x(), p1.y(), p1.z());
	    glVertex3d(p2.x(), p2.y(), p2.z());
	    for(int i=2;i<pts.size();i+=2){
		Point p3(pts[i]);
		Point p4(pts[i+1]);
		glVertex3d(p3.x(), p3.y(), p3.z());
		glVertex3d(p1.x(), p1.y(), p1.z());
		glVertex3d(p3.x(), p3.y(), p3.z());
		glVertex3d(p2.x(), p2.y(), p2.z());
		glVertex3d(p4.x(), p4.y(), p4.z());
		glVertex3d(p3.x(), p3.y(), p3.z());
		glVertex3d(p4.x(), p4.y(), p4.z());
		glVertex3d(p2.x(), p2.y(), p2.z());
		p1=p3;
		p2=p4;
	    }
	    glEnd();
	}
	break;
    case DrawInfoOpenGL::Flat:
	{
	    glBegin(GL_TRIANGLE_STRIP);
	    for(int i=0;i<pts.size();i++){
		Point p(pts[i]);
		glVertex3d(p.x(), p.y(), p.z());
	    }
	    glEnd();
	}
	break;
    case DrawInfoOpenGL::Gouraud:
	{
	    glBegin(GL_TRIANGLE_STRIP);
	    di->set_matl(mmatl[0].get_rep());
	    for(int i=0;i<pts.size();i++){
		Vector n(norms[i]);
		glNormal3d(n.x(), n.y(), n.z());
		Point p(pts[i]);
		glVertex3d(p.x(), p.y(), p.z());
	    }
	    glEnd();
	}
	break;
    case DrawInfoOpenGL::Phong:
	{
	    glBegin(GL_TRIANGLE_STRIP);
	    for(int i=0;i<pts.size();i++){
		Vector n(norms[i]);
		glNormal3d(n.x(), n.y(), n.z());
		Point p(pts[i]);
		di->set_matl(mmatl[i].get_rep());
		glVertex3d(p.x(), p.y(), p.z());
	    }
	    glEnd();
	}
	break;
    }
}

void GeomVCTri::draw(DrawInfoOpenGL* di, Material* matl) {
    di->polycount++;
    pre_draw(di, matl, 1);
    switch(di->get_drawtype()){
    case DrawInfoOpenGL::WireFrame:
	glBegin(GL_LINE_LOOP);
	glVertex3d(p1.x(), p1.y(), p1.z());
	glVertex3d(p2.x(), p2.y(), p2.z());
	glVertex3d(p3.x(), p3.y(), p3.z());
	glEnd();
	break;
    case DrawInfoOpenGL::Flat:
	glBegin(GL_TRIANGLES);
	glVertex3d(p1.x(), p1.y(), p1.z());
	glVertex3d(p2.x(), p2.y(), p2.z());
	glVertex3d(p3.x(), p3.y(), p3.z());
	glEnd();
	break;
    case DrawInfoOpenGL::Gouraud:
	glBegin(GL_TRIANGLES);
	glNormal3d(-n.x(), -n.y(), -n.z());
	di->set_matl(m1.get_rep());
	glVertex3d(p1.x(), p1.y(), p1.z());
	glVertex3d(p2.x(), p2.y(), p2.z());
	glVertex3d(p3.x(), p3.y(), p3.z());
	glEnd();
	break;
    case DrawInfoOpenGL::Phong:
	glBegin(GL_TRIANGLES);
	glNormal3d(-n.x(), -n.y(), -n.z());
	di->set_matl(m1.get_rep());
	glVertex3d(p1.x(), p1.y(), p1.z());
	di->set_matl(m2.get_rep());
	glVertex3d(p2.x(), p2.y(), p2.z());
	di->set_matl(m3.get_rep());
	glVertex3d(p3.x(), p3.y(), p3.z());
	glEnd();
	break;
    }
}

void PointLight::opengl_setup(const View&, DrawInfoOpenGL*, int& idx)
{
    int i=idx++;
    float f[4];
    f[0]=p.x(); f[1]=p.y(); f[2]=p.z(); f[3]=1.0;
    glLightfv(GL_LIGHT0+i, GL_POSITION, f);
    c.get_color(f);
    glLightfv(GL_LIGHT0+i, GL_DIFFUSE, f);
    glLightfv(GL_LIGHT0+i, GL_SPECULAR, f);
}

void HeadLight::opengl_setup(const View& view, DrawInfoOpenGL*, int& idx)
{
    int i=idx++;
    Point p(view.eyep);
    float f[4];
    f[0]=p.x(); f[1]=p.y(); f[2]=p.z(); f[3]=1.0;
    glLightfv(GL_LIGHT0+i, GL_POSITION, f);
    c.get_color(f);
    glLightfv(GL_LIGHT0+i, GL_DIFFUSE, f);
    glLightfv(GL_LIGHT0+i, GL_SPECULAR, f);
}

