/**
* Implements the osgPPU pipeline to create a simple, really faked screen space ambient occlusion algorithm.
* The algorithm is based on http://mikepan.homeip.net/ssaowcn
**/

#include <osgPPU/Processor.h>
#include <osgPPU/UnitCameraAttachmentBypass.h>
#include <osgPPU/UnitInOut.h>
#include <osgPPU/UnitOut.h>
#include <osgPPU/ShaderAttribute.h>

namespace SimpleSSAO
{

//-----------------------------------------------------------------------------------------
// Default variables desfribing the algorithm parameters
//-----------------------------------------------------------------------------------------
float gBlurSigma = 40.0;
float gBlurRadius = 15.0;
float gIntensity = 0.5;

//-----------------------------------------------------------------------------------------
// Create RTT texture
//-----------------------------------------------------------------------------------------
osg::Texture* createRenderTexture(int tex_width, int tex_height, bool depth)
{
    // create simple 2D texture
    osg::Texture2D* texture2D = new osg::Texture2D;
    texture2D->setTextureSize(tex_width, tex_height);
    texture2D->setInternalFormat(GL_RGBA);
    texture2D->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
    texture2D->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);

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


//---------------------------------------------------------------------------------
// Setup pipeline
// There will be valid textures attached to the camera. 
// Also the camera will be attached to the processor.
// Set widht and height to define internal size of the pipeline textures.
//---------------------------------------------------------------------------------
osgPPU::Processor* createPipeline(int width, int height, osg::Camera* camera, osgPPU::Unit*& lastUnit, bool showOnlyAOMap = false)
{
    using namespace osgPPU;

    //---------------------------------------------------------------------------------
    // presetup the camera
    //---------------------------------------------------------------------------------
    // set up the background color, clear mask and viewport
    camera->setClearColor(osg::Vec4(0.1f,0.2f,0.3f,1.0f));
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->setViewport(new osg::Viewport(0,0,width,height));

    // tell the camera to use OpenGL frame buffer object where supported.
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

    // create texture to render to
    osg::Texture* textureColor = createRenderTexture(width, height, false);
    osg::Texture* textureDepth = createRenderTexture(width, height, true);

    // attach the texture and use it as the color buffer.
    camera->attach(osg::Camera::COLOR_BUFFER0, textureColor);
    camera->attach(osg::Camera::DEPTH_BUFFER, textureDepth);
    
    //---------------------------------------------------------------------------------
    // Setup processor and first two units, which will bypass the camera textures
    //---------------------------------------------------------------------------------
    // create processor
    Processor* processor = new Processor;
    processor->setCamera(camera);

    // first unit does get the color input from the camera into the pipeline
    UnitCameraAttachmentBypass* colorBypass = new UnitCameraAttachmentBypass;
    colorBypass->setBufferComponent(osg::Camera::COLOR_BUFFER0);
    processor->addChild(colorBypass);

    // create unit which will bypass the depth texture into the pipleine
    UnitCameraAttachmentBypass* depthBypass = new UnitCameraAttachmentBypass;
    depthBypass->setBufferComponent(osg::Camera::DEPTH_BUFFER);
    processor->addChild(depthBypass);

    
    //---------------------------------------------------------------------------------
    // Setup a unit, which will read the depth texture clamp it accordingly and
    // process the output to a color texture, so that the value can be used further
    //---------------------------------------------------------------------------------
    /*UnitInOut* processDepth = new UnitInOut;
    {
        // create a shader which will process the depth values
        osg::Shader* fpShader = new osg::Shader(osg::Shader::FRAGMENT, 
            "uniform sampler2D depthTexture;\n"\
            "void main() {\n"\
            "   float depth = texture2D(depthTexture, gl_TexCoord[0].xy).x;\n"\
            "   gl_FragData[0].xyzw = depth;\n"\
            "}\n"
        );

        // create shader attribute and setup one input texture
        ShaderAttribute* depthShader = new ShaderAttribute;
        depthShader->addShader(fpShader);
        depthShader->setName("DepthProcessShader");
        depthShader->add("depthTexture", osg::Uniform::SAMPLER_2D);
        depthShader->set("depthTexture", 0);
        
        // create the unit and attach the shader to it
        processDepth->getOrCreateStateSet()->setAttributeAndModes(depthShader);
        depthBypass->addChild(processDepth);
        processDepth->setName("ProcessDepth");
    }*/


    //---------------------------------------------------------------------------------
    // Create units which will apply gaussian blur on the input textures
    //---------------------------------------------------------------------------------
    UnitInOut* blurx = new UnitInOut();
    UnitInOut* blury = new UnitInOut();
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

        // connect the gaussian blur ppus
        depthBypass->addChild(blurx);
        blurx->addChild(blury);
    }

