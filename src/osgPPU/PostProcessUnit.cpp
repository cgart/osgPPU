/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <osgPPU/PostProcessUnit.h>

#include <osg/Texture2D>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osg/Image>
#include <assert.h>

//------------------------------------------------------------------------------
PostProcessUnit::PostProcessUnit()
{
    initialize(new osg::State(), NULL);
}

//------------------------------------------------------------------------------
PostProcessUnit::PostProcessUnit(osg::State* s, osg::StateSet* parentStateSet)
{
    initialize(s, parentStateSet);
}


//------------------------------------------------------------------------------
void PostProcessUnit::initialize(osg::State* s, osg::StateSet* parentStateSet)
{
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
    
    // check if a geometry already exists if not so create it
    if (!sScreenQuad.valid())
    {
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
        if (parentStateSet != NULL) sScreenQuad->setStateSet(new osg::StateSet(*parentStateSet));
        else sScreenQuad->setStateSet(new osg::StateSet());
    }
    setBlendMode(false);

    // disable mipmapping per default
    mbUseMipmaps = false;
    mbUseMipmapShader = false;
    mbDirtyViewport = true;
        
    // initialze projection matrix
    sProjectionMatrix = osg::Matrix::ortho2D(0,1,0,1);
    
    // setup default modelview matrix
    sModelviewMatrix = osg::Matrixf::identity();
    
    // setup per default local state equal to the parent state 
    sState.setState(s);

    // setup uniform variable
    mShaderMipmapLevelUniform = new osg::Uniform("g_MipmapLevel", 0);
    sScreenQuad->getOrCreateStateSet()->addUniform(mShaderMipmapLevelUniform.get());
}

