TOPDIR= ../..

#-----------------------------------------------
# Include defs for defining the variables
#-----------------------------------------------
include $(TOPDIR)/Make/Makedefs

#-----------------------------------------------
# Create vpath directives
#-----------------------------------------------
#VPATH+= events ../include

#-----------------------------------------------
# We have to built this files into the library
#-----------------------------------------------
CPPFILES= PostProcess.cpp \
	  PostProcessUnit.cpp \
	  PPUInOut.cpp \
	  Shader.cpp

# define used vairables
TARGET = libosgPPU.so
INCPATH += -I$(TOPDIR)/include
CFLAGS += -fPIC
LIBS += -losg -losgUtil -losgDB

# define files for installation
INSTALL_DST_LIB = $(INST_LOCATION_LIB)
INSTALL_SRC_LIB = $(TARGET)

#-----------------------------------------------
# Include rules for handling the objects
#-----------------------------------------------
include $(TOPDIR)/Make/Makerules
sinclude make.dep
