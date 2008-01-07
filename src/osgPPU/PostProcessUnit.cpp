/***************************************************************************
 *   Copyright (c) 2008   Art Tevs                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 ***************************************************************************/

#include <osgPPU/PostProcessUnit.h>
#include <osgPPU/PostProcess.h>

#include <osg/Texture2D>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osg/Image>
#include <assert.h>

namespace osgPPU
{

//------------------------------------------------------------------------------
PostProcessUnit::PostProcessUnit()
{
    assert(false && "This constructor shouldn't be used!!!");
}

//------------------------------------------------------------------------------
PostProcessUnit::PostProcessUnit(PostProcess* parent)
{
    initialize(parent);
    setName("__Nameless_PPU_");
}


//------------------------------------------------------------------------------
void PostProcessUnit::initialize(PostProcess* parent)
{
    mParent = parent;
    mUserData = NULL;
    mInputTexIndexForViewportReference = 0;
    mbDirtyShader = false;

    // we do steup defaults
    setStartTime(0);
    setExpireTime(0);
    setStartBlendValue(1);
    setEndBlendValue(1);
    setActive(true);
    setOfflineMode(false);
    mCurrentBlendValue = 1.0;
	mbDirtyInputTextures = true;
	mbDirtyOutputTextures = true;
    mOutputInternalFormat = GL_RGBA16F_ARB;
    mUseInputPPUViewport = NULL;

    // add empty mrt=0 output texture 
    setOutputTexture(NULL, 0);
    
    // create blending color and func
    //mBlendColor = new osg::BlendColor();
    mBlendFunc = new osg::BlendFunc();  
    mBlendFunc->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    
    // create a quad geometry
    sScreenQuad = osg::createTexturedQuadGeometry(
            osg::Vec3(0,0,0),
            osg::Vec3(1,0,0),
            osg::Vec3(0,1,0),
            0.0f,0.0f, 1.0f,1.0f);
            
    // remove colors form geometry
    mScreenQuadColor = new osg::Vec4Array(1);
    (*mScreenQuadColor)[0].set(1.0f,1.0f,1.0,1.0f);
    sScreenQuad->setColorArray(NULL);
    sScreenQuad->setColorBinding(osg::Geometry::BIND_OFF);

    // assign empty state set
    sScreenQuad->setStateSet(new osg::StateSet());
    setBlendMode(false);

    // disable mipmapping per default
    mbUseMipmaps = false;
    mbUseMipmapShader = false;
    mbDirtyViewport = true;
        
    // initialze projection matrix
    sProjectionMatrix = osg::Matrix::ortho(0,1,0,1,0,1);
    
    // setup default modelview matrix
    sModelviewMatrix = osg::Matrixf::identity();
    
    // setup per default local state equal to the parent state 
    if (mParent.valid())
        sState.setState(mParent->getState());
    
    // setup uniform variable
    //mShaderMipmapLevelUniform = new osg::Uniform("g_MipmapLevel", 0.0f);
    //sScreenQuad->getOrCreateStateSet()->addUniform(mShaderMipmapLevelUniform.get());
}

//------------------------------------------------------------------------------
PostProcessUnit::PostProcessUnit(const PostProcessUnit& ppu, const osg::CopyOp& copyop) : 
    osg::Object(ppu, copyop),
    mFBO(ppu.mFBO),
    mInputTex(ppu.mInputTex),
    mOutputTex(ppu.mOutputTex),
    //mUniformMap(ppu.mUniformMap),
    //mUniforms(ppu.mUniforms),
    mShader(ppu.mShader),
    //mShaderMipmapLevelUniform(ppu.mShaderMipmapLevelUniform),
    mIndex(ppu.mIndex),
    sScreenQuad(ppu.sScreenQuad),
    sProjectionMatrix(ppu.sProjectionMatrix),
    sModelviewMatrix(ppu.sModelviewMatrix),
    sState(ppu.sState),
    mViewport(ppu.mViewport),
    mInputPPU(ppu.mInputPPU),
    mOutputPPU(ppu.mOutputPPU),
    mUseInputPPUViewport(ppu.mUseInputPPUViewport),
    mbUseMipmaps(ppu.mbUseMipmaps),
    mbUseMipmapShader(ppu.mbUseMipmapShader),
    mMipmapShader(ppu.mMipmapShader),
    mMipmapFBO(ppu.mMipmapFBO),
    mMipmapViewport(ppu.mMipmapViewport),
    mCamera(ppu.mCamera),
    mbDirtyViewport(ppu.mbDirtyViewport),
    mbDirtyInputTextures(ppu.mbDirtyInputTextures),
    mbDirtyOutputTextures(ppu.mbDirtyOutputTextures),
    mbDirtyShader(ppu.mbDirtyShader),
    mScreenQuadColor(ppu.mScreenQuadColor),
    mbOfflinePPU(ppu.mbOfflinePPU),
    mOutputInternalFormat(ppu.mOutputInternalFormat),
    mParent(ppu.mParent),
    mInputTexIndexForViewportReference(ppu.mInputTexIndexForViewportReference),
    mbActive(ppu.mbActive),
    mExpireTime(ppu.mExpireTime),
    mStartTime(ppu.mStartTime),
    mStartBlendValue(ppu.mStartBlendValue),
    mEndBlendValue(ppu.mEndBlendValue),
    mCurrentBlendValue(ppu.mCurrentBlendValue),
    mBlendFunc(ppu.mBlendFunc),
    mUserData(ppu.mUserData)
{
    
}

//------------------------------------------------------------------------------
PostProcessUnit::~PostProcessUnit()
{
}

//------------------------------------------------------------------------------
void PostProcessUnit::setState(osg::State* state)
{
    if (state != NULL)
        sState.setState(state);
}

//------------------------------------------------------------------------------
void PostProcessUnit::setRenderingPosAndSize(float left, float top, float right, float bottom)
{
    sProjectionMatrix = osg::Matrix::ortho2D(left, right, bottom, top);
}

//------------------------------------------------------------------------------
void PostProcessUnit::setInputTextureIndexForViewportReference(int index)
{
    if (index < 0)
    {
        mInputTexIndexForViewportReference = -1;
        return;
    }

    // get input texture 
    osg::Texture* tex = getInputTexture(index);

    // work only on valid input textures
    if (tex)
    {
        mInputTexIndexForViewportReference = index;
        
        // change viewport sizes
        mViewport->width() = tex->getTextureWidth();
        mViewport->height() = tex->getTextureHeight();

        // just notice that the viewport size is changed
        //mbDirtyViewport = true;
        noticeChangeViewport();
    }
}

//------------------------------------------------------------------------------
void PostProcessUnit::setInputTexture(osg::Texture* inTex, int inputIndex)
{
    if (inTex)
	{
        mInputTex[inputIndex] = inTex;
		mbDirtyInputTextures = true;
        noticeChangeInput();
	}
}

//------------------------------------------------------------------------------
void PostProcessUnit::bindInputTextureToUniform(int index, const std::string& name)
{
    osg::Texture* tex = getInputTexture(index);
    if (mShader.valid() && tex)
    {
        mShader->bind(name, tex, index);
    }
}

//------------------------------------------------------------------------------
/*void PostProcessUnit::setUniformList(const osg::StateSet::UniformList& list)
{
    mUniforms = list;
    osg::StateSet* ss = sScreenQuad->getOrCreateStateSet();
    ss->setUniformList(mUniforms);
    mbDirtyUniforms = false;    
}*/

//------------------------------------------------------------------------------
void PostProcessUnit::setOutputTexture(osg::Texture* outTex, int mrt)
{
    if (outTex)
        mOutputTex[mrt] = outTex;
    else
        mOutputTex[mrt] = osg::ref_ptr<osg::Texture>(NULL);

	mbDirtyOutputTextures = true;
}

//------------------------------------------------------------------------------
osg::Texture* PostProcessUnit::getOutputTexture(int mrt)
{
    std::map<int, osg::ref_ptr<osg::Texture> >::const_iterator it = mOutputTex.find(mrt);
    if (it == mOutputTex.end()) return NULL;
    return it->second.get();
}


//--------------------------------------------------------------------------
void PostProcessUnit::initializeBase()
{    
    // check if input PPU is specified
    for (int i=0; i < (int)mInputPPU.size(); i++)
    {
        if (mInputPPU[i] != NULL)
        {
            setInputTexture(mInputPPU[i]->getOutputTexture(0), i);
        }
    }
    //assignInputTexture();
    
    // update viewport if we have one bound
    if (mUseInputPPUViewport != NULL)
    {
        setViewport(mUseInputPPUViewport->getViewport());
    }

    // check if we have input reference size 
    setInputTextureIndexForViewportReference(getInputTextureIndexForViewportReference());

    // check if mipmapping is enabled, then enable mipmapping on output textures
    if (mbUseMipmaps) enableMipmapGeneration();    

    // mark all textures as dirty, so that they get reassigned later
    mbDirtyInputTextures = true;
    mbDirtyOutputTextures = true;
}

//--------------------------------------------------------------------------
void PostProcessUnit::enableMipmapGeneration()
{
    mbUseMipmaps = true;
    
    std::map<int, osg::ref_ptr<osg::Texture> >::iterator it = mOutputTex.begin();
    for (; it != mOutputTex.end(); it++)
    {
        // set texture if it is valid
        if (it->second.valid())
        {
            it->second->setUseHardwareMipMapGeneration(false);

			// get filter
			osg::Texture::FilterMode filter = it->second->getFilter(osg::Texture2D::MIN_FILTER);
			if (filter == osg::Texture2D::LINEAR)
	            it->second->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR_MIPMAP_NEAREST);
			else if (filter == osg::Texture2D::NEAREST)
	            it->second->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::NEAREST_MIPMAP_NEAREST);

            it->second->allocateMipmapLevels();
        }
    }

}


