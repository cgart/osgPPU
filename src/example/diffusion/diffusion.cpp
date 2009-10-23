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

//--------------------------------------------------------------------------
// Event handler to react on user input
// You can switch with some keys to specified states of the HDR pipeline
//--------------------------------------------------------------------------
class KeyboardEventHandler : public osgGA::GUIEventHandler
{
public:
    osgPPU::UnitBypassRepeat* unit;
    osg::Uniform* dt;
    osg::Uniform* intensity;

    KeyboardEventHandler()
    {
        unit = NULL;
        dt = NULL;
        intensity = NULL;
    }

    bool handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter&)
    {
        if (!unit || !dt || !intensity) return false;

        float i,t;
        intensity->get(i);
        dt->get(t);

        switch(ea.getEventType())
        {
            case(osgGA::GUIEventAdapter::KEYDOWN):
            case(osgGA::GUIEventAdapter::KEYUP):
            {
                // iteration
                if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F2)
                {
                    unit->setNumIterations(unit->getNumIterations()+1);
                }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F1)
                {
                    unit->setNumIterations(unit->getNumIterations()-1);
                }

                // intenisty
                if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F3)
                {
                    intensity->set(i-0.1f);
                }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F4)
                {
                    intensity->set(i+0.1f);
                }

                // dt
                if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F5)
                {
                    dt->set(t-0.001f);
                }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F6)
                {
                    dt->set(t+0.001f);
                }
                break;
            }
            default:
                break;
        }
        return false;
    }
};

//-----------------------------------------------------------------------------------------
// Just a callback to update the FPS counter
//-----------------------------------------------------------------------------------------
struct CameraPostDrawCallback : public osg::Camera::DrawCallback
{
    CameraPostDrawCallback(osgPPU::UnitText* unitText, KeyboardEventHandler* ke, osgViewer::Viewer* viewer) : 
        _oldTime(0), _unitText(unitText), _keyHandler(ke), _viewer(viewer)
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
            float dt, in;
            _keyHandler->dt->get(dt);
            _keyHandler->intensity->get(in);

            sprintf(txt, "Iter=%d, dT=%2.3f, I=%1.1f - FPS: %4.2f", 
                _keyHandler->unit->getNumIterations(), dt, in, 1.0 / frameTime);
            _unitText->setText(txt);
        }
    }

    mutable  float _oldTime;
    osgPPU::UnitText* _unitText;
    osgViewer::Viewer* _viewer;
    KeyboardEventHandler* _keyHandler;
};

//-----------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    // use an ArgumentParser object to manage the program arguments.
    ArgumentParser arguments(&argc, argv);

    // set up the usage document, in case we need to print out how to use this program.
    arguments.getApplicationUsage()->setDescription(arguments.getApplicationName() + " exampel of demonstraing Image Diffusion Filter with osgPPU");
    arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName());
    arguments.getApplicationUsage()->write(std::cout);

    // construct the viewer.
    osgViewer::Viewer viewer;
    unsigned int screenWidth,  windowWidth = 512;
    unsigned int screenHeight, windowHeight = 512;
    osg::GraphicsContext::getWindowingSystemInterface()->getScreenResolution(osg::GraphicsContext::ScreenIdentifier(0), screenWidth, screenHeight);
    viewer.setUpViewInWindow((screenWidth-windowWidth)/2, (screenHeight-windowHeight)/2, windowWidth, windowHeight);
    osgViewer::GraphicsWindow* window = dynamic_cast<osgViewer::GraphicsWindow*>(viewer.getCamera()->getGraphicsContext());
    if (window) window->setWindowName("Mean Curvature Diffusion Filter (naive PDE solver)");
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

    // if user request help write it out to cout.
    if (arguments.read("-h") || arguments.read("--help"))
    {
        arguments.getApplicationUsage()->write(std::cout);
        return 1;
    }

    // setup keyboard event handler
    osg::ref_ptr<KeyboardEventHandler> ke = new KeyboardEventHandler();
    viewer.addEventHandler(ke);

    // setup an osgPPU pipeline to render the results
    osgPPU::Unit* lastUnit = NULL;
    osgPPU::Processor* ppu = Diffusion::createPipeline(ke->unit, lastUnit, ke->dt, ke->intensity);

    // create a text unit, which will just print some info
    if (lastUnit)
    {
        // TEXT ppu
        osgPPU::UnitText* fpstext = new osgPPU::UnitText();
        fpstext->setName("FPSTextPPU");
        fpstext->setSize(56);
        fpstext->setText("Example ImageDiffusionFilter-pipeline");
        fpstext->setPosition(0.01, 0.95);
        lastUnit->addChild(fpstext);

        // create camera callback, so that we get update of the FPS
        viewer.getCamera()->setPostDrawCallback(new CameraPostDrawCallback(fpstext, ke, &viewer));

        // OUTPUT ppu
        osgPPU::UnitOut* ppuout = new osgPPU::UnitOut();
        ppuout->setName("PipelineResult");
        ppuout->setInputTextureIndexForViewportReference(-1); // need this here to get viewport from camera attached to processor
        ppuout->setViewport(new osg::Viewport(0,0,windowWidth, windowHeight));
        fpstext->addChild(ppuout);
    }
    
    printf("Keys:\n");
    printf("\tF1 - Decrease iteration number\n");
    printf("\tF2 - Increase number of iterations\n");
    printf("\tF3 - Decrease intensity\n");
    printf("\tF4 - Increase intensity\n");
    printf("\tF5 - Decrease dT intensity\n");
    printf("\tF6 - Increase dT intensity\n");
    //printf("\tF12 - toggle ppu On/Off\n");

    osg::Group* root = new osg::Group;
    //osg::Node* loadedModel = createTeapot();
    //root->addChild(loadedModel);
    //osg::Switch* swtich = new osg::Switch();
    //swtich->addChild(ppu);
    //swtich->setAllChildrenOff();
    //root->addChild(swtich);

    root->addChild(ppu);

    viewer.setSceneData(root);

    return viewer.run();
}
