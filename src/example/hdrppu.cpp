#include <osgPPU/Processor.h>
#include <osgPPU/Unit.h>
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

        //osg::ref_ptr<osg::Texture> mAdaptedLuminanceTex;
        
        // Setup default hdr values
        HDRRendering()
        {
            mMidGrey = 0.45;
            mHDRBlurSigma = 7.0;
            mHDRBlurRadius = 7.0;
            mGlareFactor = 2.0;
            mMinLuminance = 0.2;
            mMaxLuminance = 1.0;
            mAdaptFactor = 0.01;
        }

        // generate a 1x1 texture to store adapted luminance value
        /*void createAdaptedLuminanceTexture()
        {        
            // create texture with size 1x1
            osg::Texture2D* tex = new osg::Texture2D();
            tex->setTextureSize(1,1);
            tex->setResizeNonPowerOfTwoHint(false);
            tex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP);
            tex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP);

            // this texture must be able to store float values
            tex->setInternalFormat(GL_LUMINANCE16F_ARB);
            tex->setSourceFormat(GL_FLOAT);

            // we do not need to filter it, hence just nearest is enough
            tex->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::NEAREST);
            tex->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::NEAREST);

            mAdaptedLuminanceTex = tex;
        }*/

        
        //------------------------------------------------------------------------
        osgPPU::Processor::Pipeline createHDRPipeline(osgPPU::Processor* parent)
        {
            // resulting pipeline
            osgPPU::Processor::Pipeline pipeline;
            osg::Camera* camera = parent->getCamera();
            
            // first a simple bypass to get the data from somewhere
            // there must be a camera bypass already specified
            // You need this ppu to relay on it with following ppus
                osg::ref_ptr<osgPPU::Unit> bypass = new osgPPU::Unit(parent);
                bypass->setIndex(9);
                bypass->setName("HDRBypass");
                
                pipeline.push_back(bypass);
            #if 1

            // Now we have got a texture with only to bright pixels.
            // To simulate hdr glare we have to blur this texture.
            // We do this by first downsampling the texture and
            // applying separated gauss filter afterwards.
            osgPPU::UnitInResampleOut* resample = new osgPPU::UnitInResampleOut(parent);
            {
                resample->setIndex(10);
                resample->setName("Resample");
                resample->setFactor(0.5, 0.5);
                resample->addInputPPU(bypass.get());
            }
            pipeline.push_back(osg::ref_ptr<osgPPU::Unit>(resample));
            
            // Now we need a ppu which do compute the luminance of the scene.
            // We need to compute luminance per pixel and current luminance
            // of all pixels. For the first case we simply bypass the incoming
            // data through a luminance shader, which do compute the luminance.
            // For the second case we use the concept of mipmaps and store the
            // resulting luminance in the last mipmap level. For more info about
            // this step take a look into the according shaders.
            osg::ref_ptr<osgPPU::Unit> luminance = new osgPPU::UnitInOut(parent);
            luminance->setIndex(15);
            luminance->setName("ComputeLuminance");
            luminance->setUseMipmaps(true);
            {
                // create shader which do compute luminance per pixel
                osgPPU::Shader* lumShader = new osgPPU::Shader();
                lumShader->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, "Data/luminance_fp.glsl"));
                lumShader->setName("LuminanceShader");
                lumShader->add("texUnit0", osg::Uniform::SAMPLER_2D);
                lumShader->set("texUnit0", 0);

                // create shader which do compute the scene's luminance in mipmap levels
                osgPPU::Shader* lumShaderMipmap = new osgPPU::Shader();
                lumShaderMipmap->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, "Data/luminance_mipmap_fp.glsl"));
                lumShaderMipmap->setName("LuminanceShaderMipmap");

                // setup input texture
                lumShaderMipmap->add("texUnit0", osg::Uniform::SAMPLER_2D);
                lumShaderMipmap->set("texUnit0", 0);

                // setup shader parameters
                // samplers are bounded automagically
                lumShaderMipmap->add("g_MipmapLevel", osg::Uniform::FLOAT);
                lumShaderMipmap->add("g_MipmapLevelNum", osg::Uniform::FLOAT);
                lumShaderMipmap->add("g_ViewportWidth", osg::Uniform::FLOAT);
                lumShaderMipmap->add("g_ViewportHeight", osg::Uniform::FLOAT);

                // Setup texture sizes
                // Actually this should be done by the ppu class, but this functionality can
                // be removed with later versions.
                lumShaderMipmap->set("g_ViewportWidth", (float)camera->getViewport()->width());
                lumShaderMipmap->set("g_ViewportHeight", (float)camera->getViewport()->height());

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
            osg::ref_ptr<osgPPU::Unit> brightpass = new osgPPU::UnitInOut(parent);
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
                brightpassSh->add("texAdaptedLuminance", osg::Uniform::SAMPLER_2D);
                brightpassSh->set("hdrInput", 0);
                brightpassSh->set("lumInput", 1);
                brightpassSh->set("texAdaptedLuminance", 2);

                brightpass->setShader(brightpassSh);

                // brightpass ppu does get two input textures, hence add them
                brightpass->addInputPPU(resample, true);
                brightpass->addInputPPU(luminance.get());
            }
            pipeline.push_back(brightpass);


            // now we perform a gauss blur on the downsampled data
            osg::ref_ptr<osgPPU::Unit> blurx = new osgPPU::UnitInOut(parent);
            osg::ref_ptr<osgPPU::Unit> blury = new osgPPU::UnitInOut(parent);
            {
                // set name and indicies
                blurx->setName("BlurHorizontal");
                blury->setName("BlurVertical");
                blurx->setIndex(40);
                blury->setIndex(41);
                
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
                gaussx->add("g_ViewportWidth", osg::Uniform::FLOAT);
                gaussx->add("g_ViewportHeight", osg::Uniform::FLOAT);
                gaussx->add("texUnit0", osg::Uniform::SAMPLER_2D);

                gaussx->set("sigma", mHDRBlurSigma);
                gaussx->set("radius", mHDRBlurRadius);
                gaussx->set("texUnit0", 0);
                gaussx->set("g_ViewportWidth", (float)camera->getViewport()->width());
                gaussx->set("g_ViewportHeight", (float)camera->getViewport()->height());

                blurx->setShader(gaussx);
                
                // setup vertical blur shaders
                osgPPU::Shader* gaussy = new osgPPU::Shader();
                gaussy->addShader(vshader);
                gaussy->addShader(fvshader);
                gaussy->setName("BlurVerticalShader");

                gaussy->add("sigma", osg::Uniform::FLOAT);
                gaussy->add("radius", osg::Uniform::FLOAT);
                gaussy->add("g_ViewportWidth", osg::Uniform::FLOAT);
                gaussy->add("g_ViewportHeight", osg::Uniform::FLOAT);
                gaussy->add("texUnit0", osg::Uniform::SAMPLER_2D);
                
                gaussy->set("sigma", mHDRBlurSigma);
                gaussy->set("radius", mHDRBlurRadius);
                gaussy->set("texUnit0", 0);
                gaussy->set("g_ViewportWidth", (float)camera->getViewport()->width());
                gaussy->set("g_ViewportHeight", (float)camera->getViewport()->height());

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
            osg::ref_ptr<osgPPU::Unit> hdr = new osgPPU::UnitInOut(parent);
            {
                // setup inputs, name and index
                hdr->setIndex(50);
                hdr->setName("HDR-Result");
                hdr->addInputPPU(blury.get());
                hdr->addInputPPU(bypass.get(), true);
                hdr->addInputPPU(luminance.get());

                // setup shader
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
                sh->add("texAdaptedLuminance", osg::Uniform::SAMPLER_2D);

                sh->set("blurInput", 0);
                sh->set("hdrInput", 1);
                sh->set("lumInput", 2);
                sh->set("texAdaptedLuminance", 3);
                
                hdr->setShader(sh);
            }
            pipeline.push_back(hdr);
            
            // ok return now the pipeline
            return pipeline;
        }

        //------------------------------------------------------------------------
        void setupPPUsToComputeAdaptedLuminance(osgPPU::Processor* parent)
        {
            // create a texture which will hold the adapted luminance data
            //createAdaptedLuminanceTexture();
        
            // Create a simple offline ppu which do
            // compute the adapted luminance value.
            // The ppu does use the input of the previous frame and do recompute it.
            // The ppu is choosen to be offline to show how to setup offline ppus.
            osg::ref_ptr<osgPPU::Unit> adaptedlum = new osgPPU::UnitInOut(parent);
            {
                adaptedlum->setName("AdaptedLuminance");
                
                // offline ppus do not really need an index, because we setup the inputs manually
                adaptedlum->setIndex(0);

                // set ppu into offline mode
                adaptedlum->setOfflineMode(true);

                // create shader which do compute the adapted luminance value
                osgPPU::Shader* adaptedShader = new osgPPU::Shader();
                adaptedShader->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, "Data/luminance_adapted_fp.glsl"));
                adaptedShader->setName("AdaptLuminanceShader");

                // shader do also need the adapted luminance as input
                adaptedShader->add("texAdaptedLuminance", osg::Uniform::SAMPLER_2D);
                adaptedShader->set("texAdaptedLuminance", 0);
                
                // setup computed current luminance  input texture
                adaptedShader->add("texLuminance", osg::Uniform::SAMPLER_2D);
                adaptedShader->set("texLuminance", 1);

                // setup shader parameters
                adaptedShader->add("maxLuminance", osg::Uniform::FLOAT);
                adaptedShader->add("minLuminance", osg::Uniform::FLOAT);
                adaptedShader->add("adaptScaleFactor", osg::Uniform::FLOAT);
                adaptedShader->add("invFrameTime", osg::Uniform::FLOAT);
                
                // Set maximum and minimum representable luminance values
                adaptedShader->set("maxLuminance", mMaxLuminance);
                adaptedShader->set("minLuminance", mMinLuminance);

                // Set scaling factor which decides how fast eye adapts to new luminance
                adaptedShader->set("adaptScaleFactor", mAdaptFactor);
            
                // set both shaders
                adaptedlum->setShader(adaptedShader);

                // we do just want to have this size
                adaptedlum->setViewport(new osg::Viewport(0,0,1,1));

                // first input is the output of the luminance ppu
                // the second input is currently just NULL
                adaptedlum->setInputTexture(NULL, 0);
                adaptedlum->setInputTexture(parent->getPPU("ComputeLuminance")->getOutputTexture(0), 1);

                // we do not want to have any referenced viewport
                adaptedlum->setInputTextureIndexForViewportReference(-1);
                
                // we have to initilize by ourself
                adaptedlum->init();
            }
            parent->addPPUToPipeline(adaptedlum.get());
                
            // The adapted luminance ppu do compute it. However if you
            // can follow me for now, you maybe encounter, that this ppu do
            // have to write into the same texture as it also read from.
            // To prevent this, we do just generate an inout ppu which do
            // nothing than render the copy of input to the output.
            // We will use the output of this ppu as input for the
            // adapted luminance ppu. In this way we do not write to the
            // same texture as we have readed from.
            osg::ref_ptr<osgPPU::Unit> adaptedlumCopy = new osgPPU::UnitInOut(parent);
            {
                adaptedlumCopy->setName("AdaptedLuminanceCopy");
                adaptedlumCopy->setIndex(1);
                adaptedlumCopy->setOfflineMode(true);
                adaptedlumCopy->setViewport(new osg::Viewport(0,0,1,1));
                adaptedlumCopy->setInputTextureIndexForViewportReference(-1);
                adaptedlumCopy->setInputTexture(NULL, 1);

                adaptedlumCopy->init();

                /*osgPPU::Shader* shader = new osgPPU::Shader();
                shader->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, "Data/bypass_fp.glsl"));
                shader->add("texUnit0", osg::Uniform::SAMPLER_2D);
                shader->set("texUnit0", 0);
                adaptedlumCopy->setShader(shader);*/
            }
            parent->addPPUToPipeline(adaptedlumCopy.get());

            // do connect both ppus, so that the output of the second
            // is the input for the first.
            // And the output of the first is the input to the second.
            adaptedlumCopy->setInputTexture(adaptedlum->getOutputTexture(0), 0);
            adaptedlum->setInputTexture(adaptedlumCopy->getOutputTexture(0), 0);
            
            // And finally, since we have added this ppus to the pipeline, we
            // have to connect the output of the copy ppu as the input
            // to the ppus which require information about the adapted luminance.
            // This are Brightpass and Tonemapping.
            parent->getPPU("Brightpass")->setInputTexture(adaptedlumCopy->getOutputTexture(0), 2);
            parent->getPPU("HDR-Result")->setInputTexture(adaptedlumCopy->getOutputTexture(0), 3);
        }
};