//--------------------------------------------------------------------------
void PostProcessUnit::apply(float dTime)
{
    // update time value 
    mTime += dTime;

    // if uniforms are dirty
    /*if (mbDirtyUniforms)
    {
        mUniforms.clear();

        // iterate through all uniforms
        UniformMap::const_iterator it = mUniformMap.begin();
        for (; it != mUniformMap.end(); it++)
        {
            // add new uniform into the map
            osg::Uniform* uniform = new osg::Uniform(it->second.c_str(), it->first);
            mUniforms[it->second] = osg::StateSet::RefUniformPair(uniform, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);         
        }

        // assign new uniform map to the stateset
        osg::StateSet* ss = sScreenQuad->getOrCreateStateSet();
        ss->setUniformList(mUniforms);

        // not dirty anymore
        mbDirtyUniforms = false;
    }*/


    // if viewport is dirty, so resetup it
    if (mbDirtyViewport)
    {
        // do only change the viewport if there is no reference input texture 
        if (getInputTextureIndexForViewportReference() < 0)
        {
            // update viewport if such one is specified
            if (mUseInputPPUViewport != NULL)
            {
                setViewport(mUseInputPPUViewport->getViewport());
            }
            noticeChangeViewport();
        }
        mbDirtyViewport = false;
    }

    // in case input or output textures changes
    //if (mbDirtyInputTextures || mbDirtyOutputTextures)
    //    assignShader();

	// check if input textures are dirty
	if (mbDirtyInputTextures)
    {
        // reassign them
        assignInputTexture();

        // if we have to check for size, hence do
        if (mInputTexIndexForViewportReference >= 0)
            setInputTextureIndexForViewportReference(mInputTexIndexForViewportReference);
    }

	// check if we have to recreate output textures
	if (mbDirtyOutputTextures) assignOutputTexture();

    // check if shader is dirty
    if (mbDirtyShader) assignShader();
    
    // call on apply method 
    noticeOnApply();

    // check if any valid input texture exists
    TextureMap::const_iterator it = mInputTex.begin();
    for (; it != mInputTex.end(); it++)
    {
        if (it->second.valid())
        {
            // apply local state if it differs from parent state 
            if (sState.getState()) sState.getState()->apply(sScreenQuad->getStateSet());
            
            // apply current opengl matrix
            glMatrixMode( GL_PROJECTION ); glLoadMatrixf(sProjectionMatrix.ptr());
            glMatrixMode( GL_MODELVIEW );  glLoadMatrixf(sModelviewMatrix.ptr());

            // call rendering method if apply was successfull
            if (applyBaseRenderParameters()) render();
    
            // generate mipmaps if such are required (for each output texture)
		    if (mbUseMipmaps)
		    {
			    std::map<int, osg::ref_ptr<osg::Texture> >::iterator it = mOutputTex.begin();
			    for (; it != mOutputTex.end(); it++)
			    {
				    generateMipmaps(it->second.get(), it->first);
			    }
		    }

            // this is enough to do this once, hence break here
            break;
        }
    }

    //  unbind the framebuffers and reset the opengl states to default
    if (mFBO.valid() && sState.getState())
    {
        // disable the fbo
        osg::FBOExtensions* fbo_ext = osg::FBOExtensions::instance(sState.getContextID(),true);
        assert(fbo_ext != NULL);
        fbo_ext->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        
        // set default texture unit to 0
        sState.getState()->setActiveTextureUnit(0); 
    }

    // notice we are done
    noticeFinishRendering();
}

