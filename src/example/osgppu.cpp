#include <osg/GLExtensions>
#include <osgViewer/Renderer>
#include <osgGA/TrackballManipulator>

#include "osgteapot.cpp"
#include "hdrppu.cpp"

//--------------------------------------------------------------------------
// Event handler to react on user input
// You can switch with the keys F1, F2, F3, F4 to specified states of the HDR pipeline
//--------------------------------------------------------------------------
class KeyboardEventHandler : public osgGA::GUIEventHandler
{
public:
        osg::ref_ptr<osgPPU::PostProcess> mPostProcess;
        
        KeyboardEventHandler(osgPPU::PostProcess* pp) : mPostProcess(pp)
        {
        }
    
        bool handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter&)
        {
            switch(ea.getEventType())
            {
                case(osgGA::GUIEventAdapter::KEYDOWN):
                case(osgGA::GUIEventAdapter::KEYUP):
                {
                    osgPPU::PostProcessUnit* ppu = mPostProcess->getPPU("PictureInPicturePPU");
                    osgPPU::PostProcessUnitText* textppu = dynamic_cast<osgPPU::PostProcessUnitText*>(mPostProcess->getPPU("TextPPU"));

                    if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F1)
                    {
                        ppu->setInputTexture(mPostProcess->getPPU("CameraBypass")->getOutputTexture(0), 0);
                        textppu->getText()->setText("Original Input");
                    }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F2)
                    {
                        ppu->setInputTexture(mPostProcess->getPPU("ComputeLuminance")->getOutputTexture(0), 0);
                        textppu->getText()->setText("Per Pixel Luminance");
                    }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F3)
                    {
                        ppu->setInputTexture(mPostProcess->getPPU("Brightpass")->getOutputTexture(0), 0);
                        textppu->getText()->setText("Brightpass");
                    }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F4)
                    {
                        ppu->setInputTexture(mPostProcess->getPPU("BlurVertical")->getOutputTexture(0), 0);
                        textppu->getText()->setText("Gauss Blur on Brightpass");
                    }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F5)
                    {
                        ppu->setStartBlendValue(1.0);
                        ppu->setEndBlendValue(0.0);
                        ppu->setStartBlendTimeToCurrent();
                        ppu->setBlendDuration(3.0);
                        ppu->setBlendMode(true);
                    }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F6)
                    {
                        ppu->setStartBlendValue(0.0);
                        ppu->setEndBlendValue(1.0);
                        ppu->setStartBlendTimeToCurrent();
                        ppu->setBlendDuration(3.0);
                        ppu->setBlendMode(true);
                    }
                    break;
                }
                default:
                    break;
            }
            return false;            
        }
};

//--------------------------------------------------------------------------
// Costumized viewer to support updating of osgppu
//--------------------------------------------------------------------------
class Viewer : public osgViewer::Viewer
{
    private:
        osg::ref_ptr<osgPPU::PostProcess> mPostProcess;
        osg::ref_ptr<osg::Camera> mCamera;
        HDRRendering mHDRSetup;
        osg::ref_ptr<osg::State> mState;

        bool mbInitialized;
        
        float mOldTime;
        osg::ref_ptr<osg::Node> mSceneData;
                
    public:
        Viewer(osg::ArgumentParser& args) : osgViewer::Viewer(args)
        {
            mbInitialized = false;
            mOldTime = 0.0f;
            mCamera = new osg::Camera();
            //osgViewer::Viewer::setSceneData(createTeapot());
        }
    
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
        
        //! Just setup some stuff
        void viewerInit()
        {
            // propagate the method
            osgViewer::Viewer::viewerInit();

            // we create our own camera instead of using osgViewer's master camera
            // since it gets somehow problematic (black screen)
            setupCamera(mCamera.get(), getCamera()->getViewport());
            mCamera->addChild(getSceneData());
            mState = dynamic_cast<osgViewer::Renderer*>(getCamera()->getRenderer())->getSceneView(0)->getState();
        }

