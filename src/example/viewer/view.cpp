#include <osg/GLExtensions>
#include <osgViewer/Renderer>
#include <osgGA/TrackballManipulator>
#include <osgDB/WriteFile>

#include <osgPPU/Processor.h>

#include "osgteapot.h"

//--------------------------------------------------------------------------
// Costumized viewer to support updating of osgppu
//--------------------------------------------------------------------------
class Viewer : public osgViewer::Viewer
{
    private:
        osg::ref_ptr<osgPPU::Processor> mProcessor;
        osg::ref_ptr<osg::Camera> mCamera;
        osg::ref_ptr<osg::State> mState;
        osg::ref_ptr<osg::Node> mSceneData;

        bool mbInitialized;
        float mOldTime;
        std::string pipelineFilename;
                
    public:
        //! Default construcotr
        Viewer(osg::ArgumentParser& args) : osgViewer::Viewer(args)
        {
            mbInitialized = false;
            mOldTime = 0.0f;
        }

        //! Set filename of the pipeline to use
        void setPipelineFilename(const std::string& filename) { pipelineFilename = filename; }

        //! Get the ppu processor
        osgPPU::Processor* getProcessor() { return mProcessor.get(); }
        
        //! Create camera resulting texture
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

        //! Setup the camera to do the render to texture
        osg::Camera* setupCamera(osg::Viewport* vp)
        {
            // create texture to render to
            osg::Texture* texture = createRenderTexture((int)vp->width(), (int)vp->height());
            osg::Camera* camera = new osg::Camera();

            // set up the background color and clear mask.
            camera->setClearColor(osg::Vec4(1.0f,1.0f,1.0f,1.0f));
            camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // set viewport
            camera->setViewport(vp);
            camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
            camera->setProjectionMatrix(getCamera()->getProjectionMatrix());

            // set the camera to render after the main camera.
            camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
            camera->setRenderOrder(osg::Camera::PRE_RENDER);

            // tell the camera to use OpenGL frame buffer object where supported.
            camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

            // attach the texture and use it as the color buffer.
            camera->attach(osg::Camera::COLOR_BUFFER, texture);

            return camera;
        }

        //! Add scene data
        void setSceneData(osg::Node* n)
        {
            // if valid camera, then add new node to the camera and camera to the viewer
            if (mCamera.valid())
            {
                mCamera->removeChildren(0, mCamera->getNumChildren());
                mCamera->addChild(n);
                osgViewer::Viewer::setSceneData(mCamera.get());

            // if not then add node to the viewer
            }else
                osgViewer::Viewer::setSceneData(n);
        }
        
        //! Just setup some stuff
        void viewerInit()
        {
            // propagate the method
            osgViewer::Viewer::viewerInit();

            // we create our own camera instead of using osgViewer's master camera
            // since it gets somehow problematic (black screen)
            mCamera = setupCamera(getCamera()->getViewport());

            // we need to work on valid state, hence retrieve it
            mState = dynamic_cast<osgViewer::Renderer*>(getCamera()->getRenderer())->getSceneView(0)->getState();
        
            // this will setup the scene as child of the camera and camera as child of viewer
            setSceneData(getSceneData());
        }

        //! Setup osgppu for rendering
        void initialize()
        {
            // if already initialized then just do nothing
            if (mbInitialized == false)
                mbInitialized = true;
            else
                return;

            // I do not know why, but we have to place this here!!!
            mState->initializeExtensionProcs();

            // initialize the post process
            mProcessor = new osgPPU::Processor(mState.get());
            mProcessor->setCamera(mCamera.get());
            mProcessor->setName("Processor");

            osgPPU::Pipeline* pp = static_cast<osgPPU::Pipeline*>(osgDB::readObjectFile(pipelineFilename));
            mProcessor->setPipeline(*pp);
            
            // the post processing is updated by the post draw callback of the main camera
            // this is IMPORTANT because only in that way we can be sure that the
            // ppu pipeline is get rendered after the main camera is rendered and
            // before the main buffer swap take place
            mProcessor->initPostDrawCallback(getCamera());
        }

        //! Update the frames        
        void frame(double f = USE_REFERENCE_TIME)
        {            
            // update default viewer
            // this should also update the post processing graph
            // since it is attached to the camera
            osgViewer::Viewer::frame(f);

            // initilize if it was not done before
            initialize();

            // compute frame time
            float frameTime = elapsedTime() - mOldTime;
            mOldTime = elapsedTime();

            // update time, so that ppus work fine
            mProcessor->setTime(mOldTime);

            // update camera's view matrix according to the 
            mCamera->setViewMatrix(getCamera()->getViewMatrix());

    }
};



//--------------------------------------------------------------------------
int main(int argc, char **argv)
{
    // parse arguments
    osg::ArgumentParser arguments(&argc,argv);

    // give some info in the console
    printf("view ppufile [osgfile]\n");
    
    if (argc <= 1) return 0;

    // construct the viewer.
    osg::ref_ptr<Viewer> viewer = new Viewer(arguments);

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

    // add model to viewer.
    viewer->setSceneData( node );
    viewer->setPipelineFilename(arguments[1]);

    // run viewer                
    return viewer->run();
}


 
