/* OpenSceneGraph example, osgprerendercubemap.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*  THE SOFTWARE.
*/

#include <osgViewer/Viewer>

#include <osg/Projection>
#include <osg/Geometry>
#include <osg/Texture>
#include <osg/TexGen>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/PolygonOffset>
#include <osg/CullFace>
#include <osg/TextureCubeMap>
#include <osg/TexMat>
#include <osg/MatrixTransform>
#include <osg/Light>
#include <osg/LightSource>
#include <osg/PolygonOffset>
#include <osg/CullFace>
#include <osg/Material>
#include <osg/PositionAttitudeTransform>
#include <osg/ArgumentParser>
#include <osg/Program>
#include <osg/Shader>
#include <osg/Texture2D>
#include <osg/Camera>
#include <osg/TexGenNode>

#include <osgDB/ReadFile>

#include "simple.h"
#include "osgteapot.h"
#include <osgPPU/UnitText.h>

#include <iostream>

#define NEAR_PLANE 0.01f
#define FAR_PLANE 50.0f
#define FOVY 30.0

using namespace osg;


//-----------------------------------------------------------------------------------------
// Just a callback to update the FPS counter
//-----------------------------------------------------------------------------------------
struct CameraPostDrawCallback : public osg::Camera::DrawCallback
{
    CameraPostDrawCallback(osgPPU::UnitText* unitText, osgViewer::Viewer* viewer) : 
        _oldTime(0), _unitText(unitText), _viewer(viewer)
    {
    }
    
    void operator () (const osg::Camera& /*camera*/) const
    {
        // get current time
        float elapsed = _viewer->elapsedTime();

        // compute frame time
        float frameTime = elapsed - _oldTime;
        _oldTime = elapsed;

        // print also some info about the fps number
        if (_unitText)
        {
            char txt[64];
            sprintf(txt, "FPS: %4.2f", 1.0 / frameTime);
            _unitText->setText(txt);
        }
    }

    mutable  float _oldTime;
    osgPPU::UnitText* _unitText;
    osgViewer::Viewer* _viewer;
};


//-----------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    // use an ArgumentParser object to manage the program arguments.
    ArgumentParser arguments(&argc, argv);

    // set up the usage document, in case we need to print out how to use this program.
    arguments.getApplicationUsage()->setDescription(arguments.getApplicationName() + " exampel of demonstraing Screen-Space Ambient Occlusion within osgPPU");
    arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName());
    arguments.getApplicationUsage()->addCommandLineOption("--width","Set the width of the render to texture (default windowWidth)");
    arguments.getApplicationUsage()->addCommandLineOption("--height","Set the height of the render to texture (default windowHeight)");
    arguments.getApplicationUsage()->addCommandLineOption("--simple","Use very simple algorithm to create SSAO effect (is used per default)");
    arguments.getApplicationUsage()->addCommandLineOption("--aomap","Show only the Ambient Occlusion Map");
    arguments.getApplicationUsage()->write(std::cout);

    // construct the viewer.
    osgViewer::Viewer viewer;
    unsigned int screenWidth;
    unsigned int screenHeight;
    osg::GraphicsContext::getWindowingSystemInterface()->getScreenResolution(osg::GraphicsContext::ScreenIdentifier(0), screenWidth, screenHeight);
    unsigned int windowWidth = 640;
    unsigned int windowHeight = 480;
    viewer.setUpViewInWindow((screenWidth-windowWidth)/2, (screenHeight-windowHeight)/2, windowWidth, windowHeight);
    //viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

    // if user request help write it out to cout.
    if (arguments.read("-h") || arguments.read("--help"))
    {
        arguments.getApplicationUsage()->write(std::cout);
        return 1;
    }

    // check if we want to see the AO-map
    bool showAOMap = false;
    if (arguments.read("--aomap"))
        showAOMap = true;
    
    // get parameters
    unsigned tex_width = windowWidth;
    unsigned tex_height = windowHeight;
    while (arguments.read("--width", tex_width)) {}
    while (arguments.read("--height", tex_height)) {}
    arguments.reportRemainingOptionsAsUnrecognized();
    if (arguments.errors())
    {
      arguments.writeErrorMessages(std::cout);
      return 1;
    }

    // setup scene
    osg::Group* root= new osg::Group;
    osg::Node* loadedModel = osgDB::readNodeFiles(arguments);
    if (!loadedModel) loadedModel = osgDB::readNodeFile("Data/temple.ive");
    if (!loadedModel)
    {
        printf("File not found: Data/temple.ive !\n");
        return 1;
    }

    // rotate camera to look onto the model
    osg::MatrixTransform* rotation = new osg::MatrixTransform;
    rotation->setMatrix(osg::Matrix::rotate(osg::DegreesToRadians(15.0),0.0,1.0,0.0)
                        * osg::Matrix::rotate(osg::DegreesToRadians(65.0),0.0,0.0,-1.0));
    rotation->addChild(loadedModel);
    root->addChild(rotation);

    // setup an osgPPU pipeline to render the results
    osgPPU::Unit* lastUnit = NULL;
    osgPPU::Processor* ppu = SimpleSSAO::createPipeline(tex_width, tex_height, viewer.getCamera(), lastUnit, showAOMap);

    // create a text unit, which will just print some info
    if (lastUnit)
    {
        // TEXT ppu
        osgPPU::UnitText* fpstext = new osgPPU::UnitText();
        fpstext->setName("FPSTextPPU");
        fpstext->setSize(56);
        fpstext->setText("Example SSAO-pipeline");
        fpstext->setPosition(0.01, 0.95);
        lastUnit->addChild(fpstext);

        // create camera callback, so that we get update of the FPS
        viewer.getCamera()->setPostDrawCallback(new CameraPostDrawCallback(fpstext, &viewer));
        //viewer.getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);  
        //viewer.getCamera()->setProjectionMatrixAsPerspective(40.0, float(windowWidth)/float(windowHeight), 0.1, 100.0);

        // OUTPUT ppu
        osgPPU::UnitOut* ppuout = new osgPPU::UnitOut();
        ppuout->setName("PipelineResult");
        ppuout->setInputTextureIndexForViewportReference(-1); // need this here to get viewport from camera
        ppuout->setViewport(new osg::Viewport(0,0,windowWidth, windowHeight));
        lastUnit->addChild(ppuout);
    }
    
    // add ppu into the scene
    root->addChild(ppu);
    viewer.setSceneData(root);

    return viewer.run();
}