        //! Setup osgppu for rendering
        void initialize()
        {
            if (mbInitialized == false)
                mbInitialized = true;
            else
                return;

            // I do not know why, but we have to place this here!!!
            mState->initializeExtensionProcs();

            // initialize the post process
            mPostProcess = new osgPPU::PostProcess();
            mPostProcess->setCamera(mCamera.get());
            mPostProcess->setState(mState.get());
            mPostProcess->setName("PostProcess");
            
            // disable color clamping, because we want to work on real hdr values
            osg::ClampColor* clamp = new osg::ClampColor();
            clamp->setClampVertexColor(false);
            clamp->setClampFragmentColor(false);
            mPostProcess->getOrCreateStateSet()->setAttribute(clamp, osg::StateAttribute::ON);

            // we want to simulate hdr rendering, hence setup the pipeline
            // for the hdr rendering
            osgPPU::PostProcess::FXPipeline pipeline = mHDRSetup.createPipeline(mPostProcess.get());

            // This ppu do get the input from the camera and bypass it
            // You MUST have this ppu to get the data from the camera into the pipeline
            // A simple bypass ppu doesn't cost too much performance, since there is no
            // rendering. It is just a copying of input to output data.
            osg::ref_ptr<osgPPU::PostProcessUnit> bypass = new osgPPU::PostProcessUnit(mPostProcess.get());
            bypass->setIndex(0);
            bypass->setName("CameraBypass");
            pipeline.push_back(bypass);


            // next we setup a ppu which do render the conten of the whole pipeline
            // on the screenbuffer. This ppu MUST be as one of the last, otherwise you
            // will not able to get results from the ppu pipeline
            osg::ref_ptr<osgPPU::PostProcessUnit> ppuout = new osgPPU::PostProcessUnitOut(mPostProcess.get());
            ppuout->setIndex(1000);
            ppuout->setName("PipelineResult");
            pipeline.push_back(ppuout);


            // now just as a gimmick do setup a text ppu, to render some info on the screen
            osgPPU::PostProcessUnitText* pputext = new osgPPU::PostProcessUnitText(mPostProcess.get());
            pputext->setIndex(999);
            pputext->setName("TextPPU");
            pputext->setSize(26);
            pputext->getText()->setText("osgPPU rocks!");
            pputext->getText()->setPosition(0.005, 0.4);
            pipeline.push_back(osg::ref_ptr<osgPPU::PostProcessUnit>(pputext));

            // finally we add the list of the ppus to the pipeline
            mPostProcess->setPipeline(pipeline);

            // This ppu just render a texture over the screen.
            // We do this after we have setted up the pipeline, so that we can
            // seamless include this into the pipeline.
            {
                osg::ref_ptr<osgPPU::PostProcessUnit> bgppu = new osgPPU::PostProcessUnitInOut(mPostProcess.get());
                bgppu->setName("PictureInPicturePPU");

                // set index two 1, so that this ppu does get updated right after the bypass
                bgppu->setIndex(200);

                // now we have to include this ppu into the pipeline
                mPostProcess->addPPUToPipeline(bgppu.get());

                // after adding the ppu the input and output textures changes
                // according to the pipeline. Hence we have to change this how we
                // we want to have it
                
                // input texture is an image
                //bgppu->setInputTexture(new osg::Texture2D(osgDB::readImageFile("Images/Sky.jpg")), 0);
                bgppu->setInputTexture(bypass->getOutputTexture(0), 0);

                // output texture will be the same as the output of the hdr ppu
                bgppu->setOutputTexture(pputext->getOutputTexture(0), 0);

                // we also have to change the viewport to the viewport of the pipeline
                bgppu->setViewport(bypass->getViewport());

                // we do not want to use any ppu for viewport reference
                bgppu->setInputTextureIndexForViewportReference(-1);

                // we also disable viewport reference on the textppu, so that
                // after adding this ppu the text ppu doesn't get new viewport assigned
                pputext->setInputTextureIndexForViewportReference(-1);

                // and finally initialize the ppu
                bgppu->init();

                // setup new viewport, which will change the rendering position
                osg::Viewport* vp = new osg::Viewport(*bypass->getViewport());
                vp->x() = 0;
                vp->y() = 0;
                vp->width() = bypass->getViewport()->width() * 0.4;
                vp->height() = bypass->getViewport()->height() * 0.4;
                bgppu->setViewport(vp);
            }

            // add now new camera as new scene data, so it gets updated
            getCamera()->addChild(mCamera.get());

            // the post processing is updated by the post draw callback of the main camera
            mPostProcess->initPostDrawCallback(getCamera());

            // add a keyboard handler to react on input
            addEventHandler(new KeyboardEventHandler(mPostProcess.get()));
        }

        
        void frame(double f)
        {            
            // update default viewer
            // this should also update the post processing graph
            // since it is attached to the camera
            osgViewer::Viewer::frame(f);

            // If we have not initialized the things bofore, then do this now
            // We have to perform this here otherwise seg faults
            // Precall of realize() doesn't help. Maybe bug in osgViewer ???
            initialize();

            // compute frame time
            float frameTime = elapsedTime() - mOldTime;
            mOldTime = elapsedTime();

            // update time, so that ppus work fine
            mPostProcess->setTime(mOldTime);

            // update camera's view matrix according to the 
            mCamera->setViewMatrix(getCamera()->getViewMatrix());

            // We have to update the frame interval in one shader.
            // This is needed to simulate light adaption on different brightness
            // of the scene. Since this is only an example application we can
            // include such an ugly piece of code. In your final application
            // I would suggest to solve this in another way
            {
                // get ppu containing the shader with the variable
                osgPPU::PostProcessUnit* ppu = mPostProcess->getPPU("ComputeLuminance");
                if (ppu)
                    ppu->getMipmapShader()->set("invFrameTime", frameTime);
            }

            // print also some info about the fps number
            /*osgPPU::PostProcessUnitText* ppu = dynamic_cast<osgPPU::PostProcessUnitText*>(mPostProcess->getPPU("TextPPU"));
            if (ppu)
            {
                char txt[64];
                sprintf(txt, "osgPPU rocks! FPS:%4.2f", 1.0 / frameTime);
                ppu->getText()->setText(txt);
            }*/

            
    }
};