//------------------------------------------------------------------------------
PostProcessUnit::PostProcessUnit(const PostProcessUnit& ppu, const osg::CopyOp& copyop) : 
    osg::Object(ppu, copyop),
    mFBO(ppu.mFBO),
    mInputTex(ppu.mInputTex),
    mOutputTex(ppu.mOutputTex),
    mShader(ppu.mShader),
    mShaderMipmapLevelUniform(ppu.mShaderMipmapLevelUniform),
    mIndex(ppu.mIndex),
    sScreenQuad(ppu.sScreenQuad),
    sProjectionMatrix(ppu.sProjectionMatrix),
    sModelviewMatrix(ppu.sModelviewMatrix),
    sState(ppu.sState),
    mViewport(ppu.mViewport),
    mOutputPPU(ppu.mOutputPPU),
    mInputPPU(ppu.mInputPPU),
    mOutputInternalFormat(ppu.mOutputInternalFormat),
    mUseInputPPUViewport(ppu.mUseInputPPUViewport),
    mbUseMipmaps(ppu.mbUseMipmaps),
    mbUseMipmapShader(ppu.mbUseMipmapShader),
    mMipmapShader(ppu.mMipmapShader),
    mMipmapFBO(ppu.mMipmapFBO),
    mMipmapViewport(ppu.mMipmapViewport),
    mCamera(ppu.mCamera),
    mbDirtyInputTextures(ppu.mbDirtyInputTextures),
    mbDirtyOutputTextures(ppu.mbDirtyOutputTextures),
    mbDirtyViewport(ppu.mbDirtyViewport),
    mScreenQuadColor(ppu.mScreenQuadColor),
    mbOfflinePPU(ppu.mbOfflinePPU),
    mbActive(ppu.mbActive),
    mExpireTime(ppu.mExpireTime),
    mStartTime(ppu.mStartTime),
    mStartBlendValue(ppu.mStartBlendValue),
    mEndBlendValue(ppu.mEndBlendValue),
    mCurrentBlendValue(ppu.mCurrentBlendValue),
    mBlendFunc(ppu.mBlendFunc)    
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
    // check if input PPU name is specified
    for (int i=0; i < (int)mInputPPU.size(); i++)
    {
        if (mInputPPU[i] != NULL)
        {
            setInputTexture(mInputPPU[i]->getOutputTexture(0), i);
        }
    }
    assignInputTexture();
    
    // update viewport if we have one bound
    if (mUseInputPPUViewport != NULL)
    {
        setViewport(mUseInputPPUViewport->getViewport());
    }

    // check if mipmapping is enabled, then enable mipmapping on output textures
    if (mbUseMipmaps) enableMipmapGeneration();    
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

    // if viewport is dirty, so resetup it
    if (mbDirtyViewport)
    {
        // update viewport if such one is specified
        if (mUseInputPPUViewport != NULL)
        {
            setViewport(mUseInputPPUViewport->getViewport());
        }
        noticeChangeViewport();
        mbDirtyViewport = false;
    }

	// check if input textures are dirty
	if (mbDirtyInputTextures) assignInputTexture();

	// check if we have to recreate output textures
	if (mbDirtyOutputTextures) assignOutputTexture();

    // if input is valid texture 
    if (mInputTex[0].valid())
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
    osg::ref_ptr<osg::Program> currentShader = mShader;

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
        
    // now we assign input texture as our result texture, so we can generate mipmaps
    mInputTex[0] = output;
    assignInputTexture();

    // also we assign special shader which will generate mipmaps
    removeShader();
    mShader = mMipmapShader;
    assignShader();
    
    // now we render the result in a loop to generate mipmaps 
    for (int i=1; i < numLevels; i++)
    {
        // set mipmap level
        if (mShaderMipmapLevelUniform != NULL) 
            mShaderMipmapLevelUniform->set(float(i));

        // assign new viewport 
        mViewport = mMipmapViewport[i];
        mFBO = mMipmapFBO[i];

		//printf("mipmap %s %d, (%dx%d) %p\n", getResourceName().c_str(), i, (int)mViewport->width(), (int)mViewport->height(), output);
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
    std::map<int, osg::ref_ptr<osg::Texture> >::iterator it = mInputTex.begin();
    for (; it != mInputTex.end(); it++)
    {
        // set texture if it is valid
        if (it->second.valid())
        {
            ss->setTextureAttributeAndModes(it->first, it->second.get(), osg::StateAttribute::ON);

            // check if textures size is 0, then force an apply to load it
            if (it->second->getTextureWidth() == 0 || it->second->getTextureHeight() == 0)
            {
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
        ss->setAttributeAndModes(mShader.get(), osg::StateAttribute::ON);
        noticeAssignShader();
    }
}

//--------------------------------------------------------------------------
void PostProcessUnit::removeShader()
{
    osg::StateSet* ss = sScreenQuad->getOrCreateStateSet();
    
    // set shader if it is valid
    if (mShader.valid())
    {
        ss->setAttributeAndModes(mShader.get(), osg::StateAttribute::OFF);
        noticeRemoveShader();
    }
}

//--------------------------------------------------------------------------
void PostProcessUnit::setViewport(osg::Viewport* vp)
{
    mViewport = vp;//new osg::Viewport(*vp);
    sScreenQuad->getOrCreateStateSet()->setAttribute(mViewport.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    //noticeChangeViewport();
    mbDirtyViewport = true;
}

//--------------------------------------------------------------------------
void PostProcessUnit::setOutputInternalFormat(GLenum format)
{
    mOutputInternalFormat = format;
    
    // now generate output texture's and assign them to fbo 
    std::map<int, osg::ref_ptr<osg::Texture> >::iterator it = mOutputTex.begin();
    for (;it != mOutputTex.end(); it++)
    {
        if (it->second.valid()){
            it->second->setInternalFormat(mOutputInternalFormat);
            it->second->setSourceFormat(createSourceTextureFormat(mOutputInternalFormat));
			//printf("%s: intf=0x%x, fmt=0x%x\n", getResourceName().c_str(), mOutputInternalFormat, Util::createSourceTextureFormat(mOutputInternalFormat));
        }
    }

}

//------------------------------------------------------------------------------
void PostProcessUnit::init()
{
    mOutputTex = mInputTex;
}

//------------------------------------------------------------------------------
void PostProcessUnit::render(int mipmapLevel)
{
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

#if 0
//--------------------------------------------------------------------------
// PostProcessFXDepthmapBypass Implementation
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
PostProcessFXDepthmapBypass::PostProcessFXDepthmapBypass(osg::State* state, osg::StateSet* parentStateSet) : PostProcessFX(NULL, NULL)
{

}

//------------------------------------------------------------------------------
PostProcessFXDepthmapBypass::~PostProcessFXDepthmapBypass()
{

}

//------------------------------------------------------------------------------
void PostProcessFXDepthmapBypass::init()
{
    if (mCamera.valid())
        mOutputTex[0] = mCamera->getDepthTexture();
}

//------------------------------------------------------------------------------
void PostProcessFXDepthmapBypass::render(int mipmapLevel)
{
    if (mCamera.valid())
        mOutputTex[0] = mCamera->getDepthTexture();
    else
        LOG(("PostProcessFXDepthmapBypass: No valid camera bound!\n"));
}

//--------------------------------------------------------------------------
// PostProcessFXEmpty Implementation
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
PostProcessFXEmpty::PostProcessFXEmpty() : PostProcessFX(NULL, NULL)
{

}

//------------------------------------------------------------------------------
PostProcessFXEmpty::~PostProcessFXEmpty()
{

}

//------------------------------------------------------------------------------
void PostProcessFXEmpty::init()
{
    mOutputTex[0] = mInputTex[0];
}

//------------------------------------------------------------------------------
void PostProcessFXEmpty::render(int mipmapLevel)
{
    mOutputTex[0] = mInputTex[0];
}

//--------------------------------------------------------------------------
// PostProcessFXBypass Implementation
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
PostProcessFXBypass::PostProcessFXBypass(osg::State* state, osg::StateSet* parentStateSet) : PostProcessFXEmpty()
{

}

//------------------------------------------------------------------------------
PostProcessFXBypass::~PostProcessFXBypass()
{

}


//--------------------------------------------------------------------------
// PostProcessFXOut Implementation
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
PostProcessFXOut::PostProcessFXOut(osg::State* s, osg::StateSet* ss) : PostProcessFX(s,ss)
{

}

//------------------------------------------------------------------------------
PostProcessFXOut::~PostProcessFXOut()
{

}

//------------------------------------------------------------------------------
void PostProcessFXOut::init()
{
    if (mEventBase) mEventBase->onInit();
    
    // assign the input texture and shader if they are valid
    assignInputTexture();
    assignShader();
    
    initializeBase();
}


//------------------------------------------------------------------------------
void PostProcessFXOut::render(int mipmapLevel)
{
    // return if we do not get valid state
    #ifdef _OSG2_
    if (!sState.getState()) return;
    #else
    if (!sState.valid()) return;
    #endif
        
    // apply viewport if such is valid
    #ifdef _OSG2_
    if (mViewport.valid()) mViewport->apply(*sState.getState());
    #else
    if (mViewport.valid()) mViewport->apply(*sState);    
    #endif
        
    // render the content of the input texture into the frame buffer
    if (useBlendMode())
    {
        //glEnable(GL_BLEND);
        glColor4f(1,1,1, getCurrentBlendValue());
    }
    #ifdef _OSG2_
    sScreenQuad->draw(sState);
    #else
    sScreenQuad->draw(*sState);    
    #endif
    if (useBlendMode())
    {
        //glDisable(GL_BLEND);
        glColor4f(1,1,1,1);
    }   
}

//--------------------------------------------------------------------------
// PostProcessFXText Implementation
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
PostProcessFXText::PostProcessFXText(osg::State* s, osg::StateSet* ss) : PostProcessFXInOut(s,ss)
{
    // Create text for statistics
    mText.reset(new Text());
    mX = 0;
    mY = 0;
    mWidth = 1;
    mHeight = 1;
    mSize = 26.0;
}

//------------------------------------------------------------------------------
PostProcessFXText::~PostProcessFXText()
{
    mText.reset();
}

//------------------------------------------------------------------------------
void PostProcessFXText::init()
{
    // initialize text
    mText->setFont("data/fonts/fudd.ttf");
    mText->setColor(1,1,1,1);
    mText->getTextPtr()->setStateSet(sScreenQuad->getOrCreateStateSet());

    mOutputTex[0] = mInputTex[0];
    PostProcessFXInOut::init();

    // setup script functions
    std::vector<nrEngine::ScriptParam> param;
    param.push_back(this);
    Engine::sScriptEngine()->add("textPPUsetString", setText, param);
    Engine::sScriptEngine()->add("textPPUsetProperty", setProperty, param); 
    Engine::sScriptEngine()->add("textSetSize", setSize, param); 
    Engine::sScriptEngine()->add("textSetPosition", setPosition, param); 
    Engine::sScriptEngine()->add("textSetColor", setColor, param); 

    sProjectionMatrix = osg::Matrix::ortho2D(0,1,0,1);

}


//------------------------------------------------------------------------------
bool PostProcessFXText::parseXmlSettings(TiXmlElement* root)
{
    // we only work on ppfx tags
    if (root == NULL) return false;
    if (strcmp(root->Value(), "postprocess")) return false;
    
    // iterate through childrens of this node and check for their values
    for( TiXmlElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement() ){
        
        // which index does this pp have. Index 0 comes as first
        if (!strcmp(child->Value(), "property")){
            if (child->Attribute("name")) mPropertyName = child->Attribute("name");
        }
        else if (!strcmp(child->Value(), "dim")){
            if (child->Attribute("width")) mWidth = boost::lexical_cast<float>(child->Attribute("width"));
            if (child->Attribute("height")) mHeight = boost::lexical_cast<float>(child->Attribute("height"));
            if (child->Attribute("size")) mSize = boost::lexical_cast<float>(child->Attribute("size"));
            if (child->Attribute("x")) mX = boost::lexical_cast<float>(child->Attribute("x"));
            if (child->Attribute("y")) mY = boost::lexical_cast<float>(child->Attribute("y"));
        }
    }
    
    //printf("dim: %f %f %f %f\n", mX, mY, mWidth, mHeight);
    mText->setPosition(mX, mY);
    mText->setColor(1,1,1,1);

    // now let parent class parse it
    return PostProcessFXInOut::parseXmlSettings(root);
    
}

//------------------------------------------------------------------------------
void PostProcessFXText::render(int mipmapLevel)
{
    // return if we do not get valid state
    #ifdef _OSG2_
    if (!sState.getState()) return;
    #else
    if (!sState.valid()) return;    
    #endif
    
    // can only be done on valid data 
    if (mFBO.valid() && mViewport.valid())
    {
        // we take the width 640 as reference width for the size of characters
        mText->setSize(mSize * (float(getViewport()->width()) / 640.0));

        #ifdef _OSG2_
            // aplly stateset
            sState.getState()->apply(sScreenQuad->getStateSet());
    
            // apply framebuffer object, this will bind it, so we can use it
            mFBO->apply(*sState.getState());
    
            // apply viewport
            mViewport->apply(*sState.getState());
            
            // compute new color 
            osg::Vec4 color = mText->getColor();
            color.a() = getCurrentBlendValue();

            // get string and print it to the output
            mText->setColor(color);
            sState.getState()->apply(mText->getTextPtr()->getStateSet());
            if (mPropertyName.length() > 0)
                mText->setText(Engine::sPropertyManager()->getByFullname<std::string>(mPropertyName));

            //printf("%s ==> %s\n", getResourceName().c_str(), mText->getText().c_str());
            //if (getResourceName() == "TextRelief" || getResourceName() == "TextDecal") glClear(GL_COLOR_BUFFER_BIT);

            /*glDisable(GL_BLEND);
            glColor4f(0,0,0,1);
            mText->setColor(0,0,0,1);
            mText->getTextPtr()->draw(sState);
            mText->setColor(1,1,1,getCurrentBlendValue());
            glColor4f(1,1,1, getCurrentBlendValue());
            */

            glEnable(GL_BLEND);
            mText->getTextPtr()->draw(sState);
            glDisable(GL_BLEND);

        #else
            // aplly stateset
            sState->apply(sScreenQuad->getStateSet());
    
            // apply framebuffer object, this will bind it, so we can use it
            mFBO->apply(*sState);
    
            // apply viewport
            mViewport->apply(*sState);
            
            // get string and print it to the output
            mText->setColor(1,1,1,getCurrentBlendValue());
            sState->apply(mText->getText()->getStateSet());
            std::string str =  Engine::sPropertyManager()->getByFullname<std::string>(mPropertyName);
            mText->setText(str);
            mText->getText()->draw(*sState);        
        #endif
                
    }
}

//------------------------------------------------------------------------------
void PostProcessFXText::noticeFinishRendering()
{
}

//--------------------------------------------------------------------------
ScriptFunctionDec(setText, PostProcessFXText)
{
    if (args.size() <= 2){
        return ScriptResult(std::string("textPPUsetString (ppu, text) : wrong parameter count"));
    }

    // get text ppu 
    ResourcePtr<PostProcessFXText> ppu = Engine::sResourceManager()->getByName(args[1]);
    if (!ppu.valid()) return ScriptResult(std::string("Such a ppu does not exists in the database"));

    std::string text;
    for (unsigned int i=2; i < args.size(); i++)
    {
        text += args[i];
        text += std::string(" ");
    }

    ppu->mText->setText(text);
    ppu->mPropertyName = std::string();
    return ScriptResult(); 
}

//--------------------------------------------------------------------------
ScriptFunctionDec(setProperty, PostProcessFXText)
{
    if (args.size() <= 1){
        return ScriptResult(std::string("textPPUsetProperty (ppu, text) : wrong parameter count"));
    }
    // get text ppu 
    ResourcePtr<PostProcessFXText> ppu = Engine::sResourceManager()->getByName(args[1]);
    if (!ppu.valid()) return ScriptResult(std::string("Such a ppu does not exists in the database"));
    
    ppu->mPropertyName = args[2];
    return ScriptResult(); 
}

//--------------------------------------------------------------------------
ScriptFunctionDec(setSize, PostProcessFXText)
{
    if (args.size() <= 2){
        return ScriptResult(std::string("textSetSize (ppu, size) : wrong parameter count"));
    }

    // get text ppu 
    ResourcePtr<PostProcessFXText> ppu = Engine::sResourceManager()->getByName(args[1]);
    if (!ppu.valid()) return ScriptResult(std::string("Such a ppu does not exists in the database"));

    // get size 
    ppu->mSize = boost::lexical_cast<float>(args[2]);

    return ScriptResult(); 
}

//--------------------------------------------------------------------------
ScriptFunctionDec(setColor, PostProcessFXText)
{
    if (args.size() <= 5){
        return ScriptResult(std::string("textSetColor (ppu, r,g,b,a) : wrong parameter count"));
    }

    // get text ppu 
    ResourcePtr<PostProcessFXText> ppu = Engine::sResourceManager()->getByName(args[1]);
    if (!ppu.valid()) return ScriptResult(std::string("Such a ppu does not exists in the database"));

    // get size 
    float r = boost::lexical_cast<float>(args[2]);
    float g = boost::lexical_cast<float>(args[3]);
    float b = boost::lexical_cast<float>(args[4]);
    float a = boost::lexical_cast<float>(args[5]);

    ppu->mText->setColor(r,g,b,a);
    return ScriptResult(); 
}

//--------------------------------------------------------------------------
ScriptFunctionDec(setPosition, PostProcessFXText)
{
    if (args.size() <= 2){
        return ScriptResult(std::string("textSetPosition (ppu, x, y) : wrong parameter count"));
    }

    // get text ppu 
    ResourcePtr<PostProcessFXText> ppu = Engine::sResourceManager()->getByName(args[1]);
    if (!ppu.valid()) return ScriptResult(std::string("Such a ppu does not exists in the database"));

    // get size 
    float x = boost::lexical_cast<float>(args[2]);
    float y = boost::lexical_cast<float>(args[3]);
    ppu->mText->setPosition(x,y);

    return ScriptResult(); 
}

//--------------------------------------------------------------------------
// PostProcessFXOutCapture Implementation
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
PostProcessFXOutCapture::PostProcessFXOutCapture(osg::State* s, osg::StateSet* ss) : PostProcessFXOut(s,ss)
{
}

//------------------------------------------------------------------------------
PostProcessFXOutCapture::~PostProcessFXOutCapture()
{
}

//------------------------------------------------------------------------------
void PostProcessFXOutCapture::init()
{
    // do initialization in the base
    PostProcessFXOut::init();
}


//------------------------------------------------------------------------------
void PostProcessFXOutCapture::render(int mipmapLevel)
{
}

//------------------------------------------------------------------------------
void PostProcessFXOutCapture::noticeFinishRendering()
{
    #ifdef _OSG2_
    if (isActive() && sState.getState())
    #else
    if (isActive() && sState.valid())    
    #endif
    {
        // get properties and calculate size
        int w = glfw::Package::instance()->getWindowWidth();
        int h = glfw::Package::instance()->getWindowHeight();
        
        // if we want to capture the framebuffer
        static int captureFileNumber = 0;
        char filename[256];
        
        // for each input texture do
        for (unsigned int i=0; i < mInputTex.size(); i++)
        {
            // create file name
            sprintf( filename, "tmp/%d_%04d.png", i, captureFileNumber);
            printf("Capture %d frame (%dx%d) to %s (time %f)...", captureFileNumber,w,h,filename, Engine::sClock()->getTime());
            captureFileNumber++;
            
            // input texture 
            osg::Texture* input = getInputTexture(i);

            #ifdef _OSG2_            
                // bind input texture, so that we can get image from it
                if (input != NULL) input->apply(*sState.getState());
                
                // retrieve texture content
                osg::ref_ptr<osg::Image> img = new osg::Image();
                img->readImageFromCurrentTexture(sState.getContextID(), false); 
                //img->readPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE);
                //osgDB::ReaderWriter::WriteResult res = osgDB::Registry::instance()->writeImage(*img, filename);
                write_png(filename, img->data(), w, h, 4, 8, PNG_COLOR_TYPE_RGBA, 1);
                //if (res.success())
                    printf(" OK\n");
                //else
                //    printf(" failed! (%s)\n", res.message().c_str());
                
                // unbind the texture back 
                if (input != NULL)
                    sState.getState()->applyTextureMode(0, input->getTextureTarget(), false);
            #else
                // bind input texture, so that we can get image from it
                if (input != NULL) input->apply(*sState);
                
                // retrieve texture content
                osg::ref_ptr<osg::Image> img = new osg::Image();
                img->readImageFromCurrentTexture(sState->getContextID(), false); 
                osgDB::ReaderWriter::WriteResult res = osgDB::Registry::instance()->writeImage(*img, filename);
                if (res.success())
                    printf(" OK\n");
                else
                    printf(" failed! (%s)\n", res.message().c_str());
                
                // unbind the texture back 
                if (input != NULL)
                    sState->applyTextureMode(0, input->getTextureTarget(), false);            
            #endif
        }
    }
}


//--------------------------------------------------------------------------
// PostProcessFXInOut Implementation
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
PostProcessFXInOut::PostProcessFXInOut(osg::State* s, osg::StateSet* ss) : PostProcessFX(s,ss)
{
    // create FBO because we need it
    mFBO = new osg::FrameBufferObject();

	// if the input does have mipmaps, then they will be passed to the output
	setMipmappedIO(false);
	mNumLevels = 0;

}

//------------------------------------------------------------------------------
PostProcessFXInOut::~PostProcessFXInOut()
{

}

//------------------------------------------------------------------------------
void PostProcessFXInOut::init()
{
    if (mEventBase) mEventBase->onInit();

    initializeBase();

    assignShader();
	assignOutputTexture();
    
}

//------------------------------------------------------------------------------
void PostProcessFXInOut::assignOutputTexture()
{
    if (mFBO.valid())
    {
        // get current viewport from the state 
        assert(mViewport.valid());

		// now generate output texture's and assign them to fbo 
		std::map<int, osg::ref_ptr<osg::Texture> >::iterator it = mOutputTex.begin();
		for (int i = 0; it != mOutputTex.end(); it++, i++)
		{
			// initialze the resulting texture
			osg::Texture2D* mTex = dynamic_cast<osg::Texture2D*>(it->second.get());
			if (mTex == NULL)
			{
				mTex = new osg::Texture2D();
				mTex->setTextureSize(int(mViewport->x() + mViewport->width()), int(mViewport->y() + mViewport->height()) );
				mTex->setResizeNonPowerOfTwoHint(false);
				mTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP);
				mTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP);				
				mTex->setInternalFormat(getOutputInternalFormat());
				mTex->setSourceFormat(Util::createSourceTextureFormat(getOutputInternalFormat()));

				//}
				
				//mTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
				//mTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
				//mTex->setBorderColor(osg::Vec4(0,0,0,0));

				// check if the input texture was in nearest mode 
				if (getInputTexture(0) && getInputTexture(0)->getFilter(osg::Texture2D::MIN_FILTER) == osg::Texture2D::NEAREST)
					mTex->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::NEAREST);
				else
					mTex->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
	
				if (getInputTexture(0) && getInputTexture(0)->getFilter(osg::Texture2D::MAG_FILTER) == osg::Texture2D::NEAREST)
					mTex->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::NEAREST);
				else
					mTex->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);
	
				it->second = mTex;
        
			}

            // attach the texture to the fbo
            mFBO->setAttachment(GL_COLOR_ATTACHMENT0_EXT + i, osg::FrameBufferAttachment(mTex));

            // setup viewport
            //osg::Viewport* vp = new osg::Viewport(*mViewport);
            //setViewport(vp);
		}
        sScreenQuad->getOrCreateStateSet()->setAttribute(mViewport.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

		// clean mipmap data for output
		mMipmapFBO.clear();
		mMipmapViewport.clear();

		// recheck the mipmap bypass data structures
		checkIOMipmappedData();

	}
	mbDirtyOutputTextures = false;
}


