
#ifndef IMAGEMATERIAL_H
#define IMAGEMATERIAL_H 1

#include <Packages/rtrt/Core/Material.h>
#include <Packages/rtrt/Core/Array2.h>

namespace rtrt { 

class ImageMaterial : public Material {
public:
    enum Mode {
	Tile,
	Clamp,
	None
    };
private:
    bool flip_;
    bool valid_;

    double Kd;
    Color specular;
    double specpow;
    double refl;
    double transp;
    Array2<Color> image;
    Mode umode, vmode;
    Color outcolor;

    void read_image(char* texfile);
    void read_hdr_image(char* texfile);
public:
    ImageMaterial(int /* oldstyle */, char* filename, Mode umode, Mode vmode,
		  double Kd, const Color& specular,
		  double specpow, double refl=0);
    ImageMaterial(char* filename, Mode umode, Mode vmode,
		  double Kd, const Color& specular,
		  double specpow, double refl, 
		  double transp=0);
    ImageMaterial(char* filename, Mode umode, Mode vmode,
		  double Kd, const Color& specular,
		  double specpow, double refl=0);
    virtual ~ImageMaterial();
    virtual void shade(Color& result, const Ray& ray,
                       const HitInfo& hit, int depth, 
                       double atten, const Color& accumcolor,
                       Context* cx);
    void flip() { flip_ = !flip_; }
    bool valid() { return valid_; }
    void set_refl(double r)
      {
	refl = r;
      }
};

} // end namespace rtrt

#endif
