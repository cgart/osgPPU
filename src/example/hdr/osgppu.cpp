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

        float mOldTime;
        HDRRendering mHDRSetup;
        bool mbInitialized;
                
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
        static osg::Texture* createRenderTexture(int tex_width, int tex_height)
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
        void setupCamera(osg::Viewport* vp)
        {
            // setup viewer's default camera
            osg::Camera* camera = getCamera();

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

        //! Just setup some stuff
        void viewerInit()
        {
            // propagate the method
            osgViewer::Viewer::viewerInit();

            // setup data
            setupCamera(getCamera()->getViewport());

            // add ppu processor into the scene graph
            osg::Group* group = new osg::Group();
            group->addChild(getSceneData());
            setSceneData(group);
        }

        //! Setup osgppu for rendering
        void initialize()
        {
           // if already initialized then just do nothing
            if (mbInitialized == false)
                mbInitialized = true;
            else
                return;

            mProcessor = new osgPPU::Processor();
            dynamic_cast<osg::Group*>(getSceneData())->addChild(mProcessor.get());

            // initialize the post process
            mProcessor->setCamera(getCamera());
            mProcessor->setName("Processor");
            mProcessor->dirtyUnitSubgraph();
                            
            // we want to simulate hdr rendering, hence setup the pipeline
            // for the hdr rendering
            osgPPU::Unit* firstUnit = NULL;
            osgPPU::Unit* lastUnit = NULL;

            mHDRSetup.createHDRPipeline(mProcessor.get(), firstUnit, lastUnit);
            mProcessor->addChild(firstUnit);

            // add a text ppu after the pipeline is setted up
            {
                osgPPU::UnitText* fpstext = new osgPPU::UnitText();
                fpstext->setName("FPSTextPPU");
                fpstext->setSize(44);
                fpstext->setText("Example HDR-pipeline from a .ppu file (note: no change in adaptive luminance)");
                fpstext->setPosition(0.01, 0.95);
                lastUnit->addChild(fpstext);
            }
            

            // write pipeline to a file
            //osgDB::writeObjectFile(*mProcessor, "hdr.ppu");

            // now just as a gimmick do setup a text ppu, to render some info on the screen
            osgPPU::UnitText* pputext = new osgPPU::UnitText();
            pputext->setName("TextPPU");
            pputext->setSize(46);
            pputext->setText("osgPPU rocks!");
            pputext->setPosition(0.025, 0.425);
            lastUnit->addChild(pputext);
            
            // The following setup does show how to include an offline ppu into the graph
            // This unit will just render the content of the input unit in a small window 
            // over the screen.
            if (1)
            {
                // This is a simple texture unit, which do just provide a given texture 
                // to the output, so that all children units can access this texture as input.
                osgPPU::UnitTexture* unittex = new osgPPU::UnitTexture();

                // it doesn't matter where to put this unit in the graph, because it 
                // does not use any input. However so that this unit is updated every 
                // frame we put it somewhere, in this case under the processor
                mProcessor->addChild(unittex);

                // just to find it later in the graph
                unittex->setName("TextureUnit");

                // input texture is output of the camera bypass, so that we see original scene view
                osg::Texture2D* img = new osg::Texture2D();
                img->setImage(osgDB::readImageFile("Data/Images/reflect.rgb"));
                unittex->setTexture(img);

                // create picture in picture ppu 
                osgPPU::UnitInOut* bgppu = new osgPPU::UnitInOut();
                bgppu->setName("PictureInPicturePPU");

                // at the beginning we setup the unittex as input to this unit
                unittex->addChild(bgppu);

                // this ppu has to be rendered after the hdr pipeline output
                // however it shouldn't take the input of hdr output.
                lastUnit->addChild(bgppu);

                // hence ignore the input of the parent with index 1, which is hdr output
                bgppu->setIgnoreInput(1, true);

                // output texture will be the same as the output of the last ppu,
                // which is the same as the output of the hdr ppu.
                bgppu->setInputBypass(1);

                // we do not want to use any ppu for viewport reference because we setted up our own viewport
                bgppu->setInputTextureIndexForViewportReference(-1);

                // setup new viewport, which will change the rendering position
                osg::Viewport* vp = new osg::Viewport(*(getCamera()->getViewport()));
                vp->x() = 10;
                vp->y() = 10;
                vp->width() *= 0.4;
                vp->height() *= 0.3;                
                bgppu->setViewport(vp);

                // enable blending on this unit
                bgppu->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
            }

            // As a last step we setup a ppu which do render the content of the result
            // on the screenbuffer. This ppu MUST be as one of the last, otherwise you
            // will not be able to get results from the ppu pipeline
            osgPPU::UnitOut* ppuout = new osgPPU::UnitOut();
            ppuout->setName("PipelineResult");
            ppuout->setInputTextureIndexForViewportReference(-1); // need this here to get viewport from camera
            lastUnit->addChild(ppuout);
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

            // We have to update the frame interval in one shader.
            // This is needed to simulate light adaption on different brightness
            // of the scene. Since this is only an example application we can
            // include such an ugly piece of code. In your final application
            // I would suggest to solve this in another way
            if (1)
            {
                // get ppu containing the shader with the variable
                osgPPU::Unit* ppu = mProcessor->findUnit("AdaptedLuminance");
                if (ppu)
                    ppu->getShader()->set("invFrameTime", frameTime);
            }

            // print also some info about the fps number
            if (1)
            {
                osgPPU::UnitText* ppu = dynamic_cast<osgPPU::UnitText*>(mProcessor->findUnit("FPSTextPPU"));
                if (ppu)
                {
                    char txt[64];
                    sprintf(txt, "FPS: %4.2f", 1.0 / frameTime);
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
                osgPPU::UnitTexture* ppu = dynamic_cast<osgPPU::UnitTexture*>(viewer->getProcessor()->findUnit("TextureUnit"));
                osgPPU::UnitText* textppu = dynamic_cast<osgPPU::UnitText*>(viewer->getProcessor()->findUnit("TextPPU"));
                osgPPU::UnitInOut* pip = dynamic_cast<osgPPU::UnitInOut*>(viewer->getProcessor()->findUnit("PictureInPicturePPU"));

                if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F1)
                {
                    ppu->setTexture(viewer->getProcessor()->findUnit("HDRBypass")->getOrCreateOutputTexture(0));
                    textppu->setText("Original Input");
                }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F2)
                {
                    ppu->setTexture(viewer->getProcessor()->findUnit("ComputePixelLuminance")->getOrCreateOutputTexture(0));
                    textppu->setText("Per Pixel Luminance");
                }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F3)
                {
                    ppu->setTexture(viewer->getProcessor()->findUnit("Brightpass")->getOrCreateOutputTexture(0));
                    textppu->setText("Brightpass");
                }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F4)
                {
                    ppu->setTexture(viewer->getProcessor()->findUnit("BlurVertical")->getOrCreateOutputTexture(0));
                    textppu->setText("Gauss Blur on Brightpass");
                }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F5)
                {
                    ppu->setTexture(viewer->getProcessor()->findUnit("AdaptedLuminanceCopy")->getOrCreateOutputTexture(0));
                    textppu->setText("Adapted Luminance");
                }
                #if 1
                else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F6 && pip)
                {
                    pip->getColorAttribute()->setStartColor(osg::Vec4(1,1,1,1));
                    pip->getColorAttribute()->setEndColor(osg::Vec4(1,1,1,0));
                    pip->getColorAttribute()->setStartTime(viewer->elapsedTime());
                    pip->getColorAttribute()->setEndTime(pip->getColorAttribute()->getStartTime() + 3.0);
                }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F7 && pip)
                {
                    pip->getColorAttribute()->setStartColor(osg::Vec4(1,1,1,0));
                    pip->getColorAttribute()->setEndColor(osg::Vec4(1,1,1,1));
                    pip->getColorAttribute()->setStartTime(viewer->elapsedTime());
                    pip->getColorAttribute()->setEndTime(pip->getColorAttribute()->getStartTime() + 3.0);
                }
                #endif
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
    unsigned int screenWidth;
    unsigned int screenHeight;
    osg::GraphicsContext::getWindowingSystemInterface()->getScreenResolution(osg::GraphicsContext::ScreenIdentifier(0), screenWidth, screenHeight);
    unsigned int windowWidth = 640;
    unsigned int windowHeight = 480;
    viewer->setUpViewInWindow((screenWidth-windowWidth)/2, (screenHeight-windowHeight)/2, windowWidth, windowHeight);

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

    viewer->setSceneData( node );
        
    // give some info in the console
    printf("hdr [filename]\n");
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


 
