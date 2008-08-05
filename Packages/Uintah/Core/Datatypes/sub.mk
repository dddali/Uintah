# Makefile fragment for this subdirectory

include $(SCIRUN_SCRIPTS)/smallso_prologue.mk

SRCDIR := Packages/Uintah/Core/Datatypes

SRCS     += \
	$(SRCDIR)/Archive.cc \
	$(SRCDIR)/ScalarParticles.cc \
	$(SRCDIR)/VectorParticles.cc \
	$(SRCDIR)/TensorParticles.cc \
	$(SRCDIR)/PSet.cc \
	$(SRCDIR)/GLAnimatedStreams.cc \
	$(SRCDIR)/VariableCache.cc \
#	$(SRCDIR)/cd_templates.cc \


DATAFLOW_LIBS :=
ifeq ($BUILD_DATAFLOW,yes)
  DATAFLOW_LIBS := Dataflow/Network Core/Geom
endif

PSELIBS := \
	$(DATAFLOW_LIBS) \
	Core/Exceptions  \
	Core/Geometry    \
	Core/Persistent  \
	Core/Datatypes   \
	Core/Containers  \
	Core/Thread      \
	Core/Util        \
	Packages/Uintah/Core/Grid        \
	Packages/Uintah/Core/Parallel    \
	Packages/Uintah/Core/Util        \
	Packages/Uintah/Core/Math        \
	Packages/Uintah/Core/Disclosure  \
	Packages/Uintah/Core/ProblemSpec \
	Packages/Uintah/CCA/Ports        \
        Packages/Uintah/Core/Exceptions  


LIBS := $(XML2_LIBRARY) $(MPI_LIBRARY) $(GL_LIBRARY) $(M_LIBRARY) 

include $(SCIRUN_SCRIPTS)/smallso_epilogue.mk