//------------------------------------------------------------------------------
void PostProcessFXInOut::checkIOMipmappedData()
{
    if (mFBO.valid() && mOutputTex.size() > 0)
    {
        // do only proceed if output texture is valid
        if (mOutputTex.begin()->second == NULL) return;

		// clean viewport data
		mIOMipmapViewport.clear();
		mIOMipmapFBO.clear();
	
		// get dimensions of the output data
		int width = (mOutputTex.begin()->second)->getTextureWidth();
		int height = (mOutputTex.begin()->second)->getTextureHeight();
		int numLevels = 1 + (int)floor(log2(Util::max(width, height)));
		mNumLevels = numLevels;

		// generate fbo for each mipmap level 
		for (int level=0; level < numLevels; level++)
		{
			// generate viewport for this level
			osg::Viewport* vp = new osg::Viewport();
			int w = Util::max(1, (int)floor(float(width) / float(pow(2,level)) ));
			int h = Util::max(1, (int)floor(float(height) / float(pow(2,level)) ));
			vp->setViewport(0,0, w, h);
			mIOMipmapViewport.push_back(osg::ref_ptr<osg::Viewport>(vp));

			// this is the fbo for this level 
			osg::ref_ptr<osg::FrameBufferObject> fbo = new osg::FrameBufferObject();

			// for each output texture do
			std::map<int, osg::ref_ptr<osg::Texture> >::iterator it = mOutputTex.begin();
			for (int mrt = 0; it != mOutputTex.end(); it++, mrt++)
			{	
				// output texture 
				osg::ref_ptr<osg::Texture2D> output = dynamic_cast<osg::Texture2D*>(it->second.get());
				assert(output != NULL);
	
				// check if we have generated all the fbo's for each mipmap level
				int _width = output->getTextureWidth();
				int _height = output->getTextureHeight();

				// if width and height are not equal, then we give some error and stop here 
				if ((_width != width || _height != height))
				{
					printf("PostProcessFXInOut %s: output textures has different dimensions\n", getResourceName().c_str());
					return; 
				}
	
				// set fbo of current level with to this output			
				fbo->setAttachment(GL_COLOR_ATTACHMENT0_EXT + mrt, osg::FrameBufferAttachment(output.get(), level));

				//printf("fbo %s: %d (%d) = %p, fmt=0x%x\n", getResourceName().c_str(), mrt, level, output.get(), output->getInternalFormat());
			}

			// store fbo
			mIOMipmapFBO.push_back(fbo);
		}
	}
}