//--------------------------------------------------------------------------
bool PostProcessUnit::applyBaseRenderParameters()
{
    // only update if active 
    if (!isActive()) return false;
    
    // the fx should be shown
    if (getStartTime() <= mTime)
    {
        // blending factor
        float factor = 1.0;
        
        // if we get 0 as expire time, so the factor is the start value 
        if (getExpireTime() < 0.0001)
            factor = 0;
        else
            factor = (mTime - getStartTime()) / (getExpireTime() - getStartTime());
        
        // compute blend value for the ppu
        float alpha = getStartBlendValue()*(1-factor) + factor*getEndBlendValue();
        if (alpha > 1.0 ) { alpha = 1.0; setExpireTime(0); setStartBlendValue(1.0); }
        if (alpha < 0.0 ) { alpha = 0.0; setExpireTime(0); setStartBlendValue(0.0); }
        
        // setup new alpha value 
        mCurrentBlendValue = alpha;
        
        // if alpha value is 0, then disable this ppu
        //if (mCurrentBlendValue < 0.0001) mbActive = false;

    }else
        return false;
    
    // do rendering
    return true;
}

//--------------------------------------------------------------------------
void PostProcessUnit::setBlendMode(bool enable)
{
    osg::StateSet* ss = sScreenQuad->getOrCreateStateSet();
    ss->setAttribute(mBlendFunc.get(), enable ? osg::StateAttribute::ON : osg::StateAttribute::OFF);
    ss->setMode(GL_BLEND, enable ? osg::StateAttribute::ON : osg::StateAttribute::OFF);
}

