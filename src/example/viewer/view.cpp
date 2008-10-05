#include <osg/GLExtensions>
#include <osgViewer/Renderer>
#include <osgGA/TrackballManipulator>
#include <osgDB/WriteFile>

#include <osgPPU/Processor.h>

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
// Setup the camera to do the render to texture
//--------------------------------------------------------------------------
void setupCamera(osg::Camera* camera)
{
    osg::Viewport* vp = camera->getViewport();

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
    camera->attach(osg::Camera::COLOR_BUFFER, texture);
    camera->attach(osg::Camera::DEPTH_BUFFER, depthTexture);
}

//--------------------------------------------------------------------------
int main(int argc, char **argv)
{
    // parse arguments
    osg::ArgumentParser arguments(&argc,argv);

    // give some info in the console
    printf("view ppufile [osgfile]\n");

    if (argc <= 1) return 0;

    // construct the viewer.
    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer();

    // just make it singlethreaded since I get some problems if not in this mode
    //viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    unsigned int screenWidth;
    unsigned int screenHeight;
    osg::GraphicsContext::getWindowingSystemInterface()->getScreenResolution(osg::GraphicsContext::ScreenIdentifier(0), screenWidth, screenHeight);
    unsigned int windowWidth = 640;
    unsigned int windowHeight = 480;
    viewer->setUpViewInWindow((screenWidth-windowWidth)/2, (screenHeight-windowHeight)/2, windowWidth, windowHeight);

    // setup scene
    osg::Group* node = new osg::Group();
    osg::Node* loadedModel = NULL;
    if (argc > 2) loadedModel = osgDB::readNodeFile(arguments[2]);
    if (!loadedModel) loadedModel = createTeapot();
    if (!loadedModel) return 1;
    node->addChild(loadedModel);

    // disable color clamping, because we want to work on real hdr values
    osg::ClampColor* clamp = new osg::ClampColor();
    clamp->setClampVertexColor(GL_FALSE);
    clamp->setClampFragmentColor(GL_FALSE);
    clamp->setClampReadColor(GL_FALSE);

    // make it protected and override, so that it is done for the whole rendering pipeline
    node->getOrCreateStateSet()->setAttribute(clamp, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE | osg::StateAttribute::PROTECTED);

    // load the processor from a file
    osg::ref_ptr<osgPPU::Processor> processor = dynamic_cast<osgPPU::Processor*>(osgDB::readObjectFile(arguments[1]));
    if (!processor)
    {
        osg::notify(osg::FATAL) << "File does not contain a valid pipeline" << std::endl;
        return 0;
    }

    // setup viewers camera
    setupCamera(viewer->getCamera());
    processor->setCamera(viewer->getCamera());

    // add processor to the scene
    node->addChild(processor.get());

    // add model to viewer.
    viewer->setSceneData( node );

    // run viewer
    return viewer->run();
}



