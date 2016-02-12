/*
 * The MIT License
 *
 * Copyright (c) 1997-2016 The University of Utah
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef UINTAH_CORE_GRID_LEVEL_H
#define UINTAH_CORE_GRID_LEVEL_H

#include <CCA/Ports/LoadBalancer.h>

#include <Core/Disclosure/TypeDescription.h>
#include <Core/Containers/OffsetArray1.h>
#include <Core/Grid/GridP.h>
#include <Core/Grid/Grid.h>
#include <Core/Grid/LevelP.h>
#include <Core/Util/Handle.h>
#include <Core/Util/RefCounted.h>
#include <Core/Thread/CrowdMonitor.h>

#ifdef max
// some uintah 3p utilities define max, so undefine it before BBox chokes on it.
#  undef max
#endif

#include <Core/Geometry/BBox.h>
#include <Core/Geometry/Point.h>
#include <Core/Geometry/IntVector.h>
#include <Core/ProblemSpec/ProblemSpecP.h>
#include <Core/Grid/FixedVector.h>
#include <Core/Grid/Variables/ComputeSet.h>

#include <bitset>
#include <map>
#include <vector>

namespace Uintah {

using SCIRun::Vector;
using SCIRun::Point;
using SCIRun::IntVector;
using SCIRun::BBox;
using SCIRun::OffsetArray1;
using SCIRun::CrowdMonitor;

class PatchBVH;
class BoundCondBase;
class Box;
class Patch;
class Task;

class LevelFlags {
    public:

      LevelFlags() {
        states.reset();
      }

      enum FlagState {
        isAMR,
        isMultiScale,
        isInterface,
        isPeriodicX,
        isPeriodicY,
        isPeriodicZ,
        isIndependent,
        num_flags
      };

      void set(FlagState flag, bool value) {
        states.set(flag, value);
      }

      void clear(FlagState state) {
        states.reset(state);
      }

      bool test(FlagState state) const {
        return states.test(state);
      }

      bool none() const {
        return states.none();
      }

    private:
      std::bitset<num_flags> states;
  };

/**************************************

CLASS
   Level
   
   Just a container class that manages a set of Patches that
   make up this level.

GENERAL INFORMATION

   Level.h

   Steven G. Parker
   Department of Computer Science
   University of Utah

   Center for the Simulation of Accidental Fires and Explosions (C-SAFE)
  

KEYWORDS
   Level

DESCRIPTION
  
  
****************************************/

class Level : public RefCounted {

public:

  Level(        Grid       * grid
        , const Point      & anchor
        , const Vector     & dcell
        ,       int          index
        ,       IntVector    refinementRatio
        ,       LevelFlags & flags
        ,       int          id = -1 );

  virtual ~Level();
  
  class Compare {
    public:
      inline bool operator()(const Level* l1, const Level* l2) const {
        return (l1 != 0 && l2 !=0) ? (l1->getIndex() < l2->getIndex()) : ((l2 !=0) ? true : false);
      }
    private:
  };

  void setPatchDistributionHint( const IntVector& patchDistribution );

  void setBCTypes();
     
  typedef std::vector<Patch*>::iterator patchIterator;
  typedef std::vector<Patch*>::const_iterator const_patchIterator;

  const_patchIterator patchesBegin() const;
  patchIterator       patchesBegin();
  const_patchIterator patchesEnd() const;
  patchIterator       patchesEnd();

  const Patch* getPatch(int index) const { return d_realPatches[index]; }

  // go through the virtual ones too
  const_patchIterator allPatchesBegin() const;
  const_patchIterator allPatchesEnd() const;
      
  Patch* addPatch( const IntVector & extraLowIndex,
                   const IntVector & extraHighIndex,
                   const IntVector & lowIndex,
                   const IntVector & highIndex,
                         Grid      * grid );
      
  Patch* addPatch(  const IntVector & extraLowIndex
                  , const IntVector & extraHighIndex
                  , const IntVector & lowIndex
                  , const IntVector & highIndex
                  ,       Grid      * grid
                  ,       int         ID );

  // Move up and down the hierarchy
  const LevelP & getCoarserLevel() const;
  const LevelP & getFinerLevel()   const;
  bool           hasCoarserLevel() const;
  bool           hasFinerLevel()   const;

  IntVector      mapNodeToCoarser( const IntVector& idx ) const;
  IntVector      mapNodeToFiner( const IntVector& idx ) const;
  IntVector      mapCellToCoarser( const IntVector& idx, int level_offset = 1 ) const;
  IntVector      mapCellToFiner( const IntVector& idx ) const;

