/*

The MIT License

Copyright (c) 1997-2010 Center for the Simulation of Accidental Fires and 
Explosions (CSAFE), and  Scientific Computing and Imaging Institute (SCI), 
University of Utah.

License for the specific language governing rights and limitations under
Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.

*/


#include "ray.h"
#include "Consts.h"

#include <iostream>
#include <cstdlib>

using namespace std;


// get NoMedia from main function
ray::ray(const int &VolElementNo_,
	 const int &Ncx_,
	 const int &Ncy_,
	 const int &Ncz_,
	 const double &scat_threshold_){
  
  
  VolElementNo = VolElementNo_;
  Ncx = Ncx_;
  Ncy = Ncy_;
  Ncz = Ncz_;
  ghostX = Ncx + 2;
  ghostY = Ncy + 2;
  ghostTB = ghostX * ghostY;

  // scat_threshold is the fixed length to compare scat_len
  // thus as mesh refined, scattering possibility wont be affected.
  scat_threshold = scat_threshold_;
  
}

ray::~ray(){
}



  
void ray::set_emissS_vol(MTRand &MTrng,
			 double *sVol){
  
  double phi, theta;
  phi = 2 * pi * MTrng.randExc();
  sVol[2] = 1 - 2 *  MTrng.randExc(); // cos(theta), k
  
  theta = acos(sVol[2]); 
  sVol[0] = sin(theta) * cos( phi ); // i 
  sVol[1] = sin( theta ) * sin ( phi ) ;// j 
  
}


  
void ray::get_specular_s(double *spec_s){
  
  double sum;
  sum = dotProduct(directionVector, surface_n[surfaceFlag]);
  for ( int i = 0; i < 3; i ++ ) { 
    spec_s[i] = directionVector[i] - 2 * sum * ( * ( surface_n[surfaceFlag] + i ) );
  }
  
}