double __check_timer(GLuint in_timer_id)
{
  GLint resultAvailable = GL_FALSE;                                    
           
  
  do 
    {                                                                  
          
      glGetQueryObjectiv(in_timer_id, GL_QUERY_RESULT_AVAILABLE,      
             
             &resultAvailable);                                                
              
      
    } 
  while (resultAvailable==GL_FALSE);                                  
          
  
  GLint result;  // pick up the fresh result                          
                                       
  
  glGetQueryObjectiv(in_timer_id, GL_QUERY_RESULT, &result);   
  // result is in 10^-9sec.                                          
                         

  return (double)result/1000000000.0;
}      

//------------------------------------------------------------------------------
void PostProcessFXInOut::render(int mipmapLevel)
{
	// if we do generate mipmaps
	if (mipmapLevel >= 0)
	{
		doRender(mipmapLevel);
		return;
	}

	// do following only if we have mipmapped io
	if (getMipmappedIO())
	{
		// store current viewport 
		osg::ref_ptr<osg::Viewport> currentViewport = mViewport;
		osg::ref_ptr<osg::FrameBufferObject> currentFBO = mFBO;

        //GLuint timer_id;
        //glGenQueries(1, &timer_id);  


        //glBeginQuery(GL_TIME_ELAPSED_EXT,timer_id);

    //glFinish();	
    //float64 startTime = Engine::sClock()->getTimeSource()->getSystemTime();

		// otherwise we want to bypass mipmaps, thus do following for each mipmap level
		for (int i=0; i < mNumLevels; i++)
		{
			// set mipmap level
			if (mShader.valid()) mShader->set("g_MipmapLevel",float(i));
	
			// assign new viewport and fbo
			mViewport = mIOMipmapViewport[i];
			mFBO = mIOMipmapFBO[i];
	
			//printf("io-mipmap %s %d, (%dx%d) \n", getResourceName().c_str(), i, (int)mViewport->width(), (int)mViewport->height());
	
			// render the content
			doRender(i);
		}
        //glEndQuery(GL_TIME_ELAPSED_EXT);

        //printf("%s: %f sec\n", getResourceName().c_str(), __check_timer(timer_id) );

        //glDeleteQueries(1, &timer_id);

    //glFinish();
    //float64 stopTime = Engine::sClock()->getTimeSource()->getSystemTime();

    //printf("%s: %f sec\n", getResourceName().c_str(), stopTime - startTime);
	
		// restore current viewport and fbo 
		mFBO = currentFBO;
		mViewport = currentViewport;

	// otherwise just render
	}else{
		doRender(mipmapLevel);
	}
}

