#include <osg/GLExtensions>
#include <osgViewer/Renderer>
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>
#include <osgDB/WriteFile>
#include <osgDB/ReadFile>
#include <osg/MatrixTransform>
#include <osg/AnimationPath>
#include <osg/ShapeDrawable>
#include <osg/Material>
#include <osg/ClampColor>

#include "dofppu.h"

//--------------------------------------------------------------------------
// Costumized viewer to support updating of osgppu
//--------------------------------------------------------------------------
class Viewer : public osgViewer::Viewer
{
    private:
        osg::ref_ptr<osgPPU::Processor> mProcessor;

        float mOldTime;
        DoFRendering mDoFSetup;
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
        static osg::Texture* createRenderTexture(int tex_width, int tex_height, bool depth)
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

        //! Setup the camera to do the render to texture
        void setupCamera(osg::Viewport* vp)
        {
            // setup viewer's default camera
            osg::Camera* camera = getCamera();

            // create texture to render to
            osg::Texture* texture = createRenderTexture((int)vp->width(), (int)vp->height(), false);
            osg::Texture* depthTexture = createRenderTexture((int)vp->width(), (int)vp->height(), true);

            // set up the background color and clear mask.
            camera->setClearColor(osg::Vec4(0.0f,0.0f,0.0f,0.0f));
            camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // set viewport
            camera->setViewport(vp);
            camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
            camera->setProjectionMatrixAsPerspective(20.0, vp->width()/vp->height(), 0.1, 100.0);

            // tell the camera to use OpenGL frame buffer object where supported.
            camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

            // attach the texture and use it as the color buffer.
            camera->attach(osg::Camera::COLOR_BUFFER, texture);
            camera->attach(osg::Camera::DEPTH_BUFFER, depthTexture);
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
            osgPPU::Unit* lastUnit = NULL;

            mDoFSetup.createDoFPipeline(mProcessor.get(), lastUnit, 0.1, 100.0);

            // add a text ppu after the pipeline is setted up
            {
                osgPPU::UnitText* fpstext = new osgPPU::UnitText();
                fpstext->setName("FPSTextPPU");
                fpstext->setSize(44);
                fpstext->setText("Example DoF-pipeline from a .ppu file");
                fpstext->setPosition(0.01, 0.95);
                lastUnit->addChild(fpstext);
            }

            // As a last step we setup a ppu which do render the content of the result
            // on the screenbuffer. This ppu MUST be as one of the last, otherwise you
            // will not be able to get results from the ppu pipeline
            osgPPU::UnitOut* ppuout = new osgPPU::UnitOut();
            ppuout->setName("PipelineResult");
            ppuout->setInputTextureIndexForViewportReference(-1); // need this here to get viewport from camera
            lastUnit->addChild(ppuout);

            // write pipeline to a file
            osgDB::writeObjectFile(*mProcessor, "dof.ppu");
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

                if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F1)
                {
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
    unsigned int screenWidth;
    unsigned int screenHeight;
    osg::GraphicsContext::getWindowingSystemInterface()->getScreenResolution(osg::GraphicsContext::ScreenIdentifier(0), screenWidth, screenHeight);
    unsigned int windowWidth = 640;
    unsigned int windowHeight = 480;
    viewer->setUpViewInWindow((screenWidth-windowWidth)/2, (screenHeight-windowHeight)/2, windowWidth, windowHeight);

    // setup scene
    osg::Group* node = new osg::Group();
    osg::Node* scene = osgDB::readNodeFile("Data/cow.osg");
    osg::MatrixTransform* rotation = new osg::MatrixTransform;
    rotation->setMatrix(osg::Matrix::rotate(osg::DegreesToRadians(15.0),0.0,1.0,0.0)
                        * osg::Matrix::rotate(osg::DegreesToRadians(65.0),0.0,0.0,-1.0));
    rotation->addChild(scene);

    node->addChild(rotation);

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

    // add a keyboard handler to react on user input
    viewer->addEventHandler(new KeyboardEventHandler(viewer.get()));

    // run viewer
    return viewer->run();
}



