/* OpenSceneGraph example, blur_sceneWrks.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*  THE SOFTWARE.
*
* This Code is primarily taken from the glow.cpp example from the osgPPU
*   site.  Thank you Art.
*/

#include <osgViewer/Viewer>

#include <osg/Projection>
#include <osg/Geometry>
#include <osg/Texture>
#include <osg/TexGen>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/PolygonOffset>
#include <osg/CullFace>
#include <osg/TextureCubeMap>
#include <osg/TexMat>
#include <osg/MatrixTransform>
#include <osg/Light>
#include <osg/LightSource>
#include <osg/PolygonOffset>
#include <osg/CullFace>
#include <osg/Material>
#include <osg/PositionAttitudeTransform>
#include <osg/ArgumentParser>
#include <osgDB/ReadFile>
#include <osg/Program>
#include <osg/Shader>
#include <osg/Camera>
#include <osg/TexGenNode>
#include <osg/Texture2D>
#include <osg/GLExtensions>
#include <osgViewer/Renderer>
#include <osgGA/TrackballManipulator>
#include <osgDB/WriteFile>
#include <osgViewer/ViewerEventHandlers>


#include <osgPPU/Processor.h>
#include <osgPPU/UnitInOut.h>
#include <osgPPU/UnitOut.h>
#include <osgPPU/UnitCameraAttachmentBypass.h>
#include <osgPPU/UnitCamera.h>
#include <osgPPU/ShaderAttribute.h>
#include <osgPPU/UnitTexture.h> 


#include <iostream>


// values taht control the amount of blur to be applied to a scene
// these values are applied through the use of the opengl shaders
//   implemented within the function () by the variables,
//   osgPPU::ShaderAttribute* gaussx and gaussy.
float gBlurSigma2 = 5.0;
float gBlurRadius2 = 10.0;


//--------------------------------------------------------------------------
// Event handler to react on user input
// CAn switch between a blurred scene and a non-blurred scene by using the
//   function keys, F1- blur, and F2- no blur 
// viewer - it is the viewer for the scene though unnecessary in this case
// m_oswBlur - the switch node used to enable/disable the blurring pipeline
// m_oswNoBlur - switch node used to enable/disable the non-blur pipeline
//--------------------------------------------------------------------------
class KeyboardEventHandler : public osgGA::GUIEventHandler
{
public:
    osg::ref_ptr<osgViewer::Viewer> viewer; // not needed
    osg::ref_ptr<osg::Switch> m_oswBlur;
    osg::ref_ptr<osg::Switch> m_oswNoBlur;

    KeyboardEventHandler(osg::ref_ptr<osgViewer::Viewer> &v, 
      osg::ref_ptr<osg::Switch> &p_oswBlur,
      osg::ref_ptr<osg::Switch> &p_oswNoBlur) : viewer(v), m_oswBlur(p_oswBlur),
      m_oswNoBlur(p_oswNoBlur)
    {
    } // ctor

    bool handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter&)
    {
        switch(ea.getEventType())
        {
            case(osgGA::GUIEventAdapter::KEYDOWN):
            case(osgGA::GUIEventAdapter::KEYUP):
            {
              switch (ea.getKey())
              {
                case osgGA::GUIEventAdapter::KEY_F1 :
                  m_oswBlur->setAllChildrenOn();
                  m_oswNoBlur->setAllChildrenOff();
                break;

                case osgGA::GUIEventAdapter::KEY_F2 :
                  m_oswBlur->setAllChildrenOff();
                  m_oswNoBlur->setAllChildrenOn();
                break;
              } // sw on ea
              break;
            }// case key up
            default:
                break;
        }// sw
        return false;
    }// handle
}; // class keyevent




