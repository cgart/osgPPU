#include <osg/GLExtensions>
#include <osgViewer/Renderer>
#include <osgGA/TrackballManipulator>
#include <osgDB/WriteFile>

#include "osgteapot.h"
#include "hdrppu.h"

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
        HDRRendering mHDRSetup;

                
    public:
        //! Default construcotr
        Viewer(osg::ArgumentParser& args) : osgViewer::Viewer(args)
        {
            mbInitialized = false;
            mOldTime = 0.0f;
        }

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
            
            // we want to simulate hdr rendering, hence setup the pipeline
            // for the hdr rendering
            osgPPU::Pipeline pipeline = mHDRSetup.createHDRPipeline(mProcessor.get());

            // This ppu do get the input from the camera and bypass it
            // You MUST have this ppu to get the data from the camera into the pipeline
            // A simple bypass ppu doesn't cost too much performance, since there is no
            // rendering. It is just a copying of input to output data.
            /*osg::ref_ptr<osgPPU::Unit> bypass = new osgPPU::Unit(mState.get());
            bypass->setIndex(0);
            bypass->setName("CameraBypass");
            pipeline.push_back(bypass);*/


            // next we setup a ppu which do render the content of the result
            // on the screenbuffer. This ppu MUST be as one of the last, otherwise you
            // will not be able to get results from the ppu pipeline
            osg::ref_ptr<osgPPU::Unit> ppuout = new osgPPU::UnitOut(mState.get());
            ppuout->setIndex(1000);
            ppuout->setName("PipelineResult");
            pipeline.push_back(ppuout);

            // now just as a gimmick do setup a text ppu, to render some info on the screen
            osgPPU::UnitText* pputext = new osgPPU::UnitText(mState.get());
            pputext->setIndex(999);
            pputext->setName("TextPPU");
            pputext->setSize(26);
            pputext->setText("osgPPU rocks!");
            pputext->setPosition(0.0, 0.4);
            pipeline.push_back(osg::ref_ptr<osgPPU::Unit>(pputext));

            // finally we add the list of the ppus to the pipeline
            mProcessor->setPipeline(pipeline);

            // now after the pipeline is setted up, we add offline ppus to compute adapted luminace
            mHDRSetup.setupPPUsToComputeAdaptedLuminance(mProcessor.get());

            // test writing to file 
            osgDB::writeObjectFile(mProcessor->getPipeline(), "Data/hdr.ppu");        
            
            // This ppu just render a texture over the screen.
            // We do this after we have setted up the pipeline, so that we can
            // seamless include this into the pipeline.
            // This ppu could be setted up before, but in this way we just 
            // demonstrate how to include ppus after pipeline setup
            {
                // we need this to work on
                osg::ref_ptr<osgPPU::Unit> bypass = mProcessor->getUnit("HDRBypass");

                // create picture in picture ppu 
                osg::ref_ptr<osgPPU::Unit> bgppu = new osgPPU::UnitInOut(mState.get());
                bgppu->setName("PictureInPicturePPU");

                // set index two 200, so that this ppu does get updated after the hdr pipeline
                bgppu->setIndex(200);

                // now we have to include this ppu into the pipeline
                mProcessor->addUnitToPipeline(bgppu.get());

                // after adding the ppu the input and output textures changes
                // according to the pipeline. Hence we have to change this how we
                // we want to have it
                
                // input texture is output of the camera bypass, so that we see original scene view
                osg::Texture2D* img = new osg::Texture2D();
                img->setImage(osgDB::readImageFile("Images/reflect.rgb"));
                bgppu->setInputTexture(img, 0);
                //bgppu->setInputTexture(bypass->getOutputTexture(0), 0);

                // output texture will be the same as the output of the text ppu,
                // which is the same as the output of the hdr ppu.
                bgppu->setOutputTexture(pputext->getOutputTexture(0), 0);

                // we do not want to use any ppu for viewport reference because we setted up our own viewport
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

                osg::Shader* sh = osg::Shader::readShaderFile(osg::Shader::FRAGMENT, "Data/bypass_fp.glsl");
                osgPPU::Shader* shader = new osgPPU::Shader();
                shader->addShader(sh);
                shader->add("texUnit0", osg::Uniform::SAMPLER_2D);
                shader->set("texUnit0", 0);

                bgppu->setShader(shader);
            }

            // add a text ppu after the pipeline is setted up
            {
                osgPPU::UnitText* fpstext = new osgPPU::UnitText(mState.get());
                fpstext->setIndex(999);
                fpstext->setName("FPSTextPPU");
                fpstext->setSize(24);
                fpstext->setText("FPS: ");
                fpstext->setPosition(0.0, 0.95);

                mProcessor->addUnitToPipeline(fpstext);
                fpstext->init();
            }
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

            // We have to update the frame interval in one shader.
            // This is needed to simulate light adaption on different brightness
            // of the scene. Since this is only an example application we can
            // include such an ugly piece of code. In your final application
            // I would suggest to solve this in another way
            {
                // get ppu containing the shader with the variable
                osgPPU::Unit* ppu = mProcessor->getUnit("AdaptedLuminance");
                if (ppu)
                    ppu->getShader()->set("invFrameTime", frameTime);
            }

            // print also some info about the fps number
            {
                osgPPU::UnitText* ppu = dynamic_cast<osgPPU::UnitText*>(mProcessor->getUnit("FPSTextPPU"));
                if (ppu)
                {
                    char txt[64];
                    sprintf(txt, "FPS:%4.2f", 1.0 / frameTime);
                    ppu->setText(txt);
                }
            }
            
    }
};


