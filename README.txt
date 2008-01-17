How to build the osgPPU
===============================

The osgPPU uses the CMake build system to generate a 
platform-specific build environment. This build system 
was choosen since OpenSceneGraph is also based on it.

If you don't already have CMake installed on your system you can grab 
it from http://www.cmake.org, use version 2.4.6 or later.  Details on the 
OpenSceneGraph's CMake build can be found at:

    http://www.openscenegraph.org/projects/osg/wiki/Build/CMake

Under unices (i.e. Linux, IRIX, Solaris, Free-BSD, HP-Ux, AIX, OSX) 
use the cmake or ccmake command-line utils. To compile osgPPU type following:

    cd osgPPU
    cmake . -DCMAKE_BUILD_TYPE=Release
    make
    sudo make install
  
Alternatively, you can create an out-of-source build directory and run 
cmake or ccmake from there. The advantage to this approach is that the 
temporary files created by CMake won't clutter the osgPPU
source directory, and also makes it possible to have multiple 
independent build targets by creating multiple build directories. In a 
directory alongside the OpenSceneGraph use:

    mkdir build
    cd build
    cmake ../osgPPU -DCMAKE_BUILD_TYPE=Release
    make
    sudo make install

If you would like to install the library somehwere else than the default
install location, so type in the main directory of osgPPU the following:

    cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/path/to/install 
    make install

Under Windows use the GUI tool CMakeSetup to build your VisualStudio 
files. The following page on OpenSceneGraph's wiki dedicated to the CMake build 
system should help guide you through the process:

    http://www.openscenegraph.org/projects/osg/wiki/Support/PlatformSpecifics/VisualStudio


How to build and run examples
===============================

The examples are compiled together with the library.
The executable is then copied to the bin directory.
All the data, which are used by the examples can be
found also under bin/Data

To run the examples go into the bin directory and run it:
    cd bin
    ./hdr 