//--------------------------------------------------------------------------
int main(int argc, char **argv)
{
    osg::ArgumentParser arguments(&argc,argv);

    // construct the viewer.
    osg::ref_ptr<Viewer> viewer = new Viewer(arguments);

    // setup scene
    osg::Group* node = new osg::Group();
    osg::Node* loadedModel = osgDB::readNodeFiles(arguments);
    if (!loadedModel) loadedModel = osgDB::readNodeFile("Data/cessnafire.osg");
    if (!loadedModel) return 1;
    node->addChild(loadedModel);
    
    // disable color clamping, because we want to work on real hdr values
    osg::ClampColor* clamp = new osg::ClampColor();
    clamp->setClampVertexColor(false);
    node->getOrCreateStateSet()->setAttribute(clamp, osg::StateAttribute::ON);


    // add model to viewer.
    viewer->setSceneData( node );
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    
    // give some info in the console
    printf("osgppu [filename]\n");
    printf("Keys:\n");
    printf("\tF1 - Show original input\n");
    printf("\tF2 - Show luminance per pixel\n");
    printf("\tF3 - Show brightpassed pixels\n");
    printf("\tF4 - Show blurred version of brightpassed pixels\n");
    printf("\tF5 - Fade out the picture in picture\n");
    printf("\tF6 - Fade in the picture in picture\n");
    
    // run costumzied viewer, so that we can update our own stuff
    return viewer->run();
}


 
