#
#  The MIT License
#
#  Copyright (c) 1997-2012 The University of Utah
# 
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to
#  deal in the Software without restriction, including without limitation the
#  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
#  sell copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
# 
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
#  IN THE SOFTWARE.
# 
# 
# 
# 
# 
# Makefile fragment for this subdirectory 


include $(SCIRUN_SCRIPTS)/smallso_prologue.mk

SRCDIR   := CCA/Components/Arches/fortran

SRCS += \
        $(SRCDIR)/apcal_all.F \
        $(SRCDIR)/arrass.F \
        $(SRCDIR)/cellg.F \
        $(SRCDIR)/fixval.F \
        $(SRCDIR)/fixval_trans.F \
        $(SRCDIR)/mm_computevel.F\
        $(SRCDIR)/mm_explicit.F\
        $(SRCDIR)/mm_explicit_oldvalue.F\
        $(SRCDIR)/mm_explicit_vel.F\
        $(SRCDIR)/mmbcvelocity.F \
        $(SRCDIR)/prescoef_var.F \
        $(SRCDIR)/pressrcpred.F \
        $(SRCDIR)/pressrcpred_var.F \
        $(SRCDIR)/uvelcoef.F \
        $(SRCDIR)/uvelsrc.F \
        $(SRCDIR)/vvelcoef.F \
        $(SRCDIR)/vvelsrc.F \
        $(SRCDIR)/wallbc.F \
        $(SRCDIR)/wvelcoef.F \
        $(SRCDIR)/wvelsrc.F 

PSELIBS := 

LIBS := $(F_LIBRARY) $(M_LIBRARY)

include $(SCIRUN_SCRIPTS)/smallso_epilogue.mk

$(SRCDIR)/apcal_all.$(OBJEXT): $(SRCDIR)/apcal_all_fort.h
$(SRCDIR)/arrass.$(OBJEXT): $(SRCDIR)/arrass_fort.h
$(SRCDIR)/bcpress.$(OBJEXT): $(SRCDIR)/bcpress_fort.h
$(SRCDIR)/bcuvel.$(OBJEXT): $(SRCDIR)/bcuvel_fort.h
$(SRCDIR)/bcvvel.$(OBJEXT): $(SRCDIR)/bcvvel_fort.h
$(SRCDIR)/bcwvel.$(OBJEXT): $(SRCDIR)/bcwvel_fort.h
$(SRCDIR)/cellg.$(OBJEXT): $(SRCDIR)/cellg_fort.h
$(SRCDIR)/mm_computevel.$(OBJEXT): $(SRCDIR)/mm_computevel_fort.h
$(SRCDIR)/mm_explicit.$(OBJEXT): $(SRCDIR)/mm_explicit_fort.h
$(SRCDIR)/mm_explicit_oldvalue.$(OBJEXT): $(SRCDIR)/mm_explicit_oldvalue_fort.h
$(SRCDIR)/mm_explicit_vel.$(OBJEXT): $(SRCDIR)/mm_explicit_vel_fort.h
$(SRCDIR)/mmbcvelocity.$(OBJEXT): $(SRCDIR)/mmbcvelocity_fort.h
$(SRCDIR)/prescoef_var.$(OBJEXT): $(SRCDIR)/prescoef_var_fort.h
$(SRCDIR)/pressrcpred.$(OBJEXT): $(SRCDIR)/pressrcpred_fort.h
$(SRCDIR)/pressrcpred_var.$(OBJEXT): $(SRCDIR)/pressrcpred_var_fort.h
$(SRCDIR)/uvelcoef.$(OBJEXT): $(SRCDIR)/uvelcoef_fort.h
$(SRCDIR)/uvelsrc.$(OBJEXT): $(SRCDIR)/uvelsrc_fort.h
$(SRCDIR)/vvelcoef.$(OBJEXT): $(SRCDIR)/vvelcoef_fort.h
$(SRCDIR)/vvelsrc.$(OBJEXT): $(SRCDIR)/vvelsrc_fort.h
$(SRCDIR)/wallbc.$(OBJEXT): $(SRCDIR)/wallbc_fort.h
$(SRCDIR)/wvelcoef.$(OBJEXT): $(SRCDIR)/wvelcoef_fort.h
$(SRCDIR)/wvelsrc.$(OBJEXT): $(SRCDIR)/wvelsrc_fort.h

