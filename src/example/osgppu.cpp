#include "osgteapot.cpp"

#include <osgViewer/Renderer>

#include <osgPPU/PostProcess.h>
#include <osgPPU/PostProcessUnit.h>
#include <osgPPU/PPUInOut.h>
#include <osgPPU/PPUText.h>


//--------------------------------------------------------------------------
// Costumized viewer to support updating of osgppu
//--------------------------------------------------------------------------
class Viewer : public osgViewer::Viewer
{
    private:
        osg::ref_ptr<osgPPU::PostProcess> mPostProcess;
        osg::ref_ptr<osg::Camera> mCamera;
        
    public:
        Viewer(osg::ArgumentParser& args) : osgViewer::Viewer(args){}
    
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
        void setupCamera(osg::Camera* camera, osg::Viewport* vp)
        {
            // create texture to render to
            osg::Texture* texture = createRenderTexture((int)vp->width(), (int)vp->height());
            
            // set up the background color and clear mask.
            camera->setClearColor(osg::Vec4(0.0f,0.0f,0.0f,0.0f));
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
        }

        //! Add scene data
        void setSceneData(osg::Node* n)
        {
            if (mCamera.valid())
            {
                mCamera->removeChildren(0, mCamera->getNumChildren());
                mCamera->addChild(n);
            }
            osgViewer::Viewer::setSceneData(n);
        }
        
        //! Setup osgppu for rendering
        void viewerInit()
        {
            // propagate the method
            osgViewer::Viewer::viewerInit();

            // we create our own camera instead of using osgViewer's master camera
            // since it gets somehow problematic (black screen)
            mCamera = new osg::Camera();
            setupCamera(mCamera.get(), getCamera()->getViewport());
            mCamera->addChild(getSceneData());

            #if 1
            // initialize the post process
            mPostProcess = new osgPPU::PostProcess();
            mPostProcess->setCamera(mCamera.get());
            mPostProcess->setState(dynamic_cast<osgViewer::Renderer*>(getCamera()->getRenderer())->getSceneView(0)->getState());
            mPostProcess->setName("PostProcess");
            
            // now in order to get anything through the ppu system
            // we need to setup some default post procesing units
            osgPPU::PostProcess::FXPipeline pipeline;

            // this ppu do get the input from the camera and bypass it
            // You need this ppu to relay on it with following ppus
            osg::ref_ptr<osgPPU::PostProcessUnit> bypass = new osgPPU::PostProcessUnit(mPostProcess.get());
            bypass->setIndex(0);
            bypass->setName("CameraBypass");
            pipeline.push_back(bypass);

                    
            // next we setup a ppu which do render the conten of the whole pipeline
            // on the screenbuffer. This ppu MUST be as one of the last, otherwise you
            // will not able to get results from the ppu pipeline
            osg::ref_ptr<osgPPU::PostProcessUnit> ppuout = new osgPPU::PostProcessUnitOut(mPostProcess.get());
            ppuout->setIndex(100);
            ppuout->setName("PipelineResult");
            pipeline.push_back(ppuout);


            // now just as a gimmick do setup a text ppu, to render some info on the screen
            osgPPU::PostProcessUnitText* pputext = new osgPPU::PostProcessUnitText(mPostProcess.get());
            pputext->setIndex(50);
            pputext->setName("TextPPU");
            pputext->setSize(48);
            pputext->getText()->setText("osgPPU rocks!");
            pipeline.push_back(osg::ref_ptr<osgPPU::PostProcessUnit>(pputext));
            
            
            // finally we add the list of the ppus to the pipeline
            mPostProcess->setPipeline(pipeline);
            #endif
        
            // add now new camera as new scene data, so it gets updated
            getCamera()->addChild(mCamera.get());
            //mCamera->setPostDrawCallback(new osgPPU::PostProcess::Callback(mPostProcess.get()));

            // the post processing is updated by the post draw callback of the main camera
            getCamera()->setPostDrawCallback(new osgPPU::PostProcess::Callback(mPostProcess.get()));
        }

        
        void frame(double f)
        {
            // update default viewer
            // this should also update the post processing graph
            // since it is attached to the camera
            osgViewer::Viewer::frame(f);
            
            // update time, so that ppus work fine
            mPostProcess->setTime(elapsedTime());

            // update camera's view matrix according to the 
            mCamera->setViewMatrix(getCamera()->getViewMatrix());
        }
        
};



//--------------------------------------------------------------------------
int main(int argc, char **argv)
{
    osg::ArgumentParser arguments(&argc,argv);

    // construct the viewer.
    osg::ref_ptr<Viewer> viewer = new Viewer(arguments);

    // add model to viewer.
    viewer->setSceneData( createTeapot() );
    viewer->setUpViewInWindow(0, 0, 640, 480);
    
    // run costumzied viewer, so that we can update our own stuff
    return viewer->run();
}


 
