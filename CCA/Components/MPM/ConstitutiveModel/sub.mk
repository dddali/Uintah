# Makefile fragment for this subdirectory

SRCDIR   := Packages/Uintah/CCA/Components/MPM/ConstitutiveModel

SRCS     += $(SRCDIR)/CompMooneyRivlin.cc $(SRCDIR)/CompNeoHook.cc \
	$(SRCDIR)/CompNeoHookPlas.cc $(SRCDIR)/ConstitutiveModelFactory.cc \
	$(SRCDIR)/ConstitutiveModel.cc \
	$(SRCDIR)/MPMMaterial.cc $(SRCDIR)/ViscoScram.cc \
	$(SRCDIR)/HypoElastic.cc 

PSELIBS := Packages/Uintah/Core/Grid