//------------------------------------------------------------------------------
// Constructs a simple scene; this code is takend directly from the glow.cpp
//  example from the osgPPU site.  In this scene construction, it is very 
//  important that the element that will be made to glow is in its own
//  group, the glowedScene so that only subnodes of this group will actually
//  glow in the final scene.
// For this particular exmaple of blurring the entire scene, nothing is given
//  a glow effect, so glowedScene is ultimately ignored for now.
//
//------------------------------------------------------------------------------
osg::ref_ptr<osg::Group> createSceneWrks(osg::Node*& glowedScene)
{
  using namespace osg;

    ref_ptr<Group> scene = new Group;
    ref_ptr<Geode> geode_1 = new Geode;
    scene->addChild(geode_1.get());
    
    ref_ptr<Geode> geode_2 = new Geode;
    ref_ptr<MatrixTransform> transform_2 = new MatrixTransform;
    transform_2->addChild(geode_2.get());
    transform_2->setUpdateCallback(new osg::AnimationPathCallback(Vec3(0, 0, 0), Y_AXIS, inDegrees(45.0f)));
    scene->addChild(transform_2.get());
    
    ref_ptr<Geode> geode_3 = new Geode;
    ref_ptr<MatrixTransform> transform_3 = new MatrixTransform;
    transform_3->addChild(geode_3.get());
    transform_3->setUpdateCallback(new osg::AnimationPathCallback(Vec3(0, 0, 0), Y_AXIS, inDegrees(-22.5f)));
    scene->addChild(transform_3.get());
    
    const float radius = 0.8f;
    const float height = 1.0f;
    ref_ptr<TessellationHints> hints = new TessellationHints;
    hints->setDetailRatio(2.0f);
    ref_ptr<ShapeDrawable> shape;
    
    shape = new ShapeDrawable(new Box(Vec3(0.0f, -2.0f, 0.0f), 10, 0.1f, 10), hints.get());
    shape->setColor(Vec4(0.5f, 0.5f, 0.7f, 1.0f));
    geode_1->addDrawable(shape.get());
    
    
    shape = new ShapeDrawable(new Sphere(Vec3(-3.0f, 0.0f, 0.0f), radius), hints.get());
    shape->setColor(Vec4(0.6f, 0.8f, 0.8f, 1.0f));
    geode_2->addDrawable(shape.get());
    
    shape = new ShapeDrawable(new Box(Vec3(3.0f, 0.0f, 0.0f), 2 * radius), hints.get());
    shape->setColor(Vec4(0.4f, 0.9f, 0.3f, 1.0f));
    geode_2->addDrawable(shape.get());
    
    shape = new ShapeDrawable(new Cone(Vec3(0.0f, 0.0f, -3.0f), radius, height), hints.get());
    shape->setColor(Vec4(0.2f, 0.5f, 0.7f, 1.0f));
    geode_2->addDrawable(shape.get());
    
    shape = new ShapeDrawable(new Cylinder(Vec3(0.0f, 0.0f, 3.0f), radius, height), hints.get());
    shape->setColor(Vec4(1.0f, 0.3f, 0.3f, 1.0f));
    geode_2->addDrawable(shape.get());
    
    shape = new ShapeDrawable(new Box(Vec3(0.0f, 3.0f, 0.0f), 2, 0.1f, 2), hints.get());
    shape->setColor(Vec4(0.8f, 0.8f, 0.4f, 1.0f));
    geode_3->addDrawable(shape.get());

    // material attributes however, none determine the glow effect directly.
    //  the glow effect is determeined within the glsl code.
    ref_ptr<Material> matirial = new Material;
    matirial->setColorMode(Material::DIFFUSE);
    matirial->setAmbient(Material::FRONT_AND_BACK, Vec4(0, 0, 0, 1));
    matirial->setSpecular(Material::FRONT_AND_BACK, Vec4(1, 1, 1, 1));
    matirial->setShininess(Material::FRONT_AND_BACK, 64.0f);
    scene->getOrCreateStateSet()->setAttributeAndModes(matirial.get(), StateAttribute::ON);

    // create subnode, which will represented the glowed scene
    osg::Group* toGlow = new osg::Group;
    toGlow->getOrCreateStateSet()->setAttributeAndModes(matirial.get(), StateAttribute::ON);
    toGlow->addChild(transform_3);
    glowedScene = toGlow;


    return scene;
}



