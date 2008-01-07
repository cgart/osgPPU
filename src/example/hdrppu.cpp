#include <osgPPU/PostProcess.h>
#include <osgPPU/PostProcessUnit.h>
#include <osgPPU/PPUInOut.h>
#include <osgPPU/PPUText.h>


//---------------------------------------------------------------
// PPU setup for HDR Rendering
//
// The pipeline is build based on the following:
//     http://msdn2.microsoft.com/en-us/library/bb173484(VS.85).aspx
// 
//---------------------------------------------------------------
class HDRRendering
{
    public:
        float mMidGrey;
        float mHDRBlurSigma;
        float mHDRBlurRadius;
        float mGlareFactor;
        float mAdaptFactor;
        float mMinLuminance;
        float mMaxLuminance;
        
        // Setup default hdr values
        HDRRendering()
        {
            mMidGrey = 0.145;
            mHDRBlurSigma = 7.0;
            mHDRBlurRadius = 7.0;
            mGlareFactor = 2.0;
            mMinLuminance = 0.01;
            mMaxLuminance = 1.0;
            mAdaptFactor = 10.0;
        }
           
        // Create ppu pipeline for hdr rendering, hdr is applied on the specified camera
        osgPPU::PostProcess::FXPipeline createPipeline(osgPPU::PostProcess* parent)
        {
            // resulting pipeline
            osgPPU::PostProcess::FXPipeline pipeline;
            osg::Camera* camera = parent->getCamera();
            
            // first a simple bypass to get the data from somewhere
            // there must be a camera bypass already specified
            // You need this ppu to relay on it with following ppus
                osg::ref_ptr<osgPPU::PostProcessUnit> bypass = new osgPPU::PostProcessUnit(parent);
                bypass->setIndex(9);
                bypass->setName("HDRBypass");
                
                pipeline.push_back(bypass);
            #if 1

            // Now we need a ppu which do compute the luminance of the scene.
            // We need to compute luminance per pixel and current luminance
            // of all pixels. For the first case we simply bypass the incoming
            // data through a luminance shader, which do compute the luminance.
            // For the second case we use the concept of mipmaps and store the
            // resulting luminance in the last mipmap level. For more info about
            // this step take a look into the according shaders.
                // create now the ppu which do incorprate the both shaders 
                osg::ref_ptr<osgPPU::PostProcessUnit> luminance = new osgPPU::PostProcessUnitInOut(parent);
                luminance->setIndex(10);
                luminance->setName("ComputeLuminance");
                luminance->setUseMipmaps(true);
                {
                    // create shader which do compute luminance per pixel
                    osgPPU::Shader* lumShader = new osgPPU::Shader();
                    lumShader->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, "Data/luminance_fp.glsl"));
                    lumShader->setName("LuminanceShader");
                    