  //////////
  // Find a patch containing the point, return 0 if non exists
  const Patch* getPatchFromPoint( const Point&, const bool includeExtraCells ) const;

  //////////
  // Find a patch containing the cell or node, return 0 if non exists
  const Patch* getPatchFromIndex( const IntVector&, const bool includeExtraCells ) const;

  void finalizeLevel();

  void finalizeLevel( bool periodicX, bool periodicY, bool periodicZ );

  void assignBCS( const ProblemSpecP& ps, LoadBalancer* lb );
      
  int numPatches() const;

  long totalCells() const;

  void getSpatialRange( BBox& b) const {b.extend(d_spatial_range );};

  BBox getSpatialRange() const {
    BBox returnBox = d_spatial_range;
    return(returnBox);
  }

  void getInteriorSpatialRange(BBox& b) const {b.extend(d_int_spatial_range);};

  void findIndexRange( IntVector& lowIndex, IntVector& highIndex ) const { findNodeIndexRange(lowIndex, highIndex); }

  void findNodeIndexRange( IntVector& lowIndex, IntVector& highIndex ) const;

  void findCellIndexRange( IntVector& lowIndex, IntVector& highIndex ) const;

  void findInteriorIndexRange( IntVector& lowIndex, IntVector& highIndex ) const { findInteriorNodeIndexRange(lowIndex, highIndex); }

  void findInteriorNodeIndexRange(  IntVector & lowIndex
                                  , IntVector & highIndex ) const;

  void findInteriorCellIndexRange(  IntVector & lowIndex
                                  , IntVector & highIndex ) const;
                                  
  void computeVariableExtents(const TypeDescription::Type TD,
                              IntVector& lo,
                              IntVector& hi ) const;
      
  void performConsistencyCheck() const;

  GridP getGrid() const;
     
  const LevelP& getRelativeLevel( int offset ) const;

  int getIndexWithinSubset() const {
    return d_subsetIndex;
  }

  void setIndexWithinSubset(int inputIndex) {
    d_subsetIndex = inputIndex;
  }

  void setPerProcSubsetPatchSetIndex( const int &index) {
    d_levelSubsetPerProcPatchSetIndex = index;
  }

  int getPerProcSubsetPatchSetIndex() const {
    return d_levelSubsetPerProcPatchSetIndex;
  }

  // Grid spacing
  inline Vector dCell() const { return d_dcell; }
  
  inline void setdCell( Vector spacing ) { d_dcell = spacing; }

  inline const LevelSubset* getLevelSubset() const { return d_subset; }

  LevelSubset* setLevelSubset( LevelSubset* subset );

  /**
   * Returns the cell volume dx*dy*dz. This will not work for stretched grids.
   */
  double cellVolume() const {
    if (isStretched()) {
      throw InternalError( "Cell Volume is not unique for stretched meshes, therefore, you cannot use Patch::cellVolume() or Level::cellVolume().", __FILE__, __LINE__);
    }
    return d_dcell.x()*d_dcell.y()*d_dcell.z();
  }

  /**
   * Returns the cell area dx*dy, dx*dz, or dy*dz. This will not work for stretched grids.
   */
  double cellArea( Vector unitNormal ) const {
    if (isStretched()) {
      throw InternalError( "Cell area is not unique for stretched meshes, therefore, you cannot use Patch::cellArea or Level::cellArea.", __FILE__, __LINE__);
    }
    Vector areas(d_dcell.y() * d_dcell.z(), d_dcell.x() * d_dcell.z(), d_dcell.x() * d_dcell.y());
    return Dot(areas, unitNormal);
  }

  inline Point getAnchor() const { return d_anchor; }

  // For stretched grids
  inline bool isStretched() const { return d_stretched; }

  inline bool isAMR() const { return d_flags.test(LevelFlags::isAMR); }
  void setAMRStatus(bool status) { d_flags.set(LevelFlags::isAMR, status); }

  inline bool isMultiScale() const {return d_flags.test(LevelFlags::isMultiScale); }
  void setMultiScaleStatus(bool status) { d_flags.set(LevelFlags::isMultiScale, status); }

  inline bool isIndependent() const {return d_flags.test(LevelFlags::isIndependent); }
  void setIndependentStatus(bool status) { d_flags.set(LevelFlags::isIndependent, status); }

  void getCellWidths(Grid::Axis axis, OffsetArray1<double>& widths) const;
  void getFacePositions(Grid::Axis axis, OffsetArray1<double>& faces) const;
  void setStretched(Grid::Axis axis, const OffsetArray1<double>& faces);

  void      setExtraCells( const IntVector& ec );

  inline IntVector getExtraCells() const { return d_extraCells; }