//------------------------------------------------------------------------------
// Simple shader, which do render same color into the second MRT target if enabled
// the code "*  2.0" makes an object glow more
// somehow, this shader takes or receives two inputs from two different sources
//   the 1st source is the original scene
//   the 2nd source is the original scene as well but labeled glowColor and 
//     the color vector obtained is multiplied by 2 to give a glow effect.
// somehow these two colors are combined into a final color and the glsl
//   variable, gl_FragColor, is set to the result giving a glow effect to the
//   effected "scene" and for the purposes of the glow example, the scene had
//   only ONE element in it: glowedScene which is a group housing one node 
//   called transform_3
//------------------------------------------------------------------------------
//
const char* shaderSrc2 =
    "\n"
    "uniform sampler2D view;\n"
    "uniform sampler2D glow;\n"
    "void main () {\n"
    "\n"
    "   vec4 viewColor = texture2D(view, gl_TexCoord[0].st);\n"
    "   vec4 glowColor = texture2D(glow, gl_TexCoord[0].st);\n"
    "   gl_FragColor = viewColor + glowColor * 2.0; \n"
    "}\n";


//------------------------------------------------------------------------------
// Construct a texture which is to be attached to a camera and that tecture
//   will become that camera's resulting texture.  The camera texture can then
//   be used as input to the osgPPU pipeline.
//------------------------------------------------------------------------------
osg::Texture* createRenderTextureWrks(int tex_width, int tex_height)
{
    // create simple 2D texture
    osg::Texture2D* texture2D = new osg::Texture2D;
    texture2D->setTextureSize(tex_width, tex_height);
    texture2D->setInternalFormat(GL_RGBA);
    texture2D->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
    texture2D->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);

    // setup float format
    texture2D->setInternalFormat(GL_RGBA16F_ARB);
    texture2D->setSourceFormat(GL_RGBA);
    texture2D->setSourceType(GL_FLOAT);

    return texture2D;
}