                    // create shader which do compute the scene's luminance in mipmap levels
                    osgPPU::Shader* lumShaderMipmap = new osgPPU::Shader();
                    lumShaderMipmap->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, "Data/luminance_mipmap_fp.glsl"));
                    lumShaderMipmap->setName("LuminanceShaderMipmap");
                    
                    // setup shader parameters
                    // samplers are bounded automagically
                    lumShaderMipmap->add("g_MipmapLevel", osg::Uniform::INT);
                    lumShaderMipmap->add("g_ViewportWidth", osg::Uniform::FLOAT);
                    lumShaderMipmap->add("g_ViewportHeight", osg::Uniform::FLOAT);
                    lumShaderMipmap->add("maxLuminance", osg::Uniform::FLOAT);
                    lumShaderMipmap->add("minLuminance", osg::Uniform::FLOAT);
                    lumShaderMipmap->add("adaptScaleFactor", osg::Uniform::FLOAT);
                    lumShaderMipmap->add("invFrameTime", osg::Uniform::FLOAT);
                    
                    // Setup texture sizes
                    // Actually this should be done by the ppu class, but this functionality can
                    // be removed with later versions.
                    lumShaderMipmap->set("g_ViewportWidth", (float)camera->getViewport()->width());
                    lumShaderMipmap->set("g_ViewportHeight", (float)camera->getViewport()->height());

                    // Set maximum and minimum representable luminance values
                    lumShaderMipmap->set("maxLuminance", mMaxLuminance);
                    lumShaderMipmap->set("minLuminance", mMinLuminance);

                    // Set scaling factor which decides how fast eye adapts to new luminance
                    lumShaderMipmap->set("adaptScaleFactor", mAdaptFactor);
                
                    // set both shaders
                    luminance->setShader(lumShader);
                    luminance->setMipmapShader(lumShaderMipmap);
                }
                pipeline.push_back(luminance);
            
            
            // Now we need to setup a ppu which do pass only bright values
            // This ppu has two inputs, one is the original hdr scene data
            // and the other is the compute luminance. The according shader
            // do perform simple tonemapping operation and decides then
            // which pixels are too bright, so that they can not be represented
            // correctly. This pixels are passed through and will be blurred
            // later to simulate hdr glare.
                osg::ref_ptr<osgPPU::PostProcessUnit> brightpass = new osgPPU::PostProcessUnitInOut(parent);
                brightpass->setName("Brightpass");
                brightpass->setIndex(20);
                {
                    // setup brightpass shader
                    osgPPU::Shader* brightpassSh = new osgPPU::Shader();                    
                    brightpassSh->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, "Data/brightpass_fp.glsl"));
                    brightpassSh->setName("BrightpassShader");
                    
                    brightpassSh->add("g_fMiddleGray", osg::Uniform::FLOAT);
                    brightpassSh->set("g_fMiddleGray", mMidGrey);

                    brightpassSh->add("hdrInput", osg::Uniform::SAMPLER_2D);
                    brightpassSh->add("lumInput", osg::Uniform::SAMPLER_2D);
                    brightpassSh->set("hdrInput", 0);
                    brightpassSh->set("lumInput", 1);

                    brightpass->setShader(brightpassSh);
                }

                // brightpass ppu does get two input textures, hence add them
                brightpass->addInputPPU(bypass.get(), true);
                brightpass->addInputPPU(luminance.get());

                // this is needed to map correct shader's uniform names to the input indices
                /*brightpass->bindInputTextureToUniform(0, "hdrInput");
                brightpass->bindInputTextureToUniform(1, "lumInput");
                */
                
                pipeline.push_back(brightpass);                  

            // Now we have got a texture with only to bright pixels.
            // To simulate hdr glare we have to blur this texture.
            // We do this by first downsampling the texture and
            // applying separated gauss filter afterwards.
                osgPPU::PostProcessUnitInResampleOut* resample = new osgPPU::PostProcessUnitInResampleOut(parent);
                resample->setIndex(30);
                resample->setName("Resample");
                resample->setFactor(0.5, 0.5);
                resample->addInputPPU(brightpass.get());

                pipeline.push_back(osg::ref_ptr<osgPPU::PostProcessUnit>(resample));

            // now we perform a gauss blur on the downsampled data
                osg::ref_ptr<osgPPU::PostProcessUnit> blurx = new osgPPU::PostProcessUnitInOut(parent);
                osg::ref_ptr<osgPPU::PostProcessUnit> blury = new osgPPU::PostProcessUnitInOut(parent);
                blurx->setName("BlurHorizontal");
                blury->setName("BlurVertical");
                blurx->setIndex(40);
                blury->setIndex(41);
                {
                    // read shaders from file
                    osg::Shader* vshader = osg::Shader::readShaderFile(osg::Shader::VERTEX, "Data/gauss_convolution_vp.glsl");
                    osg::Shader* fhshader = osg::Shader::readShaderFile(osg::Shader::FRAGMENT, "Data/gauss_convolution_1Dx_fp.glsl");
                    osg::Shader* fvshader = osg::Shader::readShaderFile(osg::Shader::FRAGMENT, "Data/gauss_convolution_1Dy_fp.glsl");

                    // setup horizontal blur shaders
                    osgPPU::Shader* gaussx = new osgPPU::Shader();
                    gaussx->addShader(vshader);
                    gaussx->addShader(fhshader);
                    gaussx->setName("BlurHorizontalShader");

                    gaussx->add("sigma", osg::Uniform::FLOAT);
                    gaussx->add("radius", osg::Uniform::FLOAT);

                    gaussx->set("sigma", mHDRBlurSigma);
                    gaussx->set("radius", mHDRBlurRadius);

                    blurx->setShader(gaussx);
                    
                    // setup vertical blur shaders
                    osgPPU::Shader* gaussy = new osgPPU::Shader();
                    gaussy->addShader(vshader);
                    gaussy->addShader(fvshader);
                    gaussy->setName("BlurVerticalShader");

                    gaussy->add("sigma", osg::Uniform::FLOAT);
                    gaussy->add("radius", osg::Uniform::FLOAT);

                    gaussy->set("sigma", mHDRBlurSigma);
                    gaussy->set("radius", mHDRBlurRadius);

                    blury->setShader(gaussy);                    
                }

                pipeline.push_back(blurx);
                pipeline.push_back(blury);
            #endif

            // And finally we add a ppu which do use all the computed results:
            //  hdr scene data, luminance and blurred bright pixels
            // to combine them together. This is done by applying tonemapping
            // operation on the hdr values and adding the blurred brightpassed
            // pixels with some glare factor on it.
            osg::ref_ptr<osgPPU::PostProcessUnit> hdr = new osgPPU::PostProcessUnitInOut(parent);
            hdr->setIndex(50);
            hdr->setName("HDR-Result");
            hdr->addInputPPU(blury.get());
            hdr->addInputPPU(bypass.get(), true);
            hdr->addInputPPU(luminance.get());
            {
                osgPPU::Shader* sh = new osgPPU::Shader();
                sh->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, "Data/tonemap_hdr_fp.glsl"));
                sh->setName("HDRResultShader");
                
                sh->add("fBlurFactor", osg::Uniform::FLOAT);
                sh->add("g_fMiddleGray", osg::Uniform::FLOAT);

                sh->set("fBlurFactor", mGlareFactor);
                sh->set("g_fMiddleGray", mMidGrey);

                sh->add("blurInput", osg::Uniform::SAMPLER_2D);
                sh->add("hdrInput", osg::Uniform::SAMPLER_2D);
                sh->add("lumInput", osg::Uniform::SAMPLER_2D);

                sh->set("blurInput", 0);
                sh->set("hdrInput", 1);
                sh->set("lumInput", 2);
                
                hdr->setShader(sh);
            }

            pipeline.push_back(hdr);
            
            // ok return now the pipeline
            return pipeline;
        }

        
};

