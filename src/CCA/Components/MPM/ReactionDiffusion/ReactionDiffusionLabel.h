/*
 * The MIT License
 *
 * Copyright (c) 1997-2014 The University of Utah
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

#ifndef UINTAH_REACTIONDIFFUSIONLABEL_H
#define UINTAH_REACTIONDIFFUSIONLABEL_H

namespace Uintah {

  class VarLabel;
  
  class ReactionDiffusionLabel {
  public:
    
    ReactionDiffusionLabel();
    ~ReactionDiffusionLabel();

    // Particle Variables
    const VarLabel* pConcentrationLabel;
    const VarLabel* pConcPreviousLabel;
    const VarLabel* pdCdtLabel;
    const VarLabel* pConcGradientLabel;
    const VarLabel* pFluxLabel;
    const VarLabel* pHydroStressLabel;

    const VarLabel* pConcentrationLabel_preReloc;
    const VarLabel* pConcPreviousLabel_preReloc;
    const VarLabel* pdCdtLabel_preReloc;

    // Grid Variables
    const VarLabel* gConcentrationLabel;
    const VarLabel* gConcentrationCCLabel;
    const VarLabel* gConcentrationRateLabel;
    const VarLabel* gConcentrationNoBCLabel;
    const VarLabel* gdCdtLabel;
    const VarLabel* gConcentrationStarLabel;
    const VarLabel* gHydrostaticStressLabel;

    // Reduction variables
    const VarLabel* maxHydroStressLabel0;
    const VarLabel* maxHydroStressLabel1;
    const VarLabel* maxHydroStressLabel2;
    const VarLabel* maxHydroStressLabel3;
    const VarLabel* minHydroStressLabel0;
    const VarLabel* minHydroStressLabel1;
    const VarLabel* minHydroStressLabel2;
    const VarLabel* minHydroStressLabel3;
  };
  
} // end namespace Uintah
#endif