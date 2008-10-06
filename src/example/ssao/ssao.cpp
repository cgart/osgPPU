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
#include <osg/PolygonOffset>
#include <osg/CullFace>
#include <osg/Material>
#include <osg/PositionAttitudeTransform>
#include <osg/ArgumentParser>
#include <osg/Program>
#include <osg/Shader>
#include <osg/Texture2D>
#include <osg/Camera>
#include <osg/TexGenNode>

#include <osgDB/ReadFile>

#include <osgPPU/Processor.h>
#include <osgPPU/UnitInOut.h>
#include <osgPPU/UnitOut.h>
#include <osgPPU/UnitCameraAttachmentBypass.h>
#include <osgPPU/ShaderAttribute.h>



#include <iostream>

using namespace osg;


ref_ptr<Group> _create_scene()
{
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

  shape = new ShapeDrawable(new Box(Vec3(0.0f, -0.5f, 0.0f), 10, 0.1f, 10), hints.get());
  shape->setColor(Vec4(0.5f, 0.5f, 0.7f, 1.0f));
  geode_1->addDrawable(shape.get());

  shape = new ShapeDrawable(new Sphere(Vec3(0.0f, 0.2f, 0.0f), radius), hints.get());
  shape->setColor(Vec4(0.8f, 0.4f, 0.2f, 1.0f));
  geode_2->addDrawable(shape.get());

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
  /*ref_ptr<Material> matirial = new Material;
  matirial->setColorMode(Material::DIFFUSE);
  matirial->setAmbient(Material::FRONT_AND_BACK, Vec4(0, 0, 0, 1));
  matirial->setSpecular(Material::FRONT_AND_BACK, Vec4(1, 1, 1, 1));
  matirial->setShininess(Material::FRONT_AND_BACK, 64.0f);
  scene->getOrCreateStateSet()->setAttributeAndModes(matirial.get(), StateAttribute::ON);
    */

  return scene;
}



//-----------------------------------------------------------------------------------------
// Create RTT texture
//-----------------------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------------------
// Create render camera, which uses multiple rendering targets to render the scene.
// The data stored in the second output is a linear depth values.
//-----------------------------------------------------------------------------------------
osg::Camera* setupCamera(osg::Camera* camera, int w, int h)
{
    // create texture to render to
    osg::Texture* textureColor = createRenderTexture(w, h);
    osg::Texture* textureLinearDepth = createRenderTexture(w, h);

    // set up the background color and clear mask.
    camera->setClearColor(osg::Vec4(0.0f,0.0f,0.0f,0.0f));
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // set viewport
    camera->setViewport(new osg::Viewport(0,0,w,h));
    camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

    // tell the camera to use OpenGL frame buffer object where supported.
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

    // attach the texture and use it as the color buffer.
    camera->attach(osg::Camera::COLOR_BUFFER0, textureColor);
    camera->attach(osg::Camera::COLOR_BUFFER1, textureLinearDepth);
}


//-----------------------------------------------------------------------------------------
// Shader used to render the scene. The shader do output the things as a normal
// fixed function pipeline. However the second output contains linear depth values
//-----------------------------------------------------------------------------------------
osg::Program* createRenderShader()
{
    osg::ref_ptr<osgDB::ReaderWriter::Options> fragmentOptions = new osgDB::ReaderWriter::Options("fragment");
    osg::ref_ptr<osgDB::ReaderWriter::Options> vertexOptions = new osgDB::ReaderWriter::Options("vertex");

    osg::Shader* vshader = osgDB::readShaderFile("Data/glsl/ssao_renderscene_vp.glsl", vertexOptions.get());
    osg::Shader* fshader = osgDB::readShaderFile("Data/glsl/ssao_renderscene_fp.glsl", fragmentOptions.get());

    // setup program shaders
    osg::Program* program = new osg::Program();

    program->addShader(vshader);
    program->addShader(fshader);
    
    return program;
}

//-----------------------------------------------------------------------------------------
// Create osgPPU shader which implements the ssao technique
//-----------------------------------------------------------------------------------------
osgPPU::ShaderAttribute* createSSAOShader()
{
    osg::ref_ptr<osgDB::ReaderWriter::Options> fragmentOptions = new osgDB::ReaderWriter::Options("fragment");
    osg::ref_ptr<osgDB::ReaderWriter::Options> vertexOptions = new osgDB::ReaderWriter::Options("vertex");

    osg::Shader* vshader = osgDB::readShaderFile("Data/glsl/ssao_vp.glsl", vertexOptions.get());
    osg::Shader* fshader = osgDB::readShaderFile("Data/glsl/ssao_fp.glsl", fragmentOptions.get());
    
    // setup program shaders
    osgPPU::ShaderAttribute* program = new osgPPU::ShaderAttribute();

    program->addShader(vshader);
    program->addShader(fshader);
    
    return program;
}