    //---------------------------------------------------------------------------------
    // Now we want to substract blurred from non-blurred depth and to compute the 
    // resulting AO image
    //---------------------------------------------------------------------------------
    UnitInOut* aoUnit = new UnitInOut;
    {
        osg::Shader* fpShader = new osg::Shader(osg::Shader::FRAGMENT);

        // create a shader which will process the depth values
        if (showOnlyAOMap == false)
        {
            fpShader->setShaderSource( 
                "uniform float intensity;\n"\
                "uniform sampler2D blurredDepthTexture;\n"\
                "uniform sampler2D originalDepthTexture;\n"\
                "uniform sampler2D colorTexture;\n"\
                "void main() {\n"\
                "   float blurred = texture2D(blurredDepthTexture, gl_TexCoord[0].xy).x;\n"\
                "   float original = texture2D(originalDepthTexture, gl_TexCoord[0].xy).x;\n"\
                "   vec4 color = texture2D(colorTexture, gl_TexCoord[0].xy);\n"\
                "   vec4 result = color - intensity * clamp((original - blurred), 0.0, 1.0);\n"\
                "   gl_FragData[0].xyzw = clamp(result, 0.0, 1.0);\n"\
                "}\n"
            );
        }else
        {
            fpShader->setShaderSource( 
                "uniform float intensity;\n"\
                "uniform sampler2D blurredDepthTexture;\n"\
                "uniform sampler2D originalDepthTexture;\n"\
                "uniform sampler2D colorTexture;\n"\
                "void main() {\n"\
                "   float blurred = texture2D(blurredDepthTexture, gl_TexCoord[0].xy).x;\n"\
                "   float original = texture2D(originalDepthTexture, gl_TexCoord[0].xy).x;\n"\
                "   vec4 color = texture2D(colorTexture, gl_TexCoord[0].xy);\n"\
                "   vec4 result = 1.0 - intensity * clamp((original - blurred), 0.0, 1.0);\n"\
                "   gl_FragData[0].xyzw = clamp(result, 0.0, 1.0);\n"\
                "}\n"
            );
        }

        // create shader attribute and setup one input texture
        ShaderAttribute* shader = new ShaderAttribute;
        shader->addShader(fpShader);
        shader->add("blurredDepthTexture", osg::Uniform::SAMPLER_2D);
        shader->set("blurredDepthTexture", 0);
        shader->add("originalDepthTexture", osg::Uniform::SAMPLER_2D);
        shader->set("originalDepthTexture", 1);
        shader->add("colorTexture", osg::Uniform::SAMPLER_2D);
        shader->set("colorTexture", 2);
        shader->add("intensity", osg::Uniform::FLOAT);
        shader->set("intensity", gIntensity);

        // create the unit and attach the shader to it
        aoUnit->getOrCreateStateSet()->setAttributeAndModes(shader);
        blury->addChild(aoUnit);
        depthBypass->addChild(aoUnit);
        colorBypass->addChild(aoUnit);
        aoUnit->setName("ComputeAO");
    }
    lastUnit = aoUnit;

    return processor;
}

}; // end namepsace

