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

#include <osg/Camera>
#include <osg/TexGenNode>
#include <osg/Texture2D>

#include <osgPPU/Processor.h>
#include <osgPPU/UnitInOut.h>
#include <osgPPU/UnitOut.h>
#include <osgPPU/UnitCameraAttachmentBypass.h>
#include <osgPPU/UnitCamera.h>

#include <osg/Program>
#include <osg/Shader>

#include <iostream>

using namespace osg;

//------------------------------------------------------------------------------
// Create scene with some geometry and apply proper shader for needed objects
// return group node containing the scene, also return by reference geode, which should glow
//------------------------------------------------------------------------------
ref_ptr<Group> createScene(osg::Node*& glowedScene)
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
    toGlow->addChild(transform_3);
    glowedScene = toGlow;


    return scene;
}


//------------------------------------------------------------------------------
// Simple shader, which do render same color into the second MRT target if enabled
//------------------------------------------------------------------------------
const char* shaderSrc =
    "\n"
    "void main () {\n"
    "\n"
    "\n"
    "   gl_FragData[0] = gl_Color; \n"
    "   gl_FragData[1] = gl_Color; \n"
    "}\n";


//------------------------------------------------------------------------------
// Create camera resulting texture
//------------------------------------------------------------------------------
osg::Texture* createRenderTexture(int tex_width, int tex_height)
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
// Setup main camera and setup osgPPU
// Also setup a slave camera, which will render only the glowed scene
//------------------------------------------------------------------------------
osg::Group* setupGlow(osg::Camera* camera, osg::Node* glowedScene, unsigned tex_width, unsigned tex_height, unsigned windowWidth, unsigned windowHeight, osg::Camera::RenderTargetImplementation renderImplementation)
{
    osg::Group* group = new osg::Group;

    // create two textures which will hold the usual view and glowing mask
    osg::Texture* textureView = createRenderTexture(tex_width, tex_height);
    osg::Texture* textureGlow = createRenderTexture(tex_width, tex_height);

    // setup the main camera, which will render the usual scene
    camera->setViewport(new osg::Viewport(0,0,tex_width,tex_height));
    camera->attach(osg::Camera::COLOR_BUFFER0, textureView);
    camera->setRenderTargetImplementation(renderImplementation);

    // create and setup slave camera, which will render only the glowed scene
    osg::Camera* slaveCamera = new osg::Camera;
    {
        slaveCamera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        slaveCamera->setClearColor(osg::Vec4(0,0,0,0));
        slaveCamera->setViewport(camera->getViewport());
        slaveCamera->setReferenceFrame(osg::Transform::RELATIVE_RF);
        slaveCamera->setRenderOrder(osg::Camera::PRE_RENDER);
        slaveCamera->attach(osg::Camera::COLOR_BUFFER0, textureGlow);
        slaveCamera->setRenderTargetImplementation(renderImplementation);

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
    osgPPU::UnitCameraAttachmentBypass* unitCam2 = new osgPPU::UnitCameraAttachmentBypass();
    unitCam2->setBufferComponent(osg::Camera::COLOR_BUFFER0);
    unitCam2->setName("CameraBuffer1");
    unitSlaveCamera->addChild(unitCam2);

    processor->addChild(unitSlaveCamera);

    // setup a unit, which will render the output
    osgPPU::UnitOut* unitOut = new osgPPU::UnitOut;
    unitOut->setName("Output");
    unitOut->setInputTextureIndexForViewportReference(-1); // need this here to get viewport from camera
    unitOut->setViewport(new osg::Viewport(0,0, windowWidth, windowHeight) );
    unitCam2->addChild(unitOut);
    
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
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

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
