#ifndef __VISCOSCRAM_CONSTITUTIVE_MODEL_H__
#define __VISCOSCRAM_CONSTITUTIVE_MODEL_H__

#include "ConstitutiveModel.h"
#include <Packages/Uintah/Core/Math/Matrix3.h>
#include <Packages/Uintah/Core/Disclosure/TypeDescription.h>
#include <Packages/Uintah/Core/ProblemSpec/ProblemSpec.h>

#include <math.h>
#include <sgi_stl_warnings_off.h>
#include <vector>
#include <sgi_stl_warnings_on.h>

namespace Uintah {

  class MPMLabel;
  class MPMFlags;

  /////////////////////////////////////////////////////////////////////////////
  /*!
    \class ViscoScram
    \brief Light version of ViscoSCRAM
    \author Scott Bardenhagen \n
    C-SAFE and Department of Mechanical Engineering \n
    University of Utah \n
    Copyright (C) 2000 University of Utah
  */
  /////////////////////////////////////////////////////////////////////////////

  class ViscoScram : public ConstitutiveModel {

  public:
    struct CMData {
      double PR;
      double CoefThermExp;
      double CrackParameterA;
      double CrackPowerValue;
      double CrackMaxGrowthRate;
      double StressIntensityF;
      double CrackFriction;
      double InitialCrackRadius;
      double CrackGrowthRate;
      double G[5];
      double RTau[5];
      double Beta, Gamma;
      double DCp_DTemperature;
      int LoadCurveNumber, NumberOfPoints;
    };

    struct StateData {
      Matrix3 DevStress[5];
    };

    const VarLabel* pVolChangeHeatRateLabel;
    const VarLabel* pViscousHeatRateLabel;
    const VarLabel* pCrackHeatRateLabel;
    const VarLabel* pCrackRadiusLabel;
    const VarLabel* pStatedataLabel;
    const VarLabel* pRandLabel;

    const VarLabel* pVolChangeHeatRateLabel_preReloc;
    const VarLabel* pViscousHeatRateLabel_preReloc;
    const VarLabel* pCrackHeatRateLabel_preReloc;
    const VarLabel* pCrackRadiusLabel_preReloc;
    const VarLabel* pStatedataLabel_preReloc;
    const VarLabel* pRandLabel_preReloc;

  protected:

    friend const Uintah::TypeDescription* 
       fun_getTypeDescription(ViscoScram::StateData*);

    // Create datatype for storing model parameters
    bool d_useModifiedEOS;
    double d_bulk;

    CMData d_initialData;

  private:

    // Prevent assignment of this class
    ViscoScram& operator=(const ViscoScram &cm);

  public:

    // constructors
    ViscoScram(ProblemSpecP& ps, MPMLabel* lb, MPMFlags* flag);
    ViscoScram(const ViscoScram* cm);
       
    // destructor
    virtual ~ViscoScram();

    // compute stable timestep for this patch
    virtual void computeStableTimestep(const Patch* patch,
                                       const MPMMaterial* matl,
                                       DataWarehouse* new_dw);

    // compute stress at each particle in the patch
    virtual void computeStressTensor(const PatchSubset* patches,
                                     const MPMMaterial* matl,
                                     DataWarehouse* old_dw,
                                     DataWarehouse* new_dw);

    virtual void computeStressTensor(const PatchSubset* ,
                                     const MPMMaterial* ,
                                     DataWarehouse* ,
                                     DataWarehouse* ,
#ifdef HAVE_PETSC
                                     MPMPetscSolver* ,
#else
                                     SimpleSolver* ,
#endif
                                     const bool )
    {
    }

    // carry forward CM data for RigidMPM
    virtual void carryForward(const PatchSubset* patches,
                              const MPMMaterial* matl,
                              DataWarehouse* old_dw,
                              DataWarehouse* new_dw);

    virtual void addInitialComputesAndRequires(Task* task,
                                               const MPMMaterial* matl,
                                               const PatchSet* patches) const;

    // initialize  each particle's constitutive model data
    virtual void initializeCMData(const Patch* patch,
                                  const MPMMaterial* matl,
                                  DataWarehouse* new_dw);

    virtual void addComputesAndRequires(Task* task,
                                        const MPMMaterial* matl,
                                        const PatchSet* patches) const;

    virtual void addComputesAndRequires(Task* task,
                                        const MPMMaterial* matl,
                                        const PatchSet* patches,
                                        const bool recursion) const;

    virtual void allocateCMDataAddRequires(Task* task, const MPMMaterial* matl,
                                           const PatchSet* patch, 
                                           MPMLabel* lb) const;

    virtual void allocateCMDataAdd(DataWarehouse* new_dw,
                                   ParticleSubset* subset,
        map<const VarLabel*, ParticleVariableBase*>* newState,
                                   ParticleSubset* delset,
                                   DataWarehouse* old_dw);

    virtual void addParticleState(std::vector<const VarLabel*>& from,
                                  std::vector<const VarLabel*>& to);


    virtual double computeRhoMicroCM(double pressure,
                                     const double p_ref,
                                     const MPMMaterial* matl);

    virtual void computePressEOSCM(double rho_m, double& press_eos,
                                   double p_ref,
                                   double& dp_drho, double& ss_new,
                                   const MPMMaterial* matl);

    virtual double getCompressibility();

  };

  const Uintah::TypeDescription* fun_getTypeDescription(ViscoScram::StateData*);

} // End namespace Uintah
      
namespace SCIRun {
  void swapbytes( Uintah::ViscoScram::StateData& d);
} // namespace SCIRun

#endif  // __VISCOSCRAM_CONSTITUTIVE_MODEL_H__ 