//------------------------------------------------------------------------------
// This method constructs an osgPPU pipeline that outputs a texture to the 
//   frame buffer object which is the scene that is not blurred.
//
// camera - the camera to which a texture is attached and then that texture is
//          used as input to the osgPPU pipeline to produce a non-blurred
//          scene
// glowedScene - not used
// tex_width, tex_height - not used
// windowWidth, windowHeight - used to designate how large of a textture to 
//   construct and attached to the camera, which is passed into this method
// renderImplementation - whether output goes to the frame buffer or something
//   else
// p_ppuProcessor - returns a reference to the processor unit so that one may
//   access the processor unit later in the program without searching for it
//   down the osg node tree
//
//------------------------------------------------------------------------------
osg::Group* ogrpDoNotBlurTheScene(osg::Camera* camera, osg::Node* glowedScene, 
  unsigned tex_width, unsigned tex_height, unsigned windowWidth, 
  unsigned windowHeight, osg::Camera::RenderTargetImplementation renderImplementation,
  osg::ref_ptr<osgPPU::Processor> &p_ppuProcessor)
{
    // this group will house the osgPPU pipeline units
    osg::Group* group = new osg::Group;
    group->setName("NoBlurringEffect");

    // create the texture which will hold the usual view or scene
    osg::Texture* textureView = createRenderTextureWrks(windowWidth, windowHeight);

    // setup the camera, which will render the usual scene into the above
    //   allocated texture
    camera->setViewport(new osg::Viewport(0,0,windowWidth,windowHeight));
    camera->attach(osg::Camera::COLOR_BUFFER0, textureView);
    camera->setRenderTargetImplementation(renderImplementation);

    // setup osgPPU pipeline processor, which will use the main camera
    //   this code allows teh processor to have access to the main camera's
    //   texture output but it does NOT actually get that texture and put
    //   it into the pipeline.  It only makes that texture output from the
    //   camera available for retrieval.
    osg::ref_ptr<osgPPU::Processor> processor = new osgPPU::Processor();
    processor->setName("ProcessorForNormalScene");
    processor->setCamera(camera);
    p_ppuProcessor = processor;

    // setup unit which will bring the output of the camera into the scene
    //   UnitCameraAttachmentBypass is the unit that actually gets the output
    //   texture of a camera and brings it into the pipeline so that that 
    //   texture may be further manipulated.  It must be a direct child of the
    //   processor unit to accomplish feat.
    osgPPU::UnitCameraAttachmentBypass* ucaByPass = new osgPPU::UnitCameraAttachmentBypass();
    ucaByPass->setBufferComponent(osg::Camera::COLOR_BUFFER0);
    ucaByPass->setName("mainCamOutputTexUCAB");
    processor->addChild(ucaByPass);

    // This unit of type UnitOut is required so that any data passed to it is
    //   output to the frame buffer object.  It must be the LAST unit of the
    //   pipeline.
    // A key point is the glsl (openGL Shader Language) line:
    //   "  gl_FragColor=texture2D(textureNameInShader,gl_TexCoord[0].st);\n" 
    //   this ine takes the incoming texture and causes it to be the the
    //   output texture without modifying that texture.  It construsts a 
    //   simple pass-through.
    // Notice that the output viewport is the SAME SIZE AS the viewport
    //   of the camera's viewport.
   osgPPU::UnitOut* unitOut2= new osgPPU::UnitOut(); 
   osgPPU::ShaderAttribute* shaderAttribute= new osgPPU::ShaderAttribute(); 
   { 
      osg::Shader* shader= new osg::Shader(osg::Shader::FRAGMENT); 
      const char* shaderSource= 
         "uniform sampler2D textureNameInShader;\n" 
         "void main()\n" 
         "{\n" 
         "  gl_FragColor=texture2D(textureNameInShader,gl_TexCoord[0].st);\n" 
         "}"; 
      shader->setShaderSource(shaderSource); 
      shaderAttribute->addShader(shader); 
      shaderAttribute->setName("nomShaderAttribute"); 
      shaderAttribute->add("textureNameInShader", osg::Uniform::SAMPLER_2D); 
      shaderAttribute->set("textureNameInShader", 0); 

      unitOut2->setName("finalOutputUnit"); 
      unitOut2->setViewport(new osg::Viewport(0,0, windowWidth, windowHeight) );
      unitOut2->getOrCreateStateSet()->setAttributeAndModes(shaderAttribute); 
   } 

   // this line sends the output of ucaByPass into unitOut2, the final 
   //  destination frame buffer object
   ucaByPass->addChild(unitOut2); 


   // return osgPPU pipeline as a group and put the processor in that group
   //  as a child.
   group->addChild(processor);

    return group;
}