//--------------------------------------------------------------------------
// Event handler to react on user input
// You can switch with some keys to specified states of the HDR pipeline
//--------------------------------------------------------------------------
class KeyboardEventHandler : public osgGA::GUIEventHandler
{
public:
    osg::ref_ptr<Viewer> viewer;
    
    KeyboardEventHandler(Viewer* v) : viewer(v)
    {
    }

    bool handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter&)
    {
        switch(ea.getEventType())
        {
            case(osgGA::GUIEventAdapter::KEYDOWN):
            case(osgGA::GUIEventAdapter::KEYUP):
            {
                osgPPU::Unit* ppu = viewer->getProcessor()->getUnit("PictureInPicturePPU");
                osgPPU::UnitText* textppu = dynamic_cast<osgPPU::UnitText*>(viewer->getProcessor()->getUnit("TextPPU"));

                if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F1)
                {
                    ppu->setInputTexture(viewer->getProcessor()->getUnit("HDRBypass")->getOutputTexture(0), 0);
                    textppu->setText("Original Input");
                }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F2)
                {
                    ppu->setInputTexture(viewer->getProcessor()->getUnit("ComputeLuminance")->getOutputTexture(0), 0);
                    textppu->setText("Per Pixel Luminance");
                }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F3)
                {
                    ppu->setInputTexture(viewer->getProcessor()->getUnit("Brightpass")->getOutputTexture(0), 0);
                    textppu->setText("Brightpass");
                }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F4)
                {
                    ppu->setInputTexture(viewer->getProcessor()->getUnit("BlurVertical")->getOutputTexture(0), 0);
                    textppu->setText("Gauss Blur on Brightpass");
                }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F5)
                {
                    ppu->setInputTexture(viewer->getProcessor()->getUnit("AdaptedLuminanceCopy")->getOutputTexture(0), 0);
                    textppu->setText("Adapted Luminance");
                }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F6)
                {
                    ppu->setBlendStartValue(1.0);
                    ppu->setBlendFinalValue(0.0);
                    ppu->setBlendStartTimeToCurrent();
                    ppu->setBlendDuration(3.0);
                    ppu->setUseBlendMode(true);
                }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F7)
                {
                    ppu->setBlendStartValue(0.0);
                    ppu->setBlendFinalValue(1.0);
                    ppu->setBlendStartTimeToCurrent();
                    ppu->setBlendDuration(3.0);
                    ppu->setUseBlendMode(true);
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
int main(int argc, char **argv)
{
    // parse arguments
    osg::ArgumentParser arguments(&argc,argv);

    // construct the viewer.
    osg::ref_ptr<Viewer> viewer = new Viewer(arguments);

    // just make it singlethreaded since I get some problems if not in this mode
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->setUpViewInWindow(0,0,512,512);

    // setup scene
    osg::Group* node = new osg::Group();
    osg::Node* loadedModel = osgDB::readNodeFiles(arguments);
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
        
    // give some info in the console
    printf("osgppu [filename]\n");
    printf("Keys:\n");
    printf("\tF1 - Show original input\n");
    printf("\tF2 - Show luminance per pixel\n");
    printf("\tF3 - Show brightpassed pixels\n");
    printf("\tF4 - Show blurred version of brightpassed pixels\n");
    printf("\tF5 - Show the 1x1 texture with adapted luminance value\n");
    printf("\tF6 - Fade out the picture in picture\n");
    printf("\tF7 - Fade in the picture in picture\n");

    // add a keyboard handler to react on user input
    viewer->addEventHandler(new KeyboardEventHandler(viewer.get()));
                    
    // run viewer                
    return viewer->run();
}


 