bool
ray::surfaceIntersect( const double *X,
                       const double *Y,
                       const double *Z,
                       const int *VolFeature ) {

  // s[0] = sin(theta) cos(phi)
  // s[1] = sin(theta) sin(phi)
  // s[2] = cos(theta)
  
  //  double xcheck, ycheck, zcheck;
  double xlow, xup, ylow, yup, zlow, zup;
  double cc;

  xx[0] = X[iIndex];
  xx[1] = X[iIndex+1];
  yy[0] = Y[jIndex];
  yy[1] = Y[jIndex+1];
  zz[0] = Z[kIndex];
  zz[1] = Z[kIndex+1];
  
//  cout << "in surfaceIntersect " << endl;
//   cout << "directionVector = "  << directionVector[0] << "i+ " <<
//     directionVector[1] << " j + " << directionVector[2] << "k" << endl;
//     cout << "dirChange = " << dirChange << endl;
//      cout << "in surfaceInterect xemiss = " << xemiss << endl;
//       cout << "in surfaceInterect yemiss = " << yemiss << endl;
//       cout << "in surfaceInterect zemiss = " << zemiss << endl;
      
  // if directionVector changes, then recalculate sign.
  if (dirChange) {
    for ( int i = 0; i < 3; i ++) {      
      // to inverse is necessary to check 1/-0.0 = - inf, and 1/0.0 = +inf    
      inv_directionVector[i] = 1/directionVector[i];
      sign[i] = (inv_directionVector[i] > 0);
    }
  }

//   cout << "xx[0] = " << xx[0] << endl;
//   cout << "xx[1] = " << xx[1] << endl;
//   cout << "yy[0] = " << yy[0] << endl;
//   cout << "yy[1] = " << yy[1] << endl;
//   cout << "zz[0] = " << zz[0] << endl;
//   cout << "zz[1] = " << zz[1] << endl;
  disX = ( xx[sign[0]] - xemiss ) * inv_directionVector[0];
  disY = ( yy[sign[1]] - yemiss ) * inv_directionVector[1];
  disZ = ( zz[sign[2]] - zemiss ) * inv_directionVector[2];
  
  if ( disX <= disY ) {
    disMin = disX;
    surfaceFlag = sign[0] ? RIGHT : LEFT;
  }
  else
    {
      disMin = disY;
      surfaceFlag = sign[1] ? BACK : FRONT;
    }


  if ( disZ <= disMin ) {
    disMin = disZ;    
    surfaceFlag = sign[2] ? TOP : BOTTOM;
  }
  
//   cout << "surfaceFlag = " << surfaceFlag << endl;
//   cout << "disX = " << disX << endl;
//   cout << "disY = " << disY << endl;
//   cout << "disZ = " << disZ << endl;
//   cout << "disMin = " << disMin << endl;
//   cout << "sign = " << sign[0] << "; " << sign[1] << "; " << sign[2] << endl;
  
//   if ( disMin < 0 ) {
//     cerr << "error disMin < 0 " << endl;
//     exit(1);
//   }

  
    // could possible hit on top surface
  if ( surfaceFlag == TOP ) {

    // hitSurfaceIndex's position indices
    hitSurfaceiIndex = iIndex;
    hitSurfacejIndex = jIndex;
    hitSurfacekIndex = kIndex + 1;
    
    xhit = directionVector[0] * disMin + xemiss;
    yhit = directionVector[1] * disMin + yemiss;
    zhit = Z[hitSurfacekIndex];
        
    // hit on top virtual surface
    if (VolFeature[(hitSurfaceiIndex+1) +
		   (hitSurfacejIndex+1) * ghostX +
		   (hitSurfacekIndex+1) *ghostTB]){
      
      // update next step's volume index i, j, k,
      // but note, not updating currentvIndex yet
      
      set_futurevIndex(hitSurfaceiIndex,
		       hitSurfacejIndex,
		       hitSurfacekIndex);
      
      // make sure that if not hit on realsurface and called hitSurfaceIndex
      // will return error
      hitSurfaceIndex = -1;	
      VIRTUAL =  1;
    }
    else{
      //	cout << "hit on top real " << endl;
      VIRTUAL = 0;
      
      set_futurevIndex(iIndex,
		       jIndex,
		       kIndex);
      
      hitSurfaceIndex = hitSurfaceiIndex + hitSurfacejIndex * Ncx;
      obReal = &obTop_ray;
    }
    
    //       cout << "hitSurfaceiIndex = " << hitSurfaceiIndex << endl;
    //       cout << "hitSurfacejIndex = " << hitSurfacejIndex << endl;
    //       cout << "hitSurfacekIndex = " << hitSurfacekIndex << endl;
    return true;
    
       
  }
  
  
  // could possible hit on bottom surface
  if ( surfaceFlag == BOTTOM ) {

    // hitSurfaceIndex's position indices
    hitSurfaceiIndex = iIndex;
    hitSurfacejIndex = jIndex;
    hitSurfacekIndex = kIndex;
    
    xhit = directionVector[0] * disMin + xemiss;
    yhit = directionVector[1] * disMin + yemiss;
    zhit = Z[hitSurfacekIndex];

    // hit on bottom virtual surface
    if (VolFeature[(hitSurfaceiIndex+1) +
		   (hitSurfacejIndex+1) *ghostX +
		   (hitSurfacekIndex-1+1) *ghostTB]){
      
      set_futurevIndex(hitSurfaceiIndex,
		       hitSurfacejIndex,
		       hitSurfacekIndex-1);
      
      hitSurfaceIndex = -1;		
      VIRTUAL =  1;
    }
    else{
      
      set_futurevIndex(iIndex,
		       jIndex,
		       kIndex);
      
      VIRTUAL = 0;
      hitSurfaceIndex = hitSurfaceiIndex + hitSurfacejIndex * Ncx;
      obReal = &obBottom_ray;
    }
    //       cout << "hitSurfaceiIndex = " << hitSurfaceiIndex << endl;
    //       cout << "hitSurfacejIndex = " << hitSurfacejIndex << endl;
    //       cout << "hitSurfacekIndex = " << hitSurfacekIndex << endl;      
    return true;
    

  }
  


  // could possible hit on front surface
  if ( surfaceFlag == FRONT ) {
    
    // hitSurfaceIndex's position indices
    hitSurfaceiIndex = iIndex;
    hitSurfacejIndex = jIndex;
    hitSurfacekIndex = kIndex;
    
    xhit = directionVector[0] * disMin + xemiss;
    yhit =  Y[hitSurfacejIndex];
    zhit = directionVector[2] * disMin + zemiss;
       
    // hit on front virtual surface
    if (VolFeature[(hitSurfaceiIndex+1) +
		   (hitSurfacejIndex-1+1) *ghostX +
		   (hitSurfacekIndex+1) *ghostTB]){
      
      set_futurevIndex(hitSurfaceiIndex,
		       hitSurfacejIndex-1,
		       hitSurfacekIndex);
      
      hitSurfaceIndex = -1;	
      VIRTUAL =  1;
    }
    else{
      set_futurevIndex(iIndex,
		       jIndex,
		       kIndex);
      	
      VIRTUAL = 0;      
      hitSurfaceIndex = hitSurfaceiIndex + hitSurfacekIndex * Ncx;
      obReal = &obFront_ray;
    }
    //       cout << "hitSurfaceiIndex = " << hitSurfaceiIndex << endl;
    //       cout << "hitSurfacejIndex = " << hitSurfacejIndex << endl;
    //       cout << "hitSurfacekIndex = " << hitSurfacekIndex << endl;      
    return true;
    
 
  }



  // could possible hit on back surface
  if ( surfaceFlag == BACK ) {
    
    // hitSurfaceIndex's position indices
    hitSurfaceiIndex = iIndex;
    hitSurfacejIndex = jIndex+1;
    hitSurfacekIndex = kIndex;
    
    xhit = directionVector[0] * disMin + xemiss;
    yhit = Y[hitSurfacejIndex];
    zhit = directionVector[2] * disMin + zemiss;
        
    // hit on back virtual surface
    if (VolFeature[(hitSurfaceiIndex+1) +
		   (hitSurfacejIndex+1) *ghostX +
		   (hitSurfacekIndex+1) *ghostTB]){

      set_futurevIndex(hitSurfaceiIndex,
		       hitSurfacejIndex,
		       hitSurfacekIndex);
      
      hitSurfaceIndex = -1;		
      VIRTUAL =  1;
    }
    else{

      set_futurevIndex(iIndex,
		       jIndex,
		       kIndex);
		
      VIRTUAL = 0;      
      hitSurfaceIndex = hitSurfaceiIndex + hitSurfacekIndex * Ncx;
      obReal = &obBack_ray;
    }
    //       cout << "hitSurfaceiIndex = " << hitSurfaceiIndex << endl;
    //       cout << "hitSurfacejIndex = " << hitSurfacejIndex << endl;
    //       cout << "hitSurfacekIndex = " << hitSurfacekIndex << endl;      
    return true;
    
  }
  
  
   
  // could possible hit on left surface
  if ( surfaceFlag == LEFT ) {
    
    // hitSurfaceIndex's position indices
    hitSurfaceiIndex = iIndex;
    hitSurfacejIndex = jIndex;
    hitSurfacekIndex = kIndex;
    
    xhit = X[hitSurfaceiIndex];
    yhit = directionVector[1] * disMin + yemiss;
    zhit = directionVector[2] * disMin + zemiss;
       
    // hit on left virtual surface
    if (VolFeature[(hitSurfaceiIndex-1+1) +
		   (hitSurfacejIndex+1) *ghostX +
		   (hitSurfacekIndex+1) *ghostTB]){

      set_futurevIndex(hitSurfaceiIndex-1,
		       hitSurfacejIndex,
		       hitSurfacekIndex);
      
      hitSurfaceIndex = -1;		
      VIRTUAL =  1;
    }
    else{
      //	cout << "hit on left real " << endl;
      set_futurevIndex(iIndex,
		       jIndex,
		       kIndex);
      		
      VIRTUAL = 0;      
      hitSurfaceIndex = hitSurfacejIndex + hitSurfacekIndex * Ncy;
      obReal = &obLeft_ray;
    }
    //      cout << "hitSurfaceiIndex = " << hitSurfaceiIndex << endl;
    //       cout << "hitSurfacejIndex = " << hitSurfacejIndex << endl;
    //       cout << "hitSurfacekIndex = " << hitSurfacekIndex << endl;      
    return true;

  }
  


  
  // could possible hit on right surface
  if ( surfaceFlag == RIGHT ) {
    
    // hitSurfaceIndex's position indices
    hitSurfaceiIndex = iIndex+1;
    hitSurfacejIndex = jIndex;
    hitSurfacekIndex = kIndex;
    
    xhit = X[hitSurfaceiIndex];
    yhit = directionVector[1] * disMin + yemiss;
    zhit = directionVector[2] * disMin + zemiss;
    
    // hit on right virtual surface
    if (VolFeature[(hitSurfaceiIndex+1) +
		   (hitSurfacejIndex+1) *ghostX +
		   (hitSurfacekIndex+1) *ghostTB]){

      set_futurevIndex(hitSurfaceiIndex,
		       hitSurfacejIndex,
		       hitSurfacekIndex);
      
      hitSurfaceIndex = -1;		
      VIRTUAL =  1;
    }
    else{

      set_futurevIndex(iIndex,
		       jIndex,
		       kIndex);
      		
      VIRTUAL = 0;      
      hitSurfaceIndex = hitSurfacejIndex + hitSurfacekIndex * Ncy;
      obReal = &obRight_ray;
    }
    //       cout << "hitSurfaceiIndex = " << hitSurfaceiIndex << endl;
    //       cout << "hitSurfacejIndex = " << hitSurfacejIndex << endl;
    //       cout << "hitSurfacekIndex = " << hitSurfacekIndex << endl;      
    return true;

  }


  cerr << " No surfaces hit!" << endl;
  return false; // no surface hit
  
} // end surfaceIntersect()