//--------------------------------------------------------------------------
bool PostProcessUnit::useBlendMode()
{
    osg::StateAttribute::GLModeValue mode = sScreenQuad->getOrCreateStateSet()->getMode(GL_BLEND);
    return (mode & osg::StateAttribute::ON) == osg::StateAttribute::ON;
}

//--------------------------------------------------------------------------
void PostProcessUnit::generateMipmaps(osg::Texture* output, int mrt)
{
    // if ppu doesn't use mipmapping so return 
    if (!mbUseMipmaps || output == NULL
         || !sState.getState()) return;
        
    // check if we need to generate hardware mipmaps
    if (!mbUseMipmapShader)
    {
        osg::FBOExtensions* fbo_ext = osg::FBOExtensions::instance(sState.getContextID(),true);
        sState.getState()->applyTextureAttribute(0, output);
        fbo_ext->glGenerateMipmapEXT(output->getTextureTarget());
        return;
    }
    
    //
    // ok we generate now mipmaps by hand with the shader 
    //
    // does mipmap shader exists
    if (!mMipmapShader.valid()) return;
    assert (dynamic_cast<osg::Texture2D*>(output) != NULL);
    
    // check if we have generated all the fbo's for each mipmap level
    int width = output->getTextureWidth();
    int height = output->getTextureHeight();
    int numLevels = 1 + (int)floor(log2(std::max(width, height)));
    
    // before we start generating of mipmaps we save some data 
    osg::ref_ptr<osg::FrameBufferObject> currentFBO = mFBO;
    osg::ref_ptr<osg::Viewport> currentViewport = mViewport;
    osg::ref_ptr<osg::Texture> currentInputTex = mInputTex[0];
    osg::ref_ptr<Shader> currentShader = mShader;

    // generate fbo for each mipmap level 
    if ((int)mMipmapFBO.size() != numLevels)
    {
        // generate mipmap levels
        mMipmapFBO.clear();
        mMipmapViewport.clear();
        for (int i=0; i < numLevels; i++)
        {
            // generate viewport 
            osg::Viewport* vp = new osg::Viewport();
            int w = std::max(1, (int)floor(float(width) / float(pow(2,i)) ));
            int h = std::max(1, (int)floor(float(height) / float(pow(2,i)) ));
            vp->setViewport(0,0, w, h);
            mMipmapViewport.push_back(osg::ref_ptr<osg::Viewport>(vp));
            
            // generate fbo and assign a mipmap level to it
            osg::FrameBufferObject* fbo = new osg::FrameBufferObject();
            fbo->setAttachment(GL_COLOR_ATTACHMENT0_EXT, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2D*>(output), i));
            mMipmapFBO.push_back(osg::ref_ptr<osg::FrameBufferObject>(fbo));
        }

        // reallocate mipmap levels
        enableMipmapGeneration();
    }

    // also we assign special shader which will generate mipmaps
    removeShader();
    mShader = mMipmapShader;
    assignShader();
        
    // now we assign input texture as our result texture, so we can generate mipmaps
    mInputTex[0] = output;
    assignInputTexture();
    
    // now we render the result in a loop to generate mipmaps 
    for (int i=1; i < numLevels; i++)
    {
        // set mipmap level
        //if (mShaderMipmapLevelUniform != NULL) 
        //    mShaderMipmapLevelUniform->set(float(i));
        if (mShader.valid()) mShader->set("g_MipmapLevel", i);

        // assign new viewport 
        mViewport = mMipmapViewport[i];
        mFBO = mMipmapFBO[i];

        // render the content
        render(i);
    }

    // restore current shader 
    removeShader();
    mShader = currentShader;
    assignShader();
    
    // restore current input texture 
    mInputTex[0] = currentInputTex;
    assignInputTexture();
    
    // restore old viewport and fbo 
    mViewport = currentViewport;
    mFBO = currentFBO;
}


