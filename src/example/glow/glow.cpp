/* OpenSceneGraph example, osgprerendercubemap.
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
#include <osg/CullFace>
#include <osg/Material>
#include <osg/PositionAttitudeTransform>
#include <osg/ArgumentParser>

#include <osg/Camera>
#include <osg/TexGenNode>
#include <osg/Texture2D>

#include <osgPPU/Processor.h>
#include <osgPPU/UnitInOut.h>
#include <osgPPU/UnitOut.h>
#include <osgPPU/UnitCameraAttachmentBypass.h>
#include <osgPPU/UnitCamera.h>
#include <osgPPU/ShaderAttribute.h>
#include <osgPPU/UnitDepthbufferBypass.h>

#include <osgDB/ReadFile>
#include <osg/Program>
#include <osg/Shader>

#include <iostream>


float gBlurSigma = 3.0;
float gBlurRadius = 7.0;
float g_NearPlane = 0.01;
float g_FarPlane = 50.0;

//------------------------------------------------------------------------------
// Create scene with some geometry and apply proper shader for needed objects
// return group node containing the scene, also return by reference geode, which should glow
//------------------------------------------------------------------------------
osg::ref_ptr<osg::Group> createScene(osg::Node*& glowedScene)
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

    // material
    ref_ptr<Material> matirial = new Material;
    matirial->setColorMode(Material::DIFFUSE);
    matirial->setAmbient(Material::FRONT_AND_BACK, Vec4(0, 0, 0, 1));
    matirial->setSpecular(Material::FRONT_AND_BACK, Vec4(1, 1, 1, 1));
    matirial->setShininess(Material::FRONT_AND_BACK, 64.0f);
    scene->getOrCreateStateSet()->setAttributeAndModes(matirial.get(), StateAttribute::ON);

    // create subnode, which will represented the glowed scene
    osg::Group* toGlow = new osg::Group;
    toGlow->getOrCreateStateSet()->setAttributeAndModes(matirial.get(), StateAttribute::ON);
    toGlow->addChild(transform_3.get());
    glowedScene = toGlow;


    return scene;
}


//------------------------------------------------------------------------------
// Shader which performs depth test when rendering an object.
// This shader is needed in order to render the glowed objects using the depth values of
// the main camera.
// NOTE:
//   Because the main camera renders the glowed object too, there could be Z-fighting artifacts
//   between the glowed object and the main scene.
//   In order prevent them, one can either play around with PolygonOffset parameters or render
//   the main scene's depth buffer in two passes, once with the glowed object and once without.
//------------------------------------------------------------------------------
const char* depthTestShaderSrc =
    "\n"
    "uniform sampler2D depthBuffer;\n"
    "uniform vec2 invViewportSize;\n"
    "uniform float nearPlane;\n"
    "uniform float farPlane;\n"
    "\n"
    "void main () {\n"
    "\n"
    "   // get color and depth values of the rendered object\n"
    "   vec4 color = gl_Color;\n"
    "\n"
    "   // get depth value of the scene (the depth values are linearized by the hardware)\n"
    "   vec2 texCoord = gl_FragCoord.xy * invViewportSize; \n"
    "   float depthScene = texture2D(depthBuffer, texCoord).x;\n"
    "\n"
    "   // we need to linearize the depth value, in order to do depth test \n"
    "   float depthPixel = gl_FragCoord.z;\n"
    "\n"    
    "   // depth test, render only if ok\n"
    "   if (depthPixel < depthScene)\n"
    "       gl_FragColor = color; \n"
    "   else \n"
    "       gl_FragColor = vec4(0.0,0.0,0.0,0.0); \n"
    "}\n";

//------------------------------------------------------------------------------
// Simple shader which just combines two textures
//------------------------------------------------------------------------------
const char* shaderSrc =
    "\n"
    "uniform sampler2D view;\n"
    "uniform sampler2D glow;\n"
    "void main () {\n"
    "\n"
    "   // get color values of the view and of the glow texture\n"
    "   vec4 viewColor = texture2D(view, gl_TexCoord[0].st);\n"
    "   vec4 glowColor = texture2D(glow, gl_TexCoord[0].st);\n"
    "\n"
    "   // just the view\n"
     "   gl_FragColor = viewColor + glowColor * 2.0; \n"
    "}\n";


//------------------------------------------------------------------------------
// Create camera resulting texture
//------------------------------------------------------------------------------
osg::Texture* createRenderTexture(int tex_width, int tex_height, bool depth = false, bool nearest = false)
{
    // create simple 2D texture
    osg::Texture2D* texture2D = new osg::Texture2D;
    texture2D->setTextureSize(tex_width, tex_height);
    texture2D->setFilter(osg::Texture2D::MIN_FILTER, nearest ? osg::Texture2D::NEAREST : osg::Texture2D::LINEAR);
    texture2D->setFilter(osg::Texture2D::MAG_FILTER, nearest ? osg::Texture2D::NEAREST : osg::Texture2D::LINEAR);
    texture2D->setWrap(osg::Texture2D::WRAP_S,osg::Texture2D::CLAMP_TO_EDGE);
    texture2D->setWrap(osg::Texture2D::WRAP_T,osg::Texture2D::CLAMP_TO_EDGE);
    texture2D->setBorderColor(osg::Vec4(0.0f,0.0f,0.0f,0.0f));
    texture2D->setResizeNonPowerOfTwoHint(false);

    // setup float format
    if (!depth)
    {
        texture2D->setInternalFormat(GL_RGBA16F_ARB);
        texture2D->setSourceFormat(GL_RGBA);
        texture2D->setSourceType(GL_FLOAT);
    }else{
        texture2D->setInternalFormat(GL_DEPTH_COMPONENT);
        texture2D->setSourceFormat(GL_DEPTH_COMPONENT);
    }

    return texture2D;
}

//------------------------------------------------------------------------------
// Setup main camera and setup osgPPU
// Also setup a slave camera, which will render only the glowed scene
// Slave camera has a smaller viewport, because we do not need high resolution
// for objects which are just get blurred ;)
//------------------------------------------------------------------------------
osg::Group* setupGlow(osg::Camera* camera, osg::Node* glowedScene, unsigned tex_width, unsigned tex_height, unsigned windowWidth, unsigned windowHeight, osg::Camera::RenderTargetImplementation renderImplementation)
{
    osg::Group* group = new osg::Group;

    // create two textures which will hold the usual view and glowing mask
    osg::Texture* textureView = createRenderTexture(windowWidth, windowHeight);
    osg::Texture* textureDepthView = createRenderTexture(windowWidth, windowHeight, true, true);

    osg::Texture* textureGlow = createRenderTexture(tex_width, tex_height);
    
    // setup the main camera, which will render the usual scene
    camera->setViewport(new osg::Viewport(0,0,windowWidth,windowHeight));
    camera->attach(osg::Camera::COLOR_BUFFER0, textureView);
    camera->attach(osg::Camera::DEPTH_BUFFER, textureDepthView);
    camera->setRenderTargetImplementation(renderImplementation);

    // we have to disable the automatic calculation, because otherwise the depth values
    // of the glowing object and the normal scene could be different
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    camera->setProjectionMatrixAsPerspective(30.0, float(windowWidth)/float(windowHeight), g_NearPlane, g_FarPlane);

    // create and setup slave camera, which will render only the glowed scene
    osg::Camera* slaveCamera = new osg::Camera;
    {
        slaveCamera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        slaveCamera->setClearColor(osg::Vec4(0,0,0,0));
        slaveCamera->setViewport(new osg::Viewport(0,0,tex_width,tex_height));
        slaveCamera->setReferenceFrame(osg::Transform::RELATIVE_RF);
        slaveCamera->setRenderOrder(osg::Camera::PRE_RENDER);
        slaveCamera->attach(osg::Camera::COLOR_BUFFER0, textureGlow);
        slaveCamera->setRenderTargetImplementation(renderImplementation);
        slaveCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

        // setup shader on the glowed scene, which will do the depth test for us
        {
          // create fragment shader for depth test
          osg::Shader* fpShader = new osg::Shader(osg::Shader::FRAGMENT);
          fpShader->setShaderSource(depthTestShaderSrc);

          // create program to hold the shader and add to the glowed scene
          osg::Program* program = new osg::Program();
          program->addShader(fpShader);
          glowedScene->getOrCreateStateSet()->setAttributeAndModes(program);
          
          // create uniform to specify the sampler parameter, so the texture unit, where the depth buffer is bound
          glowedScene->getOrCreateStateSet()->getOrCreateUniform("depthBuffer", osg::Uniform::SAMPLER_2D)->set(1);
          glowedScene->getOrCreateStateSet()->getOrCreateUniform("invViewportSize", osg::Uniform::FLOAT_VEC2)->set(osg::Vec2(1.0 / tex_width, 1.0 / tex_height));
          glowedScene->getOrCreateStateSet()->getOrCreateUniform("nearPlane", osg::Uniform::FLOAT)->set(g_NearPlane);
          glowedScene->getOrCreateStateSet()->getOrCreateUniform("farPlane", osg::Uniform::FLOAT)->set(g_FarPlane);

          // bind the depth buffer texture to the glowed scene (so it will be used in depth test shader)
          glowedScene->getOrCreateStateSet()->setTextureAttributeAndModes(1, textureDepthView);
        }

        // we need to add small polygon offset, in order to prevent z-fighting, because the glowed object is also rendered in the normal scene
        osg::PolygonOffset* offset = new osg::PolygonOffset(-10.0, -10.0);
        slaveCamera->getOrCreateStateSet()->setAttributeAndModes(offset);
        slaveCamera->addChild(glowedScene);
    }

    // setup osgPPU pipeline processor, which will use the main camera
    osgPPU::Processor* processor = new osgPPU::Processor();
    processor->setCamera(camera);

    // setup unit which will bring the color output of the first camera into the pipeline
    osgPPU::UnitCameraAttachmentBypass* unitCam1 = new osgPPU::UnitCameraAttachmentBypass();
    unitCam1->setBufferComponent(osg::Camera::COLOR_BUFFER0);
    unitCam1->setName("CameraBuffer0");
    processor->addChild(unitCam1);
    
    // setup unit, which will bring the camera and its output into the pipeline
    osgPPU::UnitCamera* unitSlaveCamera = new osgPPU::UnitCamera;
    unitSlaveCamera->setCamera(slaveCamera);
    processor->addChild(unitSlaveCamera);

    // bypass color buffer from the slave camera unit
    osgPPU::UnitCameraAttachmentBypass* unitCam2 = new osgPPU::UnitCameraAttachmentBypass();
    unitCam2->setBufferComponent(osg::Camera::COLOR_BUFFER0);
    unitCam2->setName("CameraBuffer1");
    unitSlaveCamera->addChild(unitCam2);



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
        osg::ref_ptr<osgDB::ReaderWriter::Options> fragmentOptions = new osgDB::ReaderWriter::Options("fragment");
        osg::ref_ptr<osgDB::ReaderWriter::Options> vertexOptions = new osgDB::ReaderWriter::Options("vertex");
        osg::Shader* vshader = osgDB::readShaderFile("Data/glsl/gauss_convolution_vp.glsl", vertexOptions.get());
        osg::Shader* fhshader = osgDB::readShaderFile("Data/glsl/gauss_convolution_1Dx_fp.glsl", fragmentOptions.get());
        osg::Shader* fvshader = osgDB::readShaderFile("Data/glsl/gauss_convolution_1Dy_fp.glsl", fragmentOptions.get());

        if (!vshader || !fhshader || !fvshader)
        {
	        printf("One of the shader files gauss_convolution_*.glsl wasn't found!\n");
        }

        // setup horizontal blur shaders
        osgPPU::ShaderAttribute* gaussx = new osgPPU::ShaderAttribute();
        gaussx->addShader(vshader);
        gaussx->addShader(fhshader);
        gaussx->setName("BlurHorizontalShader");

        gaussx->add("sigma", osg::Uniform::FLOAT);
        gaussx->add("radius", osg::Uniform::FLOAT);
        gaussx->add("texUnit0", osg::Uniform::SAMPLER_2D);

        gaussx->set("sigma", gBlurSigma);
        gaussx->set("radius", gBlurRadius);
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

        gaussy->set("sigma", gBlurSigma);
        gaussy->set("radius", gBlurRadius);
        gaussy->set("texUnit0", 0);

        blury->getOrCreateStateSet()->setAttributeAndModes(gaussy);

        // connect the gaussian blur to the slave camera
        unitCam2->addChild(blurx);
        blurx->addChild(blury);
    }

    // create shader which will just combine both input textures
    osgPPU::ShaderAttribute* resultShader = new osgPPU::ShaderAttribute();
    {
      osg::Shader* fpShader = new osg::Shader(osg::Shader::FRAGMENT);
      fpShader->setShaderSource(shaderSrc);

      resultShader->addShader(fpShader);

      resultShader->add("view", osg::Uniform::SAMPLER_2D);
      resultShader->add("glow", osg::Uniform::SAMPLER_2D);
      resultShader->set("view", 0);
      resultShader->set("glow", 1);
    }

    // setup a unit, which will render the output
    osgPPU::UnitOut* unitOut = new osgPPU::UnitOut;
    unitOut->setName("Output");
    unitOut->setInputTextureIndexForViewportReference(-1); // need this here to get viewport from camera
    unitOut->setViewport(new osg::Viewport(0,0, windowWidth, windowHeight) );
    unitOut->getOrCreateStateSet()->setAttributeAndModes(resultShader);

    // add both camera outputs and the depth valued output to the main unit
    unitCam1->addChild(unitOut);
    blury->addChild(unitOut);

    // add scnee and processor to the resulting group and return it back
    group->addChild(slaveCamera);
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
    arguments.getApplicationUsage()->setDescription(arguments.getApplicationName() + " is the example which demonstrates using of GL_ARB_shadow extension implemented in osg::Texture class");
    arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName());
    arguments.getApplicationUsage()->addCommandLineOption("-h or --help", "Display this information");
    arguments.getApplicationUsage()->addCommandLineOption("--fbo","Use Frame Buffer Object for render to texture, where supported. [default]");
    arguments.getApplicationUsage()->addCommandLineOption("--fb","Use FrameBuffer for render to texture.");
    arguments.getApplicationUsage()->addCommandLineOption("--pbuffer","Use Pixel Buffer for render to texture, where supported.");
    arguments.getApplicationUsage()->addCommandLineOption("--window","Use a separate Window for render to texture.");
    arguments.getApplicationUsage()->addCommandLineOption("--width","Set the width of the render to texture");
    arguments.getApplicationUsage()->addCommandLineOption("--height","Set the height of the render to texture");

    // construct the viewer.
    osgViewer::Viewer viewer;
    unsigned int screenWidth;
    unsigned int screenHeight;
    osg::GraphicsContext::getWindowingSystemInterface()->getScreenResolution(osg::GraphicsContext::ScreenIdentifier(0), screenWidth, screenHeight);
    unsigned int windowWidth = 640;
    unsigned int windowHeight = 480;
    viewer.setUpViewInWindow((screenWidth-windowWidth)/2, (screenHeight-windowHeight)/2, windowWidth, windowHeight);
    osgViewer::GraphicsWindow* window = dynamic_cast<osgViewer::GraphicsWindow*>(viewer.getCamera()->getGraphicsContext());
    if (window) window->setWindowName("Particular object glow fx");
    //viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);

    // if user request help write it out to cout.
    if (arguments.read("-h") || arguments.read("--help"))
    {
        arguments.getApplicationUsage()->write(std::cout);
        return 1;
    }

    unsigned tex_width = windowWidth / 2;
    unsigned tex_height = windowHeight / 2;
    while (arguments.read("--width", tex_width)) {}
    while (arguments.read("--height", tex_height)) {}

    osg::Camera::RenderTargetImplementation renderImplementation = osg::Camera::FRAME_BUFFER_OBJECT;

    while (arguments.read("--fbo")) { renderImplementation = osg::Camera::FRAME_BUFFER_OBJECT; }
    while (arguments.read("--pbuffer")) { renderImplementation = osg::Camera::PIXEL_BUFFER; }
    while (arguments.read("--fb")) { renderImplementation = osg::Camera::FRAME_BUFFER; }
    while (arguments.read("--window")) { renderImplementation = osg::Camera::SEPERATE_WINDOW; }


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
    mainTransform->setMatrix(osg::Matrix::rotate(osg::DegreesToRadians(125.0),1.0,0.0,0.0));
    osg::Node* glowedScene = NULL;
    ref_ptr<Group> scene = createScene(glowedScene);
    //mainTransform->addChild(scene.get());


    // setup camera and glower
    osg::Group* group = setupGlow(viewer.getCamera(), glowedScene, tex_width, tex_height, windowWidth, windowHeight, renderImplementation);
    group->addChild(scene.get());
    mainTransform->addChild(group);

    // setup scene
    viewer.setSceneData(mainTransform.get());

    return viewer.run();
}