//------------------------------------------------------------------------------
// Setup main camera and setup osgPPU
// Also setup a slave camera, which will render only the glowed scene
// Slave camera has a smaller viewport, because we do not need high resolution
// for objects which are just get blurred ;)
//------------------------------------------------------------------------------
osg::Group* blurWholeScene(osg::Camera* camera, osg::Node* glowedScene, 
  unsigned tex_width, unsigned tex_height, unsigned windowWidth, 
  unsigned windowHeight, osg::Camera::RenderTargetImplementation renderImplementation,
  osg::ref_ptr<osgPPU::Processor> &p_ppuProcessor)
{
    osg::Group* group = new osg::Group;
    group->setName("BlurringEffect");

    // construct the texture which will hold the camera's view within it
    osg::Texture* textureView = createRenderTextureWrks(windowWidth, windowHeight);

    // setup the main camera, which will render the usual scene
    // the camera will output it's view into the texture which is attached 
    //   to it.
    camera->setViewport(new osg::Viewport(0,0,windowWidth,windowHeight));
    camera->attach(osg::Camera::COLOR_BUFFER0, textureView);
    camera->setRenderTargetImplementation(renderImplementation);

    // setup osgPPU pipeline processor, which will use the camera passed into
    //  this function as its camera source
    osg::ref_ptr<osgPPU::Processor> processor = new osgPPU::Processor();
    processor->setName("ProcessorForBlurring");
    processor->setCamera(camera);
    p_ppuProcessor = processor;

    // setup unit which will bring the output of the camera into the scene
    //   UnitCameraAttachmentBypass is the unit that actually gets the output
    //   texture of a camera and brings it into the pipeline so that that 
    //   texture may be further manipulated.  It must be a direct child of the
    //   processor unit to accomplish feat.
    osgPPU::UnitCameraAttachmentBypass* ucaByPass = new osgPPU::UnitCameraAttachmentBypass();
    ucaByPass->setBufferComponent(osg::Camera::COLOR_BUFFER0);
    ucaByPass->setName("mainCamOutputTexUCAB");
    processor->addChild(ucaByPass);

    // removeUnit CAUSES runtime crash!!!  This is experimental code left in as a note
    //processor->removeUnit( processor->findUnit("ucaByPass") );
    //processor->addChild(ucaByPass);


    //---------------------------------------------------------------------------------
    // Create units which will apply gaussian blur on the input textures
    //---------------------------------------------------------------------------------
    osgPPU::UnitInOut* blurx = new osgPPU::UnitInOut();
    osgPPU::UnitInOut* blury = new osgPPU::UnitInOut();
    {
      // set name and indicies
      blurx->setName("BlurHorizontal");
      blury->setName("BlurVertical");

      // read shaders from file
      osg::ref_ptr<osgDB::ReaderWriter::Options> fragmentOptions = 
        new osgDB::ReaderWriter::Options("fragment");
      osg::ref_ptr<osgDB::ReaderWriter::Options> vertexOptions = 
        new osgDB::ReaderWriter::Options("vertex");
      osg::Shader* vshader = osgDB::readShaderFile(
        "Data/glsl/gauss_convolution_vp.glsl", vertexOptions.get());
      osg::Shader* fhshader = osgDB::readShaderFile(
        "Data/glsl/gauss_convolution_1Dx_fp.glsl", fragmentOptions.get());
      osg::Shader* fvshader = osgDB::readShaderFile(
        "Data/glsl/gauss_convolution_1Dy_fp.glsl", fragmentOptions.get());

	    if (!vshader || !fhshader || !fvshader)
	      printf("One of the shader files gauss_convolution_*.glsl wasn't "
          "found!\n");

      // setup horizontal blur shaders
      osgPPU::ShaderAttribute* gaussx = new osgPPU::ShaderAttribute();
      gaussx->addShader(vshader);
      gaussx->addShader(fhshader);
      gaussx->setName("BlurHorizontalShader");

      gaussx->add("sigma", osg::Uniform::FLOAT);
      gaussx->add("radius", osg::Uniform::FLOAT);
      gaussx->add("texUnit0", osg::Uniform::SAMPLER_2D);

      gaussx->set("sigma", gBlurSigma2);
      gaussx->set("radius", gBlurRadius2);
      gaussx->set("texUnit0", 0);

      blurx->getOrCreateStateSet()->setAttributeAndModes(gaussx);

      // setup vertical blur shaders
      osgPPU::ShaderAttribute* gaussy = new osgPPU::ShaderAttribute();
      gaussy->addShader(vshader);
      gaussy->addShader(fvshader);
      gaussy->setName("BlurVerticalShader");

      gaussy->add("sigma", osg::Uniform::FLOAT);
      gaussy->add("radius", osg::Uniform::FLOAT);
      gaussy->add("texUnit0", osg::Uniform::SAMPLER_2D);

      gaussy->set("sigma", gBlurSigma2);
      gaussy->set("radius", gBlurRadius2);
      gaussy->set("texUnit0", 0);

      blury->getOrCreateStateSet()->setAttributeAndModes(gaussy);
    }


    // This unit of type UnitOut is required so that any data passed to it is
    //   output to the frame buffer object.  It must be the LAST unit of the
    //   pipeline.
    // A key point is the glsl (openGL Shader Language) line:
    //   "  gl_FragColor=texture2D(textureNameInShader,gl_TexCoord[0].st);\n" 
    //   this ine takes the incoming texture and causes it to be the the
    //   output texture without modifying that texture.  It construsts a 
    //   simple pass-through.
    // Notice that the output viewport is the SAME SIZE AS the viewport
    //   of the camera's viewport.
   osgPPU::UnitOut* unitOut2= new osgPPU::UnitOut(); 
   osgPPU::ShaderAttribute* shaderAttribute= new osgPPU::ShaderAttribute(); 
   { 
      osg::Shader* shader= new osg::Shader(osg::Shader::FRAGMENT); 
      const char* shaderSource= 
         "uniform sampler2D textureNameInShader;\n" 
         "void main()\n" 
         "{\n" 
         "  gl_FragColor=texture2D(textureNameInShader,gl_TexCoord[0].st);\n" 
         "}"; 
      shader->setShaderSource(shaderSource); 
      shaderAttribute->addShader(shader); 
      shaderAttribute->setName("nomShaderAttribute"); 
      shaderAttribute->add("textureNameInShader", osg::Uniform::SAMPLER_2D); 
      shaderAttribute->set("textureNameInShader", 0); 

      unitOut2->setName("finalOutputUnit"); 
      unitOut2->setViewport(new osg::Viewport(0,0, windowWidth, windowHeight) );
      unitOut2->getOrCreateStateSet()->setAttributeAndModes(shaderAttribute); 
   } 


   // By putting these units together in this manner, the output of the 
   //   unit camera attachement bypass, which is a texture, becomes the
   //   input to the blurx unit.  Finally, the convoluted texture is sent
   //   to unitOut2 which puts it's output texture into the frame buffer
   //   object for final output ot the screen.
   ucaByPass->addChild(blurx);
   blurx->addChild(blury);
   blury->addChild(unitOut2);

   // this code is left here as a note
   // removeUnit causes run-time crash if used right after setting up pipeline 
   //processor->removeUnit( processor->findUnit("mainCamOutputTexUCAB") );
   //try{
   //bool b = processor->removeUnit( processor->findUnit("BypassWOBlurring") );
   //std::cout<< "running\n";
   //if (b) std::cout<< "removed unit\n";
   //}
   //catch(...)
   //{
   //  std::cout<< "error\n";
   //}

   // causes runtime crash!
   //bypassWithBlurring->removeChild(blurx);
   //blurx->removeChild(blury);
   //blury->removeChild(unitOut2);
   //bypassWithBlurring->addChild(unitOut2);

   //// trial; Does NOT Like removing a child!
   ////ucaByPass->removeChild(blurx);
   ////ucaByPass->addChild(unitOut2);

   // Activating or deactivating the output of the blury unit
   //  causes the output to be a black screen
   //blury->setActive(false);

   // return pipeline as a group
   group->addChild(processor);

    return group;
}


