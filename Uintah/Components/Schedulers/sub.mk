#
# Makefile fragment for this subdirectory
# $Id$
#

include $(SRCTOP)/scripts/smallso_prologue.mk

SRCDIR   := Uintah/Components/Schedulers

SRCS     += $(SRCDIR)/BrainDamagedScheduler.cc \
	$(SRCDIR)/OnDemandDataWarehouse.cc

PSELIBS := Uintah/Grid Uintah/Interface SCICore/Thread Uintah/Parallel \
	Uintah/Exceptions SCICore/Exceptions
LIBS := $(XML_LIBRARY) -lmpi

include $(SRCTOP)/scripts/smallso_epilogue.mk

#
# $Log$
# Revision 1.5  2000/05/21 08:19:07  sparker
# Implement NCVariable read
# Do not fail if variable type is not known
# Added misc stuff to makefiles to remove warnings
#
# Revision 1.4  2000/05/05 06:42:43  dav
# Added some _hopefully_ good code mods as I work to get the MPI stuff to work.
#
# Revision 1.3  2000/04/12 22:59:56  sparker
# Link with xerces
#
# Revision 1.2  2000/03/20 19:38:26  sparker
# Added VPATH support
#
# Revision 1.1  2000/03/17 09:29:45  sparker
# New makefile scheme: sub.mk instead of Makefile.in
# Use XML-based files for module repository
# Plus many other changes to make these two things work
#
