# 
# 
# The MIT License
# 
# Copyright (c) 1997-2010 Center for the Simulation of Accidental Fires and 
# Explosions (CSAFE), and  Scientific Computing and Imaging Institute (SCI), 
# University of Utah.
# 
# License for the specific language governing rights and limitations under
# Permission is hereby granted, free of charge, to any person obtaining a 
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation 
# the rights to use, copy, modify, merge, publish, distribute, sublicense, 
# and/or sell copies of the Software, and to permit persons to whom the 
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included 
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
# DEALINGS IN THE SOFTWARE.
# 
# 
# 
# 
# Makefile fragment for this subdirectory


SRCDIR   := CCA/Components/Solvers/AMR


ifeq ($(HAVE_HYPRE),yes)

  SRCS += $(SRCDIR)/AMRSolver.cc \
	$(SRCDIR)/HypreDriver.cc \
	$(SRCDIR)/HypreDriverStruct.cc \
	$(SRCDIR)/HypreDriverSStruct.cc

  SUBDIRS := \
	$(SRCDIR)/HypreSolvers \
	$(SRCDIR)/HyprePreconds

  include $(SCIRUN_SCRIPTS)/recurse.mk

endif # if $(HAVE_HYPRE)

PSELIBS := \
	Core/Containers                  \
	Core/Exceptions                  \
	Core/Geometry                    \
	Core/Thread                      \
	Core/Util                        \
	Core/Geometry                    \
	CCA/Ports        \
	Core/Exceptions  \
	Core/Grid        \
	Core/Util        \
	Core/Disclosure  \
	Core/Parallel    \
	Core/ProblemSpec \
	\
	Core/Math

LIBS := $(XML2_LIBRARY) $(MPI_LIBRARY) $(BLAS_LIBRARY) $(LAPACK_LIBRARY)

ifeq ($(HAVE_HYPRE),yes)
  LIBS := $(LIBS) $(HYPRE_LIBRARY) 
endif