//------------------------------------------------------------------------------
void PostProcessFXInOut::setMipmappedIO(bool b)
{
	mbDirtyOutputTextures = b;
	mbMipmappedIO = b;
}

//------------------------------------------------------------------------------
void PostProcessFXInOut::doRender(int mipmapLevel)
{
    // return if we do not get valid state
    #ifdef _OSG2_
    if (!sState.getState()) return;
    #else
    if (!sState.valid()) return;    
    #endif
    //if (mInputTex[0]->getTextureWidth() <= 0) return;
    //if (mInputTex[0]->getTextureWidth() != mOutputTex[0]->getTextureWidth()) return;

    // can only be done on valid data 
    if (mFBO.valid() && mViewport.valid())
    {
        // setup shader global properties
        //if (mShader.valid()) mShader->set("g_TextureWidth", (float)(mViewport->x() + mViewport->width()));
        //if (mShader.valid()) mShader->set("g_TextureHeight",(float)(mViewport->y() + mViewport->height()));
        //    if (mShader.valid()) mShader->set("g_MipmapLevel",float(mipmapLevel));

        // update shaders manually, because they are do not updated from scene graph
        if (mShader.valid()) mShader->update();
            /*if (getResourceName() == "HeightmapBackPrepareNormalmapPPU")
            {            
                mShader->enable(sScreenQuad->getStateSet(), true);
            }*/

        #ifdef _OSG2_
        
            // aplly stateset
            sState.getState()->apply(sScreenQuad->getStateSet());
            
            // if we render to mipmap levels>0 rather than to base level
            if (mipmapLevel  >= 0)
            {
                // HACK: Need this here, because otherwise the texture is not bound,
                // probably some problem in the state set
//                 sState.getState()->setActiveTextureUnit(0);
//                 mInputTex[0]->apply(*sState.getState());
    
                // if we render to a mipmap, so we can read only from level below current
//                 glTexParameteri(mInputTex[0]->getTextureTarget(), GL_TEXTURE_BASE_LEVEL, mipmapLevel-1);
//                 glTexParameteri(mInputTex[0]->getTextureTarget(), GL_TEXTURE_MAX_LEVEL, mipmapLevel-1);
            }
            
            // apply framebuffer object, this will bind it, so we can use it
// mFBO->setAttachment(GL_COLOR_ATTACHMENT0_EXT + 0, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2D*>(mOutputTex[0].get())));
            mFBO->apply(*sState.getState());
            
            //printf("in %s : %d\n", getResourceName().c_str(), getInputTexture(0)->getInternalFormat());
            //printf("out %s : %d\n", getResourceName().c_str(), getOutputTexture(0)->getInternalFormat());

            // apply viewport
            mViewport->apply(*sState.getState());

            /*if (getResourceName() == "HeightmapBackPrepareNormalmapPPU")
            {            
                int v[4];
                glGetIntegerv(GL_VIEWPORT, v);
                printf("%d %d %d %d\n", v[0], v[1], v[2], v[3]);

                //int w = mInputTex[0]->getTextureWidth();
                //int h = mInputTex[0]->getTextureHeight();
                //glViewport(0,0,256,256);

                //printf("%d %d\n", w, h);
                //sState.getState()->applyTextureMode(0, GL_TEXTURE_2D, true);
                //mInputTex[0]->apply(*sState.getState());
                printf("size %d: %d\n", mOutputTex.size(), mOutputTex[0]->getTextureWidth());
                //sState.getState()->applyTextureMode(0, GL_TEXTURE_2D, false);
  //              dynamic_cast<osg::Texture2D*>(mOutputTex[0].get())->setTextureSize(256, 256);
  //              mOutputTex[0]->apply(*sState.getState());
            }*/

            // render the content of the input texture into the frame buffer
            if (useBlendMode() && getOfflineMode() == false)
            {
                //printf("%s - %d, %d, %f\n", getResourceName().c_str(), useBlendMode(), getOfflineMode(), getCurrentBlendValue()); 
                glEnable(GL_BLEND);
                glColor4f(1,1,1, getCurrentBlendValue());
                sScreenQuad->draw(sState);
                glDisable(GL_BLEND);
                glColor4f(1,1,1,1);
            }else{
                glDisable(GL_BLEND);
                glColor4f(1,1,1,1);
                sScreenQuad->draw(sState);
                glColor4f(1,1,1,1);
            }
        #else
            // aplly stateset
            sState->apply(sScreenQuad->getStateSet());
    
            // if we render to mipmap levels>0 rather than to base level
            if (mipmapLevel >= 0)
            {
                // HACK: Need this here, because otherwise the texture is not bound,
                // probably some problem in the state set
//                sState->setActiveTextureUnit(0);
//                mInputTex[0]->apply(*sState);
    
                // if we render to a mipmap, so we can read only from level below current
//                 glTexParameteri(mInputTex[0]->getTextureTarget(), GL_TEXTURE_BASE_LEVEL, mipmapLevel-1);
//                 glTexParameteri(mInputTex[0]->getTextureTarget(), GL_TEXTURE_MAX_LEVEL, mipmapLevel-1);
            }
            
            // apply framebuffer object, this will bind it, so we can use it
            mFBO->apply(*sState);
    
            // apply viewport
            mViewport->apply(*sState);
    
            // render the content of the input texture into the frame buffer
            if (useBlendMode())
            {
                glEnable(GL_BLEND);
                glColor4f(1,1,1, getCurrentBlendValue());
            }
            sScreenQuad->draw(*sState);
            if (useBlendMode())
            {
                glDisable(GL_BLEND);
                glColor4f(1,1,1,1);
            }        
        #endif
    GET_GLERROR(0);

    }
    
}


