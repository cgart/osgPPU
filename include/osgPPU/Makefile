TOPDIR= ../..

#-----------------------------------------------
# Include defs for defining the variables
#-----------------------------------------------
include $(TOPDIR)/Make/Makedefs

#-----------------------------------------------
# We have to built this files into the library
#-----------------------------------------------
INCFILES=   PostProcess.h \
		PostProcessUnit.h \
		PPUInOut.h
		
# define files for installation
INSTALL_DST_INC = $(INST_LOCATION_INC)/osgPPU
INSTALL_SRC_INC = $(INCFILES)
UNINSTALL_DIR = $(INST_LOCATION_INC)/osgPPU

#-----------------------------------------------
# Include rules for handling the objects
#-----------------------------------------------
include $(TOPDIR)/Make/Makerules
