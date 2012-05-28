/*
 
 The MIT License
 
 Copyright (c) 1997-2012 Center for the Simulation of Accidental Fires and 
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


#ifndef UINTAH_HOMEBREW_RateModel_H
#define UINTAH_HOMEBREW_RateModel_H

#include <Core/Parallel/UintahParallelPort.h>
#include <Core/Grid/Variables/ComputeSet.h>
#include <Core/Grid/GridP.h>
#include <Core/Grid/LevelP.h>
#include <Core/Labels/MPMLabel.h>
#include <Core/Grid/SimulationStateP.h>
#include <Core/Util/Handle.h>
#include <Core/ProblemSpec/ProblemSpecP.h>
#include <CCA/Ports/SchedulerP.h>
#include <CCA/Ports/Output.h>

#include <Core/Grid/Variables/CCVariable.h>


namespace Uintah {
    /**************************************
     
     CLASS
     RateModel
     
     Short description...
     
     GENERAL INFORMATION
     
     RateModel.h
     
     Joseph R. Peterson
     Department of Chemistry
     University of Utah
     
     Center for the Model of Accidental Fires and Explosions (C-SAFE)
     
     Copyright (C) 2012 SCI Group
     
     KEYWORDS
     RateModel
     
     DESCRIPTION
     Long description...
     
     WARNING
     
     ****************************************/
    class RateModel {
    public:
        virtual void outputProblemSpec(ProblemSpecP& ps) = 0;

        /// @brief Get the contribution to the rate from the fraction reacted
        /// @param fractionReactant The fraction in the volume that is reactant, i.e. m_r/(m_r+m_p)
        /// @return a scalar for the extent of reaction
        virtual double getDifferentialFractionChange(double fractionReactant) = 0;
     
        // This quiets compiler warnings
        virtual ~RateModel()
        {
        }
   
    private:
        
    };
    
} // End namespace Uintah



#endif