//------------------------------------------------------------------------------
bool PostProcessFXInOut::parseXmlSettings(TiXmlElement* root)
{   
    // we only work on ppfx tags
    if (root == NULL) return false;
    if (strcmp(root->Value(), "postprocess")) return false;

    // iterate through childrens of this node and check for their values
    for( TiXmlElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement() )
    {   
        if (!strcmp(child->Value(), "output"))
        {
            if (child->Attribute("inoutmipmap")) setMipmappedIO(boost::lexical_cast<bool>(child->Attribute("inoutmipmap")));
        }
    }

    // now let parent class parse it
    return PostProcessFX::parseXmlSettings(root);
}

//------------------------------------------------------------------------------
void PostProcessFXInOut::noticeChangeViewport()
{
    // change size of the result texture according to the viewport
    std::map<int, osg::ref_ptr<osg::Texture> >::iterator it = mOutputTex.begin();
    for (; it != mOutputTex.end(); it++)
    {
        if (it->second.valid())
        {
            // currently we are working only with 2D textures
            assert (dynamic_cast<osg::Texture2D*>(it->second.get()) != NULL);
            
            // change size
            osg::Texture2D* mTex = dynamic_cast<osg::Texture2D*>(it->second.get());
            mTex->setTextureSize(int(mViewport->x() + mViewport->width()), int(mViewport->y() + mViewport->height()) );
            
                /*it->second->apply(*sState.getState());
                glBindTexture(it->second->getTextureTarget(), 0);

            // attach the texture to the fbo
            {            
            printf("Change Viewport %s %d %d\n" , getResourceName().c_str(), int(mViewport->x() + mViewport->width()), int(mViewport->y() + mViewport->height()));
            printf("out_size: %d %d\n", mInputTex[0]->getTextureWidth(), mInputTex[0]->getTextureHeight());
            }*/
        }
    }
}