//--------------------------------------------------------------------------
void PostProcessUnit::assignInputTexture()
{
    // here the textures will be applied
    osg::StateSet* ss = sScreenQuad->getOrCreateStateSet();

    // for all entries
    TextureMap::iterator it = mInputTex.begin();
    for (; it != mInputTex.end(); it++)
    {
        // set texture if it is valid
        if (it->second.valid())
        {
            ss->setTextureAttributeAndModes(it->first, it->second.get(), osg::StateAttribute::ON);

            // check if textures size is 0, then force an apply to load it
            if (it->second->getTextureWidth() == 0 || it->second->getTextureHeight() == 0)
            {
                // apply the texture once, so it gets loaded
                it->second->apply(*sState.getState());
                glBindTexture(it->second->getTextureTarget(), 0);
            }
        }
    }

	mbDirtyInputTextures = false;
}

//--------------------------------------------------------------------------
void PostProcessUnit::assignShader()
{
    osg::StateSet* ss = sScreenQuad->getOrCreateStateSet();
    
    // set shader if it is valid
    if (mShader.valid())
    {
        // enable shader 
        //ss->setAttributeAndModes(mShader.get(), osg::StateAttribute::ON);
        mShader->enable(ss);

        // notice about changes in the shader assignment
        noticeAssignShader();

        mbDirtyShader = false;
    }
}

