#include <osg/Node>
#include <osg/Geometry>
#include <osg/Notify>
#include <osg/Texture3D>
#include <osg/TexGen>
#include <osg/Geode>
#include <osg/ClampColor>
#include <osg/ImageStream>
#include <osg/Texture2D>

#include <osgDB/Registry>
#include <osgDB/ReadFile>

#include <osgViewer/Viewer>

#include <osgPPU/Processor.h>
#include <osgPPU/UnitInOut.h>
#include <osgPPU/UnitTexture.h>
#include <osgPPU/ShaderAttribute.h>

#include <iostream>


//
// A simple demo demonstrating video processing with osgPPU
//

osg::Uniform* gUniform = NULL;
osg::Texture2D* gVideoTexture = NULL;
osg::Texture* gUnitTexture = NULL;
osg::Geode*   gQuad = NULL;

//--------------------------------------------------------------------------
const char* shaderSrc = 
    "uniform int osgppu_ViewportWidth;\n"
    "uniform int osgppu_ViewportHeight;\n"
    "uniform sampler2D inputTexture;\n"
    "uniform int useOriginalOutput;\n"
    "\n"
    "void main () {\n"
    "\n"
    "   // get color from the texture\n"
    "   vec3 color = texture2D(inputTexture, gl_TexCoord[0].st).rgb;\n"
    "\n"
    "   // modify the color\n"
    "   vec3 modified = vec3( color.r * 0.2125 + color.g * 0.7154 + color.b * 0.0721 );\n"
    "\n"
    "   if (useOriginalOutput)\n"
    "       gl_FragColor.rgb = color; \n"
    "   else\n"
    "       gl_FragColor.rgb = modified; \n"
    "\n"
    "   gl_FragColor.a = 1.0;\n"
    "}\n";


//--------------------------------------------------------------------------
// create a square with center at 0,0,0 and aligned along the XZ plan
//--------------------------------------------------------------------------
osg::Drawable* createSquare(float textureCoordMax=1.0f)
{
    // set up the Geometry.
    osg::Geometry* geom = new osg::Geometry;

    osg::Vec3Array* coords = new osg::Vec3Array(4);
    (*coords)[0].set(-1.25f,0.0f,1.0f);
    (*coords)[1].set(-1.25f,0.0f,-1.0f);
    (*coords)[2].set(1.25f,0.0f,-1.0f);
    (*coords)[3].set(1.25f,0.0f,1.0f);
    geom->setVertexArray(coords);

    osg::Vec3Array* norms = new osg::Vec3Array(1);
    (*norms)[0].set(0.0f,-1.0f,0.0f);
    geom->setNormalArray(norms);
    geom->setNormalBinding(osg::Geometry::BIND_OVERALL);

    osg::Vec2Array* tcoords = new osg::Vec2Array(4);
    (*tcoords)[0].set(0.0f,0.0f);
    (*tcoords)[1].set(0.0f,textureCoordMax);
    (*tcoords)[2].set(textureCoordMax,textureCoordMax);
    (*tcoords)[3].set(textureCoordMax,0.0f);
    geom->setTexCoordArray(0,tcoords);
    
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4));

    return geom;
}


//--------------------------------------------------------------------------
// Event handler to react on user input
//--------------------------------------------------------------------------
class KeyboardEventHandler : public osgGA::GUIEventHandler
{
public:

    bool handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter&)
    {
        switch(ea.getEventType())
        {
            case(osgGA::GUIEventAdapter::KEYDOWN):
            case(osgGA::GUIEventAdapter::KEYUP):
            {

                if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F1)
                {
                    gUniform->set(1);
                    gQuad->getOrCreateStateSet()->setTextureAttributeAndModes(0, gVideoTexture);
                    printf("F1: Show original video\n");
                }else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F2)
                {
                    gUniform->set(0);
                    gQuad->getOrCreateStateSet()->setTextureAttributeAndModes(0, gUnitTexture);
                    printf("F2: Show processed video\n");
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
int main(int , char **)
{
    // construct the viewer.
    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer();

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
    gQuad = new osg::Geode();
    gQuad->addDrawable(createSquare());
    node->addChild(gQuad);

    // load video file
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile("Data/Images/video.avi");
    osg::ImageStream* videoStream = dynamic_cast<osg::ImageStream*>(image.get());
    if (videoStream)
    {
        videoStream->setLoopingMode(osg::ImageStream::LOOPING);
        videoStream->play();
    }

    // setup texture which will hold the video file
    gVideoTexture = new osg::Texture2D();
    gVideoTexture->setResizeNonPowerOfTwoHint(false);
    gVideoTexture->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
    gVideoTexture->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);
    gVideoTexture->setWrap(osg::Texture2D::WRAP_S,osg::Texture2D::CLAMP_TO_EDGE);
    gVideoTexture->setWrap(osg::Texture2D::WRAP_T,osg::Texture2D::CLAMP_TO_EDGE);
    gVideoTexture->setImage(image.get());
    gQuad->getOrCreateStateSet()->setTextureAttributeAndModes(0, gVideoTexture);

    // create osgPPU's units and processor
    osgPPU::Processor* processor = new osgPPU::Processor();
    osgPPU::UnitTexture* unitTexture = new osgPPU::UnitTexture(gVideoTexture);
    osgPPU::UnitInOut* unitInOut = new osgPPU::UnitInOut();

    // create a processing shader, this will process the given image and output to the output texture
    osg::Shader* sh = new osg::Shader(osg::Shader::FRAGMENT);
    sh->setShaderSource(shaderSrc);
    osgPPU::ShaderAttribute* videoShader = new osgPPU::ShaderAttribute();
    videoShader->add("inputTexture", osg::Uniform::SAMPLER_2D);
    videoShader->set("inputTexture", 0);
    videoShader->addShader(sh);

    // add shader program to the unit
    unitInOut->getOrCreateStateSet()->setAttributeAndModes(videoShader);

    // setup global variables
    gUniform = unitInOut->getOrCreateStateSet()->getOrCreateUniform("useOriginalOutput", osg::Uniform::INT);
    gUnitTexture = unitInOut->getOrCreateOutputTexture(0);

    // setup appropriate pipeline to perform video processing
    node->addChild(processor);
    processor->addChild(unitTexture);
    unitTexture->addChild(unitInOut);    
    
    // add model to viewer.
    viewer->setSceneData( node );

    // give some info to the console
    printf("video (Play the video found under Data/Images/video.avi)\n");
    printf("Keys:\n");
    printf("\tF1 - Show original input\n");
    printf("\tF2 - Show postprocessed video\n");

    // add a keyboard handler to react on user input
    viewer->addEventHandler(new KeyboardEventHandler());

    // run viewer                
    return viewer->run();
}
