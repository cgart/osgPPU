/**
* Implements the osgPPU pipeline to create a simple, image diffusion filter.
* The filter will be applied several iterations long
* see http://www.cs.technion.ac.il/~ron/PAPERS/spira_sochen_kimmel.pdf
**/

#include <osgPPU/Processor.h>
#include <osgPPU/UnitCameraAttachmentBypass.h>
#include <osgPPU/UnitInOut.h>
#include <osgPPU/UnitOut.h>
#include <osgPPU/UnitTexture.h>
#include <osgPPU/UnitBypassRepeat.h>
#include <osgPPU/ShaderAttribute.h>

namespace Diffusion
{

//-----------------------------------------------------------------------------------------
// Default variables desfribing the algorithm parameters
//-----------------------------------------------------------------------------------------
float gIntensity = 1.0;
float gdT = 0.2;
int gNumiterations = 10;


//---------------------------------------------------------------------------------
// Setup pipeline
// There will be valid textures attached to the camera. 
// Also the camera will be attached to the processor.
// Set widht and height to define internal size of the pipeline textures.
//---------------------------------------------------------------------------------
osgPPU::Processor* createPipeline(osgPPU::UnitBypassRepeat*& repeatUnit, osgPPU::Unit*& lastUnit, osg::Uniform*& dt, osg::Uniform*& intensity)
{
    using namespace osgPPU;

    //---------------------------------------------------------------------------------
    // Setup processor, don't use any camera input, because we are working only on images
    //---------------------------------------------------------------------------------
    // create processor
    Processor* processor = new Processor;


    //---------------------------------------------------------------------------------
    // Place a texture unit which will put a texture into the pipeline
    //---------------------------------------------------------------------------------
    UnitTexture* colorBypass = new UnitTexture();
    {
        osg::Image* img = osgDB::readImageFile("Data/Images/lenna.png");
        if (img == NULL)
        {
            osg::notify(osg::FATAL) << "File Data/Images/lenna.png not found!" << std::endl;
            exit(1);
        }
        osg::Texture2D* tex = new osg::Texture2D();
        tex->setImage(img);
        colorBypass->setTexture(tex);
        colorBypass->setName("InputImage");
        processor->addChild(colorBypass);
    }

    //---------------------------------------------------------------------------------
    // Next unit is the iterative unit, which run the computation for certain amount of iterations
    //---------------------------------------------------------------------------------
    UnitBypassRepeat* repeat = new UnitBypassRepeat;
    repeatUnit = repeat;
    {
        colorBypass->addChild(repeat);
        repeat->setName("Iterate");
        repeat->setNumIterations(gNumiterations);
    }


    //---------------------------------------------------------------------------------
    // Create unit which will compute second derivative of the input image
    //---------------------------------------------------------------------------------
    UnitInOut* derivativeX = new UnitInOut();
    UnitInOut* derivativeY = new UnitInOut();
    {
        // set name and indicies
        derivativeX->setName("DerivativeX");
        derivativeY->setName("DerivativeY");

        {
            osg::Shader* fpShader = new osg::Shader(osg::Shader::FRAGMENT);
            fpShader->setShaderSource( 
                "uniform sampler2D colorTexture;\n"\
                "uniform float osgppu_ViewportWidth;\n"\
                "uniform float osgppu_ViewportHeight;\n"\
                "void main() {\n"\
                "   vec2 step = 1.0 / vec2(osgppu_ViewportWidth, osgppu_ViewportHeight);\n"\
                "\n"\
                "   vec3 xy = texture2D(colorTexture, gl_TexCoord[0].xy).xyz;\n"\
                "   vec3 px = texture2D(colorTexture, gl_TexCoord[0].xy + vec2(step.x, 0.0)).xyz;\n"\
                "   vec3 nx = texture2D(colorTexture, gl_TexCoord[0].xy - vec2(step.x, 0.0)).xyz;\n"\
                "   vec3 pxpy = texture2D(colorTexture, gl_TexCoord[0].xy + vec2(step.x, step.y)).xyz;\n"\
                "   vec3 pxny = texture2D(colorTexture, gl_TexCoord[0].xy + vec2(step.x, -step.y)).xyz;\n"\
                "   vec3 nxpy = texture2D(colorTexture, gl_TexCoord[0].xy + vec2(-step.x, step.y)).xyz;\n"\
                "   vec3 nxny = texture2D(colorTexture, gl_TexCoord[0].xy + vec2(-step.x, -step.y)).xyz;\n"\
                "\n"\
                "   vec3 dx  = (px - nx) * 0.5;\n"\
                "   vec3 dxx = px - 2.0 * xy + nx;\n"\
                "   vec3 dxy = (pxpy - pxny - nxpy + nxny) * 0.25;\n"\
                "\n"\
                "   gl_FragData[0].xyz = dx;\n"\
                "   gl_FragData[1].xyz = dxx;\n"\
                "   gl_FragData[2].xyz = dxy;\n"\
                "}\n"
            );


            ShaderAttribute* shader = new ShaderAttribute;
            shader->addShader(fpShader);
            shader->add("colorTexture", osg::Uniform::SAMPLER_2D);
            shader->set("colorTexture", 0);
            
            derivativeX->getOrCreateStateSet()->setAttributeAndModes(shader);
            derivativeX->setOutputDepth(3);
        }

        {
            osg::Shader* fpShader = new osg::Shader(osg::Shader::FRAGMENT);
            fpShader->setShaderSource( 
                "uniform sampler2D colorTexture;\n"\
                "uniform float osgppu_ViewportWidth;\n"\
                "uniform float osgppu_ViewportHeight;\n"\
                "void main() {\n"\
                "   vec2 step = 1.0 / vec2(osgppu_ViewportWidth, osgppu_ViewportHeight);\n"\
                "\n"\
                "   vec3 xy = texture2D(colorTexture, gl_TexCoord[0].xy).xyz;\n"\
                "   vec3 py = texture2D(colorTexture, gl_TexCoord[0].xy + vec2(0.0, step.y)).xyz;\n"\
                "   vec3 ny = texture2D(colorTexture, gl_TexCoord[0].xy - vec2(0.0, step.y)).xyz;\n"\
                "\n"\
                "   vec3 dy  = (py - ny) * 0.5;\n"\
                "   vec3 dyy = py - 2.0 * xy + ny;\n"\
                "\n"\
                "   gl_FragData[0].xyz = dy;\n"\
                "   gl_FragData[1].xyz = dyy;\n"\
                "}\n"
            );

            ShaderAttribute* shader = new ShaderAttribute;
            shader->addShader(fpShader);
            shader->add("colorTexture", osg::Uniform::SAMPLER_2D);
            shader->set("colorTexture", 0);
            
            derivativeY->getOrCreateStateSet()->setAttributeAndModes(shader);
            derivativeY->setOutputDepth(2);
        }
        repeat->addChild(derivativeX);
        repeat->addChild(derivativeY);
    }

    //---------------------------------------------------------------------------------
    // Now using both second derivatives and original image we compute simple laplace diffusion
    //---------------------------------------------------------------------------------
    UnitInOut* combine = new UnitInOut;
    {
        osg::Shader* fpShader = new osg::Shader(osg::Shader::FRAGMENT);

        // create a shader which will process the depth values
        {
            fpShader->setShaderSource( 
                "uniform sampler2D derivativeX;\n"\
                "uniform sampler2D derivativeXX;\n"\
                "uniform sampler2D derivativeXY;\n"\
                "uniform sampler2D derivativeY;\n"\
                "uniform sampler2D derivativeYY;\n"\
                "uniform sampler2D colorTexture;\n"\
                "uniform float dt;\n"\
                "void main() {\n"\
                "   vec3 dxx = texture2D(derivativeXX, gl_TexCoord[0].xy).xyz;\n"\
                "   vec3 dyy = texture2D(derivativeYY, gl_TexCoord[0].xy).xyz;\n"\
                "   vec3 dxy = texture2D(derivativeXY, gl_TexCoord[0].xy).xyz;\n"\
                "   vec3 dx  = texture2D(derivativeX, gl_TexCoord[0].xy).xyz;\n"\
                "   vec3 dy  = texture2D(derivativeY, gl_TexCoord[0].xy).xyz;\n"\
                "   vec3 xy  = texture2D(colorTexture, gl_TexCoord[0].xy).xyz;\n"\
                "   gl_FragData[0].xyz = xy + dt * ((1.0+dx*dx)*dyy - 2.0*dx*dy*dxy + (1.0 + dy*dy)*dxx) / (1.0 + dx*dx + dy*dy);\n"\
                "}\n"
            );
        }

        // create shader attribute and setup one input texture
        ShaderAttribute* shader = new ShaderAttribute;
        shader->addShader(fpShader);
        shader->add("derivativeX", osg::Uniform::SAMPLER_2D);
        shader->set("derivativeX", 0);
        shader->add("derivativeXX", osg::Uniform::SAMPLER_2D);
        shader->set("derivativeXX", 1);
        shader->add("derivativeXY", osg::Uniform::SAMPLER_2D);
        shader->set("derivativeXY", 2);
        shader->add("derivativeY", osg::Uniform::SAMPLER_2D);
        shader->set("derivativeY", 3);
        shader->add("derivativeYY", osg::Uniform::SAMPLER_2D);
        shader->set("derivativeYY", 4);
        shader->add("colorTexture", osg::Uniform::SAMPLER_2D);
        shader->set("colorTexture", 5);
        shader->add("dt", osg::Uniform::FLOAT);
        shader->set("dt", gdT);
        dt = shader->get("dt");

        // create the unit and attach the shader to it
        combine->getOrCreateStateSet()->setAttributeAndModes(shader);
        derivativeX->addChild(combine);
        derivativeY->addChild(combine);
        repeat->addChild(combine);
        combine->setName("Combine");
        repeat->setLastNode(combine);
    }

    //---------------------------------------------------------------------------------
    // Resulting unit, which combine original and filterd data
    //---------------------------------------------------------------------------------
    UnitInOut* result = new UnitInOut;
    {
        osg::Shader* fpShader = new osg::Shader(osg::Shader::FRAGMENT);

        // create a shader which will process the depth values
        {
            fpShader->setShaderSource( 
                "uniform sampler2D original;\n"\
                "uniform sampler2D filtered;\n"\
                "uniform float intensity;\n"\
                "void main() {\n"\
                "   vec3 old = texture2D(original, gl_TexCoord[0].xy).xyz;\n"\
                "   vec3 new = texture2D(filtered, gl_TexCoord[0].xy).xyz;\n"\
                "   gl_FragData[0].xyz = new * intensity;\n"\
                "}\n"
            );
        }

        // create shader attribute and setup one input texture
        ShaderAttribute* shader = new ShaderAttribute;
        shader->addShader(fpShader);
        shader->add("original", osg::Uniform::SAMPLER_2D);
        shader->set("original", 0);
        shader->add("filtered", osg::Uniform::SAMPLER_2D);
        shader->set("filtered", 1);
        shader->add("intensity", osg::Uniform::FLOAT);
        shader->set("intensity", gIntensity);
        intensity = shader->get("intensity");

        // create the unit and attach the shader to it
        result->getOrCreateStateSet()->setAttributeAndModes(shader);
        colorBypass->addChild(result);
        combine->addChild(result);
        result->setName("ResultImage");
    }
    lastUnit = result;


    return processor;
}

}; // end namepsace