//------------------------------------------------------------------------------
// Main code
//------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    using namespace osg;

    // use an ArgumentParser object to manage the program arguments.
    ArgumentParser arguments(&argc, argv);

    // set up the usage document, in case we need to print out how to use this program.
    arguments.getApplicationUsage()->setDescription(
      arguments.getApplicationName() + 
      " is the example which demonstrates using of GL_ARB_shadow extension "
      " implemented in osg::Texture class");
    arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName());
    arguments.getApplicationUsage()->addCommandLineOption("-h or --help", 
      "F1 - show blurred scene; F2 - show non-blurred scene");


    // construct the viewer.
    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer();
    unsigned int screenWidth;
    unsigned int screenHeight;
    osg::GraphicsContext::getWindowingSystemInterface()->
      getScreenResolution(osg::GraphicsContext::ScreenIdentifier(0),screenWidth, 
      screenHeight);
    unsigned int windowWidth = 640;
    unsigned int windowHeight = 480;
    viewer->setUpViewInWindow((screenWidth-windowWidth)/2, 
      (screenHeight-windowHeight)/2, windowWidth, windowHeight);
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);

    // if user request help write it out to cout.
    if (arguments.read("-h") || arguments.read("--help"))
    {
        arguments.getApplicationUsage()->write(std::cout);
        return 1;
    }

    unsigned tex_width = 256;
    unsigned tex_height = 256;
    while (arguments.read("--width", tex_width)) {}
    while (arguments.read("--height", tex_height)) {}

    osg::Camera::RenderTargetImplementation renderImplementation = 
      osg::Camera::FRAME_BUFFER_OBJECT;


    // any option left unread are converted into errors to write out later.
    arguments.reportRemainingOptionsAsUnrecognized();

    // report any errors if they have occurred when parsing the program arguments.
    if (arguments.errors())
    {
      arguments.writeErrorMessages(std::cout);
      return 1;
    }

    // create main scene and a rotator to bring scene in default position
    ref_ptr<MatrixTransform> mainTransform = new MatrixTransform;
    mainTransform->setMatrix(osg::Matrix::rotate(
      osg::DegreesToRadians(125.0),1.0,0.0,0.0));

    // construct the main scene
    // for this example, glowedScene is ignored
    osg::Node* glowedScene = NULL;
    ref_ptr<Group> scene = createSceneWrks(glowedScene);


    // setup camera and glower
    // the glower is not used in this example
    // This is the pipeline that produces a blurred scene.  Pipeline
    //   is returned as a group though only one node is within the 
    //   group.
    // Then, upon return from blurWholeScene(), the scene node group
    //   is added to this blurWholeScene group as a child.
    osg::ref_ptr<osgPPU::Processor> m_ppuProcessor;
    osg::ref_ptr<osg::Group> group = blurWholeScene(viewer->getCamera(), 
      glowedScene, tex_width, tex_height, windowWidth, windowHeight, 
      renderImplementation, m_ppuProcessor);
    group->addChild(scene.get());


    // This is the switch node which is an OSG element and not an osgPPU
    //   element.  It can be used to turn off all nodes attached to 
    //   it as children. So I attach the "group" returned from 
    //   blurWholeScene() to this switch node so that I can turn off
    //   the blurring pipeline.
    osg::ref_ptr<osg::Switch> blurswitch = new osg::Switch();
    blurswitch->addChild(group);
    blurswitch->setAllChildrenOff();


    // m_ppuProcessor2 is the pipeline representing the Non-Blurred scene
    // noBlurScene returns a group that houses that pipeline
    osg::ref_ptr<osgPPU::Processor> m_ppuProcessor2;
    osg::ref_ptr<osg::Group> grp2 = ogrpDoNotBlurTheScene(viewer->getCamera(), 
      glowedScene, tex_width,       tex_height, windowWidth, windowHeight, 
      renderImplementation, m_ppuProcessor2);
    grp2->addChild(scene.get());

    // This switch is used to turn off and on the non-blurring pipeline
    osg::ref_ptr<osg::Switch> noblurswitch = new osg::Switch();
    noblurswitch->addChild(grp2);
    noblurswitch->setAllChildrenOn();


    // Finally, I take the two switches and add them as children to a final
    //  group node that will allow me to turn on one pipeline and turn off
    //  the other pipeline.
    // The real trick is that BOTH pipelines output to the SAME osgPPU Unit,
    //   unitOut2.  Since both pipelines output to the exact same ppu and 
    //   that ppu simply takes the input given it and sends that input to
    //   the frame buffer object as direct output, I have one texture being
    //   sent to unitOut2 and that texture is either blurred or not
    //   blurred.
    // By convention, I must set the switches to allow ONLY ONE pipeline
    //   to be active.
    osg::ref_ptr<osg::Group> finalGrp= new osg::Group();
    finalGrp->addChild(blurswitch.get());
    finalGrp->addChild(noblurswitch.get());

    // this transform makes the scene moveable and shows each object within the
    //  scene moving as expected.
    mainTransform->addChild(finalGrp.get());

    // keyboard handler; used to allow the user to interact with this application
    printf("Keys:\n");
    printf("\tF1 - Show Blurred Scene\n");
    printf("\tF2 - Show Non-Blurred Scene\n");
    viewer->addEventHandler(new KeyboardEventHandler(viewer, blurswitch, noblurswitch));

    // setup scene
    viewer->setSceneData(mainTransform.get());

    return viewer->run();
}

