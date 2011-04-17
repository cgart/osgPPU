How to use examples
===============================

ATTENTION: 
Examples do load .ppu and .glsl files out of the Data directory, which can
be found in the main direcotry of osgPPU. In order that this files are found
you have to setup the OSG_FILE_PATH environment variable accordingly.
Hence on Unix systems do following:
  export OSG_FILE_PATH=$OSG_FILE_PATH:/path/were/osgPPU/is/installed/

Other solution would be just to copy the Data/ directory under your bin/
directory to let examnple run


The Data/ irectory does contain some examples and tools coming together
witht the osgPPU package. Use them to see how all the things are setted up.
Use the viewer to see the .ppu files which you can find in the bin/Data/
directory.
For example:
  cd bin
  ./viewer Data/motionblur.ppu 
will show you how the motionblur effect can be realized with the osgPPU.
The corresponding .ppu file does contain a main setup for the correct
effect pipeline.




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
    cmake ../ -DCMAKE_BUILD_TYPE=Release
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




How to build osg plugin osgdb_ppu
===============================
The plugin is build together with the complete library.
However to install the library file in correct plugin
directory you have to specify the version of osg with:

    cmake . -DOSG_VERSION=2.5.x

The consecutive call of "make install" would install
the ppu plugin under PREFIX/lib/osgPlugins-2.5.x/.
If no version was specified then the installation path
is PREFIX/lib/osgPlugins-2.4.0/


   