//--------------------------------------------------------------------------
// PostProcessFXInResampleOut Implementation
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
PostProcessFXInResampleOut::PostProcessFXInResampleOut(osg::State* s, osg::StateSet* ss) : PostProcessFX(s,ss)
{
    // create FBO because we need it
    mFBO = new osg::FrameBufferObject();
    
    // setup default values 
    //mWidth = 0;
    //mHeight = 0;
    //mX = 0;
    //mY = 0;
    mWidthFactor = 1.0;
    mHeightFactor = 1.0;
    mDirtyFactor = false;
}

//------------------------------------------------------------------------------
PostProcessFXInResampleOut::~PostProcessFXInResampleOut()
{

}

//------------------------------------------------------------------------------
bool PostProcessFXInResampleOut::parseXmlSettings(TiXmlElement* root)
{
    // we only work on ppfx tags
    if (root == NULL) return false;
    if (strcmp(root->Value(), "postprocess")) return false;

    // iterate through childrens of this node and check for their values
    for( TiXmlElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement() ){
        
        // which index does this pp have. Index 0 comes as first
        if (!strcmp(child->Value(), "resample")){
            if (child->Attribute("fwidth")) mWidthFactor = boost::lexical_cast<float>(child->Attribute("fwidth"));
            if (child->Attribute("fheight")) mHeightFactor = boost::lexical_cast<float>(child->Attribute("fheight"));
            mDirtyFactor = true;
        }
        
    }
    
    // now let parent class parse it
    return PostProcessFX::parseXmlSettings(root);
    
}

//------------------------------------------------------------------------------
void PostProcessFXInResampleOut::setFactor(float w, float h)
{
    mWidthFactor = w;
    mHeightFactor = h;
    mDirtyFactor = true;
}


//--------------------------------------------------------------------------
ScriptFunctionDec(setResamplingFactor, PostProcessFXInResampleOut)
{
    if (args.size() <= 3){
        return ScriptResult(std::string("setResamplingFactorPPU (ppu, x,y) : wrong parameter count"));
    }

    // get text ppu 
    ResourcePtr<PostProcessFXInResampleOut> ppu = Engine::sResourceManager()->getByName(args[1]);
    if (!ppu.valid()) return ScriptResult(std::string("Such a ppu does not exists in the database"));

    // get size 
    float x = boost::lexical_cast<float>(args[2]);
    float y = boost::lexical_cast<float>(args[3]);

    ppu->setFactor(x,y);
    return ScriptResult(); 
}

//------------------------------------------------------------------------------
void PostProcessFXInResampleOut::init()
{
    if (mEventBase) mEventBase->onInit();
    
    // assign shader
    assignInputTexture(); 
    assignShader();
    
    // if FBO is initialized, so assign the output texture to it
    if (mFBO.valid())
    {
        // a viewport has to be bound
        assert(mViewport.valid());
        mOrigWidth = int(mViewport->x() + mViewport->width());
        mOrigHeight = int(mViewport->y() + mViewport->height());
        
        // now setup output textures
        std::map<int, osg::ref_ptr<osg::Texture> >::iterator it = mOutputTex.begin();
        for (int i = 0; it != mOutputTex.end(); it++, i++)
        {
            // initialze the resulting texture
            osg::Texture2D* mTex = dynamic_cast<osg::Texture2D*>(it->second.get());
            if (mTex == NULL)
            {
                mTex = new osg::Texture2D();
            
                // setup texture
                mTex->setTextureSize(int(float(mOrigWidth) * mWidthFactor), int(float(mOrigHeight) * mHeightFactor));
                mTex->setResizeNonPowerOfTwoHint(false);
                mTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
                mTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
                mTex->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
                mTex->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);
                mTex->setInternalFormat(getOutputInternalFormat());
                mTex->setSourceFormat(Util::createSourceTextureFormat(getOutputInternalFormat()));
                it->second = mTex;
            }
            
            // setup viewport
            osg::Viewport* vp = new osg::Viewport(*mViewport);
            setViewport(vp);
            
            // attach the texture to the fbo
            mFBO->setAttachment(GL_COLOR_ATTACHMENT0_EXT + i, osg::FrameBufferAttachment(mTex));
        }
    }
    
    initializeBase();

    std::vector<nrEngine::ScriptParam> param;
    Engine::sScriptEngine()->add("setResampleFactorPPU", setResamplingFactor, param);

}

//------------------------------------------------------------------------------
void PostProcessFXInResampleOut::noticeChangeViewport()
{
    // setup new sizes
    mOrigWidth = int(mViewport->x() + mViewport->width());
    mOrigHeight = int(mViewport->y() + mViewport->height());

    // change size of the result texture according to the viewport
    std::map<int, osg::ref_ptr<osg::Texture> >::iterator it = mOutputTex.begin();
    for (; it != mOutputTex.end(); it++)
    {
        if (it->second.valid())
        {
            // currently we are working only with 2D textures
            assert (dynamic_cast<osg::Texture2D*>(it->second.get()) != NULL);
            
            // change size
            osg::Texture2D* mTex = dynamic_cast<osg::Texture2D*>(it->second.get());
            mTex->setTextureSize(mOrigWidth, mOrigHeight);
        }
    }
}

