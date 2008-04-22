#include <osg/GLExtensions>
#include <osgViewer/Renderer>
#include <osgGA/TrackballManipulator>
#include <osgDB/WriteFile>

#include <osgPPU/Processor.h>

#include "osgteapot.h"

//--------------------------------------------------------------------------
// Create camera resulting texture
//--------------------------------------------------------------------------
osg::Texture* createRenderTexture(int tex_width, int tex_height)
{
    // create simple 2D texture
    osg::Texture2D* texture2D = new osg::Texture2D;
    texture2D->setTextureSize(tex_width, tex_height);
    texture2D->setInternalFormat(GL_RGBA);
    texture2D->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
    texture2D->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);

    // since we want to use HDR, setup float format
    texture2D->setInternalFormat(GL_RGBA16F_ARB);
    texture2D->setSourceFormat(GL_RGBA);
    texture2D->setSourceType(GL_FLOAT);

    return texture2D;
}

//--------------------------------------------------------------------------
// Setup the camera to do the render to texture
//--------------------------------------------------------------------------
void setupCamera(osg::Camera* camera)
{
    osg::Viewport* vp = camera->getViewport();

    // create texture to render to
    osg::Texture* texture = createRenderTexture((int)vp->width(), (int)vp->height());

    // set up the background color and clear mask.
    camera->setClearColor(osg::Vec4(0.0f,0.0f,0.0f,0.0f));
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // set viewport
    camera->setViewport(vp);
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

    // tell the camera to use OpenGL frame buffer object where supported.
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

    // attach the texture and use it as the color buffer.
    camera->attach(osg::Camera::COLOR_BUFFER, texture);
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
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->setUpViewInWindow(0,0,640,480);

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
    osgPPU::Processor* processor = static_cast<osgPPU::Processor*>(osgDB::readObjectFile(arguments[1]));
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
    viewer->setSceneData( node );

    // run viewer                
    return viewer->run();
}


 
