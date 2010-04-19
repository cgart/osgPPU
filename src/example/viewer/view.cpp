#include <osg/GLExtensions>
#include <osgViewer/Renderer>
#include <osgGA/TrackballManipulator>
#include <osgDB/WriteFile>

#include <osgPPU/Processor.h>
#include <osgPPU/UnitBypass.h>
#include <osgPPU/UnitInResampleOut.h>
#include <osgPPU/UnitOut.h>

#include "osgteapot.h"

//--------------------------------------------------------------------------
// Create camera resulting texture
//--------------------------------------------------------------------------
osg::Texture* createRenderTexture(int tex_width, int tex_height, bool depth)
{
    // create simple 2D texture
    osg::Texture2D* texture2D = new osg::Texture2D;
    texture2D->setTextureSize(tex_width, tex_height);
    texture2D->setResizeNonPowerOfTwoHint(false);
    texture2D->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
    texture2D->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);
    texture2D->setWrap(osg::Texture2D::WRAP_S,osg::Texture2D::CLAMP_TO_BORDER);
    texture2D->setWrap(osg::Texture2D::WRAP_T,osg::Texture2D::CLAMP_TO_BORDER);
    texture2D->setBorderColor(osg::Vec4(1.0f,1.0f,1.0f,1.0f));

    // setup float format
    if (!depth)
    {
        texture2D->setInternalFormat(GL_RGBA16F_ARB);
        texture2D->setSourceFormat(GL_RGBA);
        texture2D->setSourceType(GL_FLOAT);
    }else{
        texture2D->setInternalFormat(GL_DEPTH_COMPONENT);
    }

    return texture2D;
}

//--------------------------------------------------------------------------
// Setup the camera 
//--------------------------------------------------------------------------
void setupCamera(osg::Camera* camera, osg::Viewport* viewport = NULL)
{
	osg::Viewport* vp = viewport == NULL ? camera->getViewport() : viewport;

    // create texture to render to
    osg::Texture* texture = createRenderTexture((int)vp->width(), (int)vp->height(), false);
    osg::Texture* depthTexture = createRenderTexture((int)vp->width(), (int)vp->height(), true);

    // set up the background color and clear mask.
    camera->setClearColor(osg::Vec4(0.0f,0.0f,0.0f,0.0f));
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // set viewport
    camera->setViewport(vp);
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    camera->setProjectionMatrixAsPerspective(35.0, vp->width()/vp->height(), 0.001, 100.0);

    // tell the camera to use OpenGL frame buffer object where supported.
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

    // attach the texture and use it as the color buffer.
    camera->attach(osg::Camera::COLOR_BUFFER, texture);//, 0, 0, false, 8, 8);
    camera->attach(osg::Camera::DEPTH_BUFFER, depthTexture);
}

//--------------------------------------------------------------------------
// Event handler to react on resize events
//--------------------------------------------------------------------------
class ResizeEventHandler : public osgGA::GUIEventHandler
{
public:
    osgPPU::Processor* _processor;
    osg::Camera* _camera;
    
    ResizeEventHandler(osgPPU::Processor* proc, osg::Camera* cam) : _processor(proc), _camera(cam) {}
    
    bool handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter&)
    {
        if(ea.getEventType() == osgGA::GUIEventAdapter::RESIZE)            
        {
			int width = ea.getWindowWidth();
			int height = ea.getWindowHeight();

            // resetup camera with new viewport, this is neccessary to reset camera's RTT texture to proper size
            osg::ref_ptr<osg::Viewport> vp = new osg::Viewport(0, 0, width, height);
			setupCamera(_camera, vp);

			// inform renderer that there was a resize event and that it has to update the main camera's FBO
			osgViewer::Renderer* renderer = (osgViewer::Renderer*)_camera->getRenderer();
			renderer->getSceneView(0)->getRenderStage()->setCameraRequiresSetUp(true);
			renderer->getSceneView(0)->getRenderStage()->setFrameBufferObject(NULL);

			// let processor know, that viewport changes, processor will inform all units
			_processor->onViewportChange();
			_processor->dirtyUnitSubgraph();
        }
        return false;
    }
};


//--------------------------------------------------------------------------
int main(int argc, char **argv)
{
    // parse arguments
    osg::ArgumentParser arguments(&argc,argv);

    // give some info in the console
    printf("Usage: viewer ppufile [osgfile]\n");

    if (argc <= 1) return 0;

    // construct the viewer.
    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer();

    // just make it singlethreaded since I get some problems if not in this mode
    unsigned int screenWidth;
    unsigned int screenHeight;
    osg::GraphicsContext::getWindowingSystemInterface()->getScreenResolution(osg::GraphicsContext::ScreenIdentifier(0), screenWidth, screenHeight);
    unsigned int windowWidth = 640;
    unsigned int windowHeight = 480;
    viewer->setUpViewInWindow((screenWidth-windowWidth)/2, (screenHeight-windowHeight)/2, windowWidth, windowHeight);
    osgViewer::GraphicsWindow* window = dynamic_cast<osgViewer::GraphicsWindow*>(viewer->getCamera()->getGraphicsContext());
    if (window) window->setWindowName(".ppu file viewer");
    //viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);

    // setup scene
    osg::Group* node = new osg::Group();
    osg::Node* loadedModel = NULL;
    if (argc > 2) loadedModel = osgDB::readNodeFile(arguments[2]);
    if (argc > 2 && !loadedModel)
    {
        printf("File not found %s !\n", arguments[2]);
        return 1;
    }
    if (!loadedModel) loadedModel = createTeapot();
    node->addChild(loadedModel);

    // disable color clamping, because we want to work on real hdr values
    osg::ClampColor* clamp = new osg::ClampColor();
    clamp->setClampVertexColor(GL_FALSE);
    clamp->setClampFragmentColor(GL_FALSE);
    clamp->setClampReadColor(GL_FALSE);

    // make it protected and override, so that it is done for the whole rendering pipeline
    node->getOrCreateStateSet()->setAttribute(clamp, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE | osg::StateAttribute::PROTECTED);

    // load the processor from a file
    osgPPU::Processor* processor = dynamic_cast<osgPPU::Processor*>(osgDB::readObjectFile(arguments[1]));
    if (!processor)
    {
        osg::notify(osg::FATAL) << "File does not contain a valid pipeline" << std::endl;
        return 0;
    }

    // setup viewers camera
    setupCamera(viewer->getCamera());
    processor->setCamera(viewer->getCamera());

    // add processor to the scene
    node->addChild(processor);

    // add model to viewer.
    viewer->addEventHandler(new ResizeEventHandler(processor, viewer->getCamera()));
    viewer->setSceneData( node );

    // run viewer
    return viewer->run();
}