//-----------------------------------------------------------------------------------------
// Create osgPPU setup for ssao technique
//-----------------------------------------------------------------------------------------
osgPPU::Processor* createPPUPipeline(osg::Camera* viewCamera, int windowWidth, int windowHeight)
{
    osgPPU::Processor* proc = new osgPPU::Processor;
    proc->setCamera(viewCamera);

    // setup two input units, which will bypass the both camera outputs into the pipeline
    osgPPU::UnitCameraAttachmentBypass* out1 = new osgPPU::UnitCameraAttachmentBypass;
    out1->setBufferComponent(osg::Camera::COLOR_BUFFER0);
    out1->setName("ColorBuffer0Input");

    osgPPU::UnitCameraAttachmentBypass* out2 = new osgPPU::UnitCameraAttachmentBypass;
    out2->setBufferComponent(osg::Camera::COLOR_BUFFER1);
    out2->setName("ColorBuffer1Input");

    proc->addChild(out1);
    proc->addChild(out2);

    // setup unit which will use the both input to compute the ssao
    osgPPU::UnitOut* unitOut = new osgPPU::UnitOut();
    unitOut->setName("SSAO-Result");
    unitOut->setInputTextureIndexForViewportReference(-1);
    unitOut->setViewport(new osg::Viewport(0,0,windowWidth, windowHeight));
    
    out1->addChild(unitOut);
    out2->addChild(unitOut);

    // setup shader
    osgPPU::ShaderAttribute* sh = createSSAOShader();
    sh->setName("SSAOShader");
    unitOut->getOrCreateStateSet()->setAttributeAndModes(sh);
    unitOut->setInputToUniform(out1, "tex0");
    unitOut->setInputToUniform(out2, "tex1");

    // setup random noise uniform data
    for (unsigned i=0; i < 32; i++)
    {
        float r = float(rand()) / float(RAND_MAX);
        osg::Uniform* fk3f = unitOut->getOrCreateStateSet()->getOrCreateUniform("rad", osg::Uniform::FLOAT);
        fk3f->set(r);
    }

    return proc;
}


//-----------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
    // use an ArgumentParser object to manage the program arguments.
    ArgumentParser arguments(&argc, argv);

    // set up the usage document, in case we need to print out how to use this program.
    arguments.getApplicationUsage()->setDescription(arguments.getApplicationName() + " exampel of demonstraing Screen-Space Ambient Occlusion within osgPPU");
    arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName());
    arguments.getApplicationUsage()->addCommandLineOption("-h or --help", "Display this information");
    arguments.getApplicationUsage()->addCommandLineOption("--window","Use a separate Window for render to texture.");
    arguments.getApplicationUsage()->addCommandLineOption("--width","Set the width of the render to texture (default 512)");
    arguments.getApplicationUsage()->addCommandLineOption("--height","Set the height of the render to texture (default 512)");

    // construct the viewer.
    osgViewer::Viewer viewer;
    unsigned int screenWidth;
    unsigned int screenHeight;
    osg::GraphicsContext::getWindowingSystemInterface()->getScreenResolution(osg::GraphicsContext::ScreenIdentifier(0), screenWidth, screenHeight);
    unsigned int windowWidth = 640;
    unsigned int windowHeight = 480;
    viewer.setUpViewInWindow((screenWidth-windowWidth)/2, (screenHeight-windowHeight)/2, windowWidth, windowHeight);
    //viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

    // if user request help write it out to cout.
    if (arguments.read("-h") || arguments.read("--help"))
    {
        arguments.getApplicationUsage()->write(std::cout);
        return 1;
    }
    
    unsigned tex_width = windowWidth;
    unsigned tex_height = windowHeight;
    while (arguments.read("--width", tex_width)) {}
    while (arguments.read("--height", tex_height)) {}
    
    // any option left unread are converted into errors to write out later.
    arguments.reportRemainingOptionsAsUnrecognized();

    // report any errors if they have occurred when parsing the program arguments.
    if (arguments.errors())
    {
      arguments.writeErrorMessages(std::cout);
      return 1;
    }

    ref_ptr<MatrixTransform> scene = new MatrixTransform;
    scene->setMatrix(osg::Matrix::rotate(osg::DegreesToRadians(125.0),1.0,0.0,0.0));

    ref_ptr<Group> subgraph = _create_scene();    
    if (!subgraph.valid()) return 1;

    ref_ptr<Program> renderProg = createRenderShader();
    if (!renderProg) return 1;
    subgraph->getOrCreateStateSet()->setAttributeAndModes(createRenderShader());

    scene->addChild(subgraph.get());
    
    ref_ptr<osg::Group> root= new osg::Group;
    root->addChild(scene.get());

    // setup SSAO osgPPu pipeline
    setupCamera(viewer.getCamera(), tex_width, tex_height);
    root->addChild(createPPUPipeline(viewer.getCamera(), windowWidth, windowHeight));

    viewer.setSceneData(root.get());

    return viewer.run();
}