// Backward Left Intensity

// surfaceFlag, currentvIndex, currentIndex, are the private data members of the class ray
// scattering and absorption in medium
// get the Er beforehand for two cases: uniform T, non-uniform T

// store path Index, Index's path length ( might not need to be stored),
// left fraction

// modify scattering scheme.
// instead of picking a scattering length every time, pick scat-length once,
// then keep tracking if the ray goes to that scatter length yet.
// otherwise, if scat-length is too long, the ray will never get a chance to scatter
void ray::TravelInMediumInten(MTRand &MTrng,
			      VirtualSurface &obVirtual,
			      const double *kl_Vol,
			      const double *scatter_Vol,
			      const double *X,
			      const double *Y,
			      const double *Z,
			      const int *VolFeature,
			      double &PathLeft,
			      double &PathSurfaceLeft) {
  

  if ( !surfaceIntersect(X, Y, Z, VolFeature) ) {
    cout << "Error: did not find the intersection surface.\n";
    exit(1);
  }

  length = ray_length();
  //  cout << "length = " << length << endl;
  
  // calculate the energy lost during absorption in this currentvIndex
  double kl_m, scat_m;
  double random1, random2, random3, random;
  double sIncoming[3];
  double sumScat; // the sum of all the zigzag path within this subregion
  bool vIndexUpdate;
    
  vIndexUpdate = 0; // no update
  sumScat = 0;
  
  // look up vol property array for kl_m, and scat_m
  // got the currentvIndex from ray.cc function get_currentvIndex()
  // and this is based on iIndex, jIndex, kIndex
  kl_m = kl_Vol[currentvIndex];
  scat_m = scatter_Vol[currentvIndex];
  dirChange = 0; 

  // usually scattercoeff is uniform??
  // this only works for a uniform scattering coeff case.
  // or just go with it, everytime pick a different scat_len, but
  // consistently compare to the same straight_len
  
  do {
    
    // rng.RandomNumberGen(random);
    // the random number from drand48() or MersenneTwister could return [0,1)
    // to avoid log(zero), now we use log(1-random)
 //    cout << " ~~~~~~~~~~~~~~~~~~~do starts~~~~~~~~~~~~~~~~~~~~~~~" << endl;
//     cout << "currentvIndex = " << currentvIndex << endl;
//     cout << "iIndex = " << iIndex << endl;
//     cout << "jIndex = " << jIndex << endl;
//     cout << "kIndex = " << kIndex << endl;
    
    // only use scattering coeff to get the scat_len.
    if (!scat_will_happen)
      scat_len = - log( 1- MTrng.randExc() ) / scat_m;

    
    pre_straight_len = straight_len;
    straight_len += length;

 //      cout << "scat_len = " << scat_len << endl;
//       cout << "straight_len = " << straight_len << endl;
//       cout << "length = " << length << endl;
//       cout << "scat_will_happen = " << scat_will_happen << endl;
      
    // scattering happens instantly in this current cell
    if ( scat_len <= scat_threshold && scat_len <= straight_len && scat_len > pre_straight_len){
      
      // update current position
      // x = xemiss + Dist * s[i];
      // y = yemiss + Dist * s[j];
      // z = zemiss + Dist * s[k];
      dirChange = 1;
      xemiss = xstraight + scat_len * directionVector[0];
      yemiss = ystraight + scat_len * directionVector[1];
      zemiss = zstraight + scat_len * directionVector[2];
      sumScat = sumScat + (scat_len - pre_straight_len);

//       cout << "--------- scat happens instantly --- " << endl;

//       cout << "straight_len - length = " << pre_straight_len << endl;
//    cout << "before scattering, previous direction: " << endl;
//    cout << "directionVector = "  << directionVector[0] << "i+ " <<
//      directionVector[1] << " j + " << directionVector[2] << "k" << endl;
  
//       cout << "xstraight = " << xstraight << endl;
//       cout << "ystraight = " << ystraight << endl;
//       cout << "zstraight = " << zstraight << endl; 
//       cout << "new xemiss = " << xemiss << endl;
//       cout << "new yemiss = " << yemiss << endl;
//       cout << "new zemiss = " << zemiss << endl;
//       cout << "boundaries are " << endl;
//       cout << "xlow and xup = " << X[iIndex] << "; " << X[iIndex+1] << endl;
//       cout << "ylow and yup = " << Y[jIndex] << "; " << Y[jIndex+1] << endl;
//       cout << "zlow and zup = " << Z[kIndex] << "; " << Z[kIndex+1] << endl;
                    
 
      if ( xemiss > X[iIndex+1] || xemiss < X[iIndex] ){
	cerr << " xemiss out of bound " << endl;
	exit(1);
      }
      if ( yemiss > Y[jIndex+1] || yemiss < Y[jIndex] ){
	cerr << " yemiss out of bound " << endl;
	exit(1);
      }
      if ( zemiss > Z[kIndex+1] || zemiss < Z[kIndex] ){
	cerr << " zemiss out of bound " << endl;
	exit(1);
      }      
      
      // re-set straight Position
      set_straightP();
      straight_len = 0;
      scat_will_happen = false;
      
      // set s be the sIncoming
       for ( int i = 0; i < 3 ; i ++ ) sIncoming[i] = directionVector[i];
      
      // get a new direction vector s
      obVirtual.get_s(MTrng, sIncoming, directionVector);
      
//  cout << "after scattering, previous direction: " << endl;
//   cout << "directionVector = "  << directionVector[0] << "i+ " <<
//     directionVector[1] << " j + " << directionVector[2] << "k" << endl;
    
      // update on xhit, yhit, zhit, and ray_length
      if( surfaceIntersect(X, Y, Z, VolFeature) ) {
	length = ray_length();
      }
      else {
	cerr << " error @ not getting hit point coordinates after scattering!\n";
	exit(1); // terminate program
      }

      
    }
    else if ( !scat_will_happen && scat_len <= scat_threshold && VIRTUAL){
      // else if scat_len greater than current raylength
      // but smaller than the threshold, and ray hits on a virtual surface
      // scattering will happen in the future
      // but the scat_len might be too long that ray later will hit on a wall
//       cout << "---- scatter doesnot happen this time but will happen-- " << endl;
//         cout << "new xemiss = " << xemiss << endl;
//       cout << "new yemiss = " << yemiss << endl;
//       cout << "new zemiss = " << zemiss << endl;
      
//       cout << "new xhit = " << xhit << endl;
//       cout << "new yhit = " << yhit << endl;
//       cout << "new zhit = " << zhit << endl;
      
      scat_will_happen = true;
      sumScat = sumScat + length;
      
      // no matter if it is virtual or real,
      // if real, the PathSurfaceLeft will be updated again later
      PathSurfaceLeft = 1;      
      vIndexUpdate = 1;     
     
    }
    else if ( scat_will_happen && scat_len > straight_len){
      // within this subregion scatter doesnot happen
//       cout << "------------- scatter schedualed to happen but not yet------------- " << endl;
       
//         cout << "new xemiss = " << xemiss << endl;
//       cout << "new yemiss = " << yemiss << endl;
//       cout << "new zemiss = " << zemiss << endl;
//       cout << "new xhit = " << xhit << endl;
//       cout << "new yhit = " << yhit << endl;
//       cout << "new zhit = " << zhit << endl;
      
      sumScat = sumScat + length;
      // no matter if it is virtual or real,
      // if real, the PathSurfaceLeft will be updated again later
      PathSurfaceLeft = 1;      
      vIndexUpdate = 1;
 
      }
    else{
//             cout << "------------- the last else ------------- " << endl;
          
//             cout << "new xemiss = " << xemiss << endl;
//       cout << "new yemiss = " << yemiss << endl;
//       cout << "new zemiss = " << zemiss << endl;     
//       cout << "new xhit = " << xhit << endl;
//       cout << "new yhit = " << yhit << endl;
//       cout << "new zhit = " << zhit << endl;

      
      sumScat = sumScat + length;
      scat_will_happen = false;
      // no matter if it is virtual or real,
      // if real, the PathSurfaceLeft will be updated again later
      PathSurfaceLeft = 1;      
      vIndexUpdate = 1;
    }
    

    
    if ( !scat_will_happen && straight_len >= scat_threshold){
  //     cout << "=================== straight_len overceeds threshold==========" << endl;
//       cout << "straight_len = " << straight_len << endl;
      straight_len = 0;
      set_straightP(xhit, yhit, zhit);
      
 //        cout << "scat_len = " << scat_len << endl;
//       cout << "straight_len = " << straight_len << endl;
//       cout << "length = " << length << endl;
      
//         cout << "new xhit = " << xhit << endl;
//       cout << "new yhit = " << yhit << endl;
//       cout << "new zhit = " << zhit << endl;   
    }

    if ( abs(xemiss) >0.5 || abs(yemiss) >0.5 || abs(zemiss) >0.5 || abs(xhit) >0.5
	 || abs(yhit) > 0.5 || abs(zemiss) > 0.5)
      {
	cerr << "*******error out of bound!!!******* " << endl;
     cout << "new xhit = " << xhit << endl;
      cout << "new yhit = " << yhit << endl;
      cout << "new zhit = " << zhit << endl;
       cout << "new xemiss = " << xemiss << endl;
      cout << "new yemiss = " << yemiss << endl;
      cout << "new zemiss = " << zemiss << endl;
           
	exit(1);
      }

    
  } while ( ! vIndexUpdate );
  
  
  // update LeftFraction later

  //  extinc_medium =   (kl_m) * sumScat ;
 
  // store the a_n * delta l_n for each segement n
  PathLeft =  (kl_m) * sumScat ;
  
}



