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
CPPFILES= osgppu.cpp

# define used vairables
TARGET = osgppu
INCPATH += -I$(TOPDIR)/include
LIBS += -losgPPU -losgViewer
LDFLAGS = $(LIBPATH) $(LIBS)

# directory where to put result in
MAKE_DST = bin

#-----------------------------------------------
# Include rules for handling the objects
#-----------------------------------------------
include $(TOPDIR)/Make/Makerules
sinclude make.dep