  Point     getNodePosition( const IntVector& ) const;
  Point     getCellPosition( const IntVector& ) const;
  IntVector getCellIndex(    const Point& ) const;
  Point     positionToIndex( const Point& ) const;

  Box getBox(const IntVector&, const IntVector&) const;

  static const int MAX_PATCH_SELECT = 32;
  typedef FixedVector<const Patch*, MAX_PATCH_SELECT> selectType;
      

  void selectPatches(  const IntVector  &
                     , const IntVector  &
                     ,       selectType &
                     ,       bool         withExtraCells = false
                     ,       bool         cache = true ) const;

  bool containsPointIncludingExtraCells( const Point& ) const;
  bool containsPoint( const Point& ) const;
  bool containsCell( const IntVector& ) const;

  // IntVector whose elements are each 1 or 0 specifying whether there
  // are periodic boundaries in each dimension (1 means periodic).
  inline IntVector getPeriodicBoundaries() const { return d_periodicBoundaries; }

  // The eachPatch() function returns a PatchSet containing patches on
  // this level with one patch per PatchSubSet.  Eg: { {1}, {2}, {3} }
  const PatchSet* eachPatch() const;
  const PatchSet* allPatches() const;
  
  const Patch* selectPatchForCellIndex( const IntVector& idx) const;
  const Patch* selectPatchForNodeIndex( const IntVector& idx) const;
  
  // getID() returns a unique identifier so if the grid is rebuilt the new
  // levels will have different id numbers (like a  serial number).
  inline int getID() const { return d_id; }

 // getIndex() returns the relative position of the level - 0 is coarsest, 1 is next and so forth.
  inline int getIndex() const { return d_index; }

  inline IntVector getRefinementRatio() const { return d_refinementRatio; }

  int getRefinementRatioMaxDim() const;

  inline bool testFlag( LevelFlags::FlagState flag_state ) const { return d_flags.test(flag_state); };

  friend std::ostream& operator<<(std::ostream& out, const Uintah::Level& level);

private:

  // disable copy and assignment
  Level( const Level& );
  Level& operator=( const Level& );
      

  // Quick and dirty to handle indirection of perSubset, perProcessorLevelBasedPatchSets
  int d_levelSubsetPerProcPatchSetIndex;

  std::vector<Patch*> d_patches;

  Grid        * d_grid;
  Point         d_anchor;
  Vector        d_dcell;

  // The spatial range of the level.
  BBox      d_spatial_range;
  BBox      d_int_spatial_range;
  
  bool      d_finalized;
  int       d_index; // number of the level
  int       d_subsetIndex; // index within the level subset

  IntVector d_patchDistribution;
  IntVector d_periodicBoundaries;

  PatchSet    * d_each_patch;
  PatchSet    * d_all_patches;
  LevelSubset * d_subset;

  long      d_totalCells;
  IntVector d_extraCells;

  std::vector<Patch*> d_realPatches; // only real patches
  std::vector<Patch*> d_virtualAndRealPatches; // real and virtual

  int       d_id;
  IntVector d_refinementRatio;

  // vars for select_grid - don't ifdef them here, so if we change it we don't have to compile everything
  IntVector d_idxLow;
  IntVector d_idxHigh;
  IntVector d_idxSize;
  IntVector d_gridSize;

  std::vector<int>    d_gridStarts;
  std::vector<Patch*> d_gridPatches;

  // For stretched grids
  bool d_stretched;

  // for various level-related flags, e.g. isAMR, isMultiScale, etc
  LevelFlags d_flags;

  // This is three different arrays containing the x,y,z coordinate of the face position
  // be sized to d_idxSize[axis] + 1.  Used only for stretched grids
  OffsetArray1<double> d_facePosition[3];


  ///////////////////////////////////////////////////////
  // vars for select_rangetree
  class IntVectorCompare {
  public:
    bool operator() (const std::pair<IntVector, IntVector>&a, const std::pair<IntVector, IntVector>&b) const 
    {
      return (a.first < b.first) || (!(b.first < a.first) && a.second < b.second);
    }
  };

  typedef std::map<std::pair<IntVector, IntVector>, std::vector<const Patch*>, IntVectorCompare> selectCache;
  mutable selectCache   d_selectCache; // we like const Levels in most places :)
  PatchBVH*             d_bvh;
  mutable CrowdMonitor  d_cachelock;
};

const Level* getLevel( const PatchSubset* subset );
const Level* getLevel( const PatchSet* set );
const LevelP& getLevelP( const PatchSubset* subset );

} // End namespace Uintah

#endif // end #ifndef UINTAH_CORE_GRID_LEVEL_H