//--------------------------------------------------------------------------
void PostProcessUnit::removeShader()
{
    osg::StateSet* ss = sScreenQuad->getOrCreateStateSet();
    
    // set shader if it is valid
    if (mShader.valid())
    {
        //ss->setAttributeAndModes(mShader.get(), osg::StateAttribute::OFF);
        mShader->disable(ss);
        noticeRemoveShader();
    }
}

//--------------------------------------------------------------------------
void PostProcessUnit::setViewport(osg::Viewport* vp)
{
    // if viewport is valid and we have to ignore new settings
    if (mViewport.valid() && getInputTextureIndexForViewportReference() >=0)
        return;

    // otherwise setup new viewport
    mViewport = new osg::Viewport(*vp);
    sScreenQuad->getOrCreateStateSet()->setAttribute(mViewport.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    mbDirtyViewport = true;
}

//--------------------------------------------------------------------------
void PostProcessUnit::setOutputInternalFormat(GLenum format)
{
    mOutputInternalFormat = format;
    
    // now generate output texture's and assign them to fbo 
    TextureMap::iterator it = mOutputTex.begin();
    for (;it != mOutputTex.end(); it++)
    {
        if (it->second.valid()){
            it->second->setInternalFormat(mOutputInternalFormat);
            it->second->setSourceFormat(createSourceTextureFormat(mOutputInternalFormat));
        }
    }

}

//------------------------------------------------------------------------------
void PostProcessUnit::init()
{
    // just copy input to the output
    mOutputTex = mInputTex;
}

//------------------------------------------------------------------------------
void PostProcessUnit::render(int mipmapLevel)
{
    // do nothing just copy the data
    mOutputTex = mInputTex;
}


//--------------------------------------------------------------------------
GLenum PostProcessUnit::createSourceTextureFormat(GLenum internalFormat)
{
    switch (internalFormat)
    {
        case GL_LUMINANCE32F_ARB:
        case GL_LUMINANCE16F_ARB: return GL_LUMINANCE;

        case GL_LUMINANCE_ALPHA32F_ARB:
        case GL_LUMINANCE_ALPHA16F_ARB: return GL_LUMINANCE_ALPHA;

        case GL_RGB32F_ARB:
        case GL_RGB16F_ARB: return GL_RGB;

        case GL_RGBA32F_ARB:
        case GL_RGBA16F_ARB: return GL_RGBA;

        case GL_LUMINANCE32UI_EXT:
        case GL_LUMINANCE32I_EXT:
        case GL_LUMINANCE16UI_EXT:
        case GL_LUMINANCE16I_EXT:
        case GL_LUMINANCE8UI_EXT:
        case GL_LUMINANCE8I_EXT: return GL_LUMINANCE_INTEGER_EXT;

        case GL_LUMINANCE_ALPHA32UI_EXT:
        case GL_LUMINANCE_ALPHA32I_EXT:
        case GL_LUMINANCE_ALPHA16UI_EXT:
        case GL_LUMINANCE_ALPHA16I_EXT:
        case GL_LUMINANCE_ALPHA8UI_EXT:
        case GL_LUMINANCE_ALPHA8I_EXT: return GL_LUMINANCE_ALPHA_INTEGER_EXT;

        case GL_RGB32UI_EXT:
        case GL_RGB32I_EXT:
        case GL_RGB16UI_EXT:
        case GL_RGB16I_EXT:
        case GL_RGB8UI_EXT:
        case GL_RGB8I_EXT: return GL_RGB_INTEGER_EXT;

        case GL_RGBA32UI_EXT:
        case GL_RGBA32I_EXT:
        case GL_RGBA16UI_EXT:
        case GL_RGBA16I_EXT:
        case GL_RGBA8UI_EXT:
        case GL_RGBA8I_EXT: return GL_RGBA_INTEGER_EXT;

        default: return internalFormat;
    }
}

}; // end namespace