//------------------------------------------------------------------------------
void PostProcessFXInResampleOut::render(int mipmapLevel)
{
    // if we have to reset the resampling factor
    if (mDirtyFactor)
    {
        // there must be an viewport
        assert(mViewport.valid());

        // setup new viewport size
        mViewport->width() = mOrigWidth * mWidthFactor;
        mViewport->height() = mOrigHeight * mHeightFactor;
        noticeChangeViewport();
        mDirtyFactor = false;
    }

    #ifdef _OSG2_
        // return if we do not get valid state
        if (!sState.getState()) return;
    
        // can only be done on valid data 
        if (mFBO.valid() && mViewport.valid())
        {
            // setup shader global properties
            if (mShader.valid()) mShader->set("g_TextureWidth", (float)(mViewport->x() + mViewport->width()));
            if (mShader.valid()) mShader->set("g_TextureHeight",(float)(mViewport->y() + mViewport->height()));
            
            // update shaders manually, because they are do not updated from scene graph
            if (mShader.valid()) mShader->update();
            
            // aplly stateset
            sState.getState()->apply(sScreenQuad->getStateSet());
    
            // if we render to mipmap levels>0 rather than to base level
            if (mipmapLevel  >= 0)
            {
                // HACK: Need this here, because otherwise the texture is not bound,
                // probably some problem in the state set
//                sState.getState()->setActiveTextureUnit(0);
//                mInputTex[0]->apply(*sState.getState());
    
                // if we render to a mipmap, so we can read only from level below current
//                 glTexParameteri(mInputTex[0]->getTextureTarget(), GL_TEXTURE_BASE_LEVEL, mipmapLevel-1);
//                 glTexParameteri(mInputTex[0]->getTextureTarget(), GL_TEXTURE_MAX_LEVEL, mipmapLevel-1);
            }
            
            // apply framebuffer object, this will bind it, so we can use it
            mFBO->apply(*sState.getState());
    
            // apply viewport
            mViewport->apply(*sState.getState());
    
            // render the content of the input texture into the frame buffer
            glColor4f(1,1,1,1);
            sScreenQuad->draw(sState);      
        }

    #else
        // return if we do not get valid state
        if (!sState.valid()) return;
    
        // can only be done on valid data 
        if (mFBO.valid() && mViewport.valid())
        {
            // setup shader global properties
            if (mShader.valid()) mShader->set("g_TextureWidth", (float)(mViewport->x() + mViewport->width()));
            if (mShader.valid()) mShader->set("g_TextureHeight",(float)(mViewport->y() + mViewport->height()));
            
            // update shaders manually, because they are do not updated from scene graph
            if (mShader.valid()) mShader->update();
            
            // aplly stateset
            sState->apply(sScreenQuad->getStateSet());
    
            // if we render to mipmap levels>0 rather than to base level
            if (mipmapLevel  >= 0)
            {
                // HACK: Need this here, because otherwise the texture is not bound,
                // probably some problem in the state set
                sState->setActiveTextureUnit(0);
                mInputTex[0]->apply(*sState);
    
                // if we render to a mipmap, so we can read only from level below current
//                 glTexParameteri(mInputTex[0]->getTextureTarget(), GL_TEXTURE_BASE_LEVEL, mipmapLevel-1);
//                 glTexParameteri(mInputTex[0]->getTextureTarget(), GL_TEXTURE_MAX_LEVEL, mipmapLevel-1);
            }
            
            // apply framebuffer object, this will bind it, so we can use it
            mFBO->apply(*sState);
    
            // apply viewport
            mViewport->apply(*sState);
    
            // render the content of the input texture into the frame buffer
            sScreenQuad->draw(*sState);      
        }
    
    #endif
}



//--------------------------------------------------------------------------
// PostProcessFXInOutCopy Implementation
//--------------------------------------------------------------------------
//------------------------------------------------------------------------------
PostProcessFXInOutCopy::PostProcessFXInOutCopy(osg::State* s, osg::StateSet* ss) : PostProcessFXInOut(s,ss)
{
    mMipmapLevel = -1;
}

//------------------------------------------------------------------------------
PostProcessFXInOutCopy::~PostProcessFXInOutCopy()
{
}

//------------------------------------------------------------------------------
void PostProcessFXInOutCopy::init()
{
    PostProcessFXInOut::init();
}


//------------------------------------------------------------------------------
void PostProcessFXInOutCopy::render(int mipmapLevel)
{
    // do rendering as it should be done
    PostProcessFXInOut::render(mipmapLevel);
}

//------------------------------------------------------------------------------
void PostProcessFXInOutCopy::noticeFinishRendering()
{
    // do nothing if nothing is valid
    #ifdef _OSG2_
    if (!sState.getState() || !mOutputTex[0].valid()) return;
    #else
    if (!sState.valid() || !mOutputTex[0].valid()) return;    
    #endif
    
    // get maximal level count
    int width = mOutputTex[0]->getTextureWidth();
    int height = mOutputTex[0]->getTextureHeight();
    int numLevels = 1 + (int)floor(log2(Util::max(width, height)));

    // the mipmap level we want to read from is
    int level = mMipmapLevel < 0 ? numLevels + mMipmapLevel : mMipmapLevel;
    if (level < 0) level = 0;
    
    // create image to hold result, if no such already exists
    if (!mMipmapImage.valid()) mMipmapImage = new osg::Image();

    // compute the w and h based on the level
    int w = Util::max(1, (int)floor(float(width) / float(pow(2,level)) ));
    int h = Util::max(1, (int)floor(float(height) / float(pow(2,level)) ));
    
    // now we want to copy result
    if ((int)mMipmapFBO.size() > level && mCopyLastMipmapTo.length())
    {
        // read out
        #ifdef _OSG2_
        mMipmapFBO[level]->apply(*sState.getState());
        #else
        mMipmapFBO[level]->apply(*sState);        
        #endif
        glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
        mMipmapImage->readPixels(0, 0, w, h, GL_ALPHA, GL_FLOAT);

        // store result in variable
        float f = *(float*)(mMipmapImage->data());
        Engine::sPropertyManager()->setByFullname(f, mCopyLastMipmapTo);
    }
    
}

//------------------------------------------------------------------------------
bool PostProcessFXInOutCopy::parseXmlSettings(TiXmlElement* root)
{
    // we only work on ppfx tags
    if (root == NULL) return false;
    if (strcmp(root->Value(), "postprocess")) return false;

    // iterate through childrens of this node and check for their values
    for( TiXmlElement* child = root->FirstChildElement(); child; child = child->NextSiblingElement() )
    {   
        if (!strcmp(child->Value(), "readback"))
        {
            if (child->Attribute("variable")) mCopyLastMipmapTo = child->Attribute("variable");
            if (child->Attribute("mipmap")) mMipmapLevel = boost::lexical_cast<int>(child->Attribute("mipmap"));
        }
    }
    
    // now let parent class parse it
    return PostProcessFX::parseXmlSettings(root);
    
}

#endif

