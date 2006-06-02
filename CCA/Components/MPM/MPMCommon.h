#ifndef UINTAH_HOMEBREW_MPM_COMMON_H
#define UINTAH_HOMEBREW_MPM_COMMON_H


#include <Packages/Uintah/CCA/Components/MPM/MPMFlags.h>
#include <Packages/Uintah/Core/ProblemSpec/ProblemSpecP.h>
#include <Packages/Uintah/Core/Grid/SimulationStateP.h>

#include <Packages/Uintah/CCA/Components/MPM/share.h>
namespace Uintah {

  using namespace SCIRun;
  
  class SCISHARE MPMCommon {

  public:
    MPMCommon();
    virtual ~MPMCommon();

    virtual void materialProblemSetup(const ProblemSpecP& prob_spec,
                                      SimulationStateP& sharedState,
                                      MPMFlags* flags);
  };
}

#endif