// For Intensity
void ray::hitRealSurfaceInten(MTRand &MTrng,
			      const double *absorb_surface,
			      const double *rs_surface,
			      const double *rd_surface,
			      double &PathSurfaceLeft){


  // for real surface,
  // PathLength stores the left percentage, 1 - alpha
  // PathIndex stores the index of the surface
  
  double alpha, rhos, rhod, ratio;
  double alpha_other, rhos_other, rhod_other;
  double spec_s[3];

  // dealing with the surface element
  // calculate surface energy first, then push its index back in

  // already done in scatter_medium function
  //  currentIndex = hitSurfaceIndex; // very important
  
  dirChange = 1;
  straight_len = 0;
  set_straightP(xhit, yhit, zhit);
  scat_will_happen = false;
  //  cout << "hit on real surface " << endl;
  
 //  cout << "before hit on realsurface, previous direction: " << endl;
//   cout << "directionVector = "  << directionVector[0] << "i+ " <<
//     directionVector[1] << " j + " << directionVector[2] << "k" << endl;
  
  alpha = absorb_surface[hitSurfaceIndex];
  rhos = rs_surface[hitSurfaceIndex];
  rhod = rd_surface[hitSurfaceIndex];
  //   cout << "hitSurfaceIndex inside = " << hitSurfaceIndex << endl;
  // rng.RandomNumberGen( random );
  // cout << "alpha = " << alpha << endl;
  if ( alpha == 1 ) ratio = 10; // black body
  else ratio = rhod / ( rhos + rhod );

  // the left part! the carrying on part
  PathSurfaceLeft = 1 - alpha;

  // check for the rest of the ray , if diffuse reflected or specular reflected
  if ( ratio <= 1   ) {
    // cout << "random for reflection = " << random << endl;
    if ( MTrng.randExc() <= ratio ) { // pure diffuse reflection
      // cout << "diffuse" << endl;
      // check which surface, top , bottom, front, back, left or right
      // must follow this order
      obReal->get_s(MTrng, directionVector);
      //      cout << "ray line 732 " << endl;
    }
    else { // pure specular reflection
      //  cout << "specular " << endl;
      //     cout << "ray line 735 " << endl;
      get_specular_s(spec_s);
      set_directionS(spec_s);
      
    }
    
  }

    
}  





  
