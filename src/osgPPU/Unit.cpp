/***************************************************************************
 *   Copyright (c) 2008   Art Tevs                                         *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation; either version 3 of        *
 *   the License, or (at your option) any later version.                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Lesse General Public License for more details.                    *
 *                                                                         *
 *   The full license is in LICENSE file included with this distribution.  *
 ***************************************************************************/

#include <osgPPU/Unit.h>
#include <osgPPU/Processor.h>

#include <osg/Texture2D>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osg/Image>
#include <math.h>

namespace osgPPU
{

//------------------------------------------------------------------------------
Unit::Unit()
{
    initialize();
}

//------------------------------------------------------------------------------
Unit::Unit(osg::State* state)
{
    initialize();
    setState(state);
}

//------------------------------------------------------------------------------
void Unit::initialize()
{
    setName("__Nameless_PPU_");
    mUserData = NULL;
    mInputTexIndexForViewportReference = 0;
    mbDirtyShader = false;

    // we do steup defaults
    setBlendStartTime(0);
    setBlendFinalTime(0);
    setBlendStartValue(1);
    setBlendFinalValue(1);
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
    osg::Vec4Array* screenQuadColor = new osg::Vec4Array(1);
    (*screenQuadColor)[0].set(1.0f,1.0f,1.0,1.0f);
    sScreenQuad->setColorArray(screenQuadColor);
    sScreenQuad->setColorBinding(osg::Geometry::BIND_OFF);

    // assign empty state set
    sScreenQuad->setStateSet(new osg::StateSet());
    sScreenQuad->setUseDisplayList(false);
    setUseBlendMode(false);

    // the viewport is dirty, hence it will be resetted by the next apply
    mbDirtyViewport = true;
        
    // initialze projection matrix
    sProjectionMatrix = osg::Matrix::ortho(0,1,0,1,0,1);
    
    // setup default modelview matrix
    sModelviewMatrix = osg::Matrixf::identity();

}

//------------------------------------------------------------------------------
Unit::Unit(const Unit& ppu, const osg::CopyOp& copyop) :
    osg::Object(ppu, copyop),
    mInputTex(ppu.mInputTex),
    mOutputTex(ppu.mOutputTex),
    mShader(ppu.mShader),
    mIndex(ppu.mIndex),
    sScreenQuad(ppu.sScreenQuad),
    sProjectionMatrix(ppu.sProjectionMatrix),
    sModelviewMatrix(ppu.sModelviewMatrix),
    sState(ppu.sState),
    mViewport(ppu.mViewport),
    mInputPPU(ppu.mInputPPU),
    mUseInputPPUViewport(ppu.mUseInputPPUViewport),
    mUpdateCallback(ppu.mUpdateCallback),
    mbDirtyViewport(ppu.mbDirtyViewport),
    mbDirtyInputTextures(ppu.mbDirtyInputTextures),
    mbDirtyOutputTextures(ppu.mbDirtyOutputTextures),
    mbDirtyShader(ppu.mbDirtyShader),
    mbOfflinePPU(ppu.mbOfflinePPU),
    mOutputInternalFormat(ppu.mOutputInternalFormat),
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
Unit::~Unit()
{
}

//------------------------------------------------------------------------------
void Unit::setRenderingFrustum(float left, float top, float right, float bottom)
{
    sProjectionMatrix = osg::Matrix::ortho2D(left, right, bottom, top);
}

//------------------------------------------------------------------------------
void Unit::setInputTextureIndexForViewportReference(int index)
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
void Unit::setInputTexture(osg::Texture* inTex, int inputIndex)
{
    if (inTex)
	{
        mInputTex[inputIndex] = inTex;
		mbDirtyInputTextures = true;
        noticeChangeInput();
	}
}

//------------------------------------------------------------------------------
void Unit::bindInputTextureToUniform(int index, const std::string& name)
{
    osg::Texture* tex = getInputTexture(index);
    if (mShader.valid() && tex)
    {
        mShader->bind(name, tex, index);
    }
}

//------------------------------------------------------------------------------
void Unit::setOutputTexture(osg::Texture* outTex, int mrt)
{
    if (outTex)
        mOutputTex[mrt] = outTex;
    else
        mOutputTex[mrt] = osg::ref_ptr<osg::Texture>(NULL);

	mbDirtyOutputTextures = true;
}

//--------------------------------------------------------------------------
void Unit::apply(float dTime)
{
    // call update callback of the ppu
    if (mUpdateCallback.valid()) (*mUpdateCallback)(this);

    // update time value 
    mTime += dTime;

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

	// check if input textures are dirty
	if (mbDirtyInputTextures || mbDirtyShader)
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
    //TextureMap::const_iterator it = mInputTex.begin();
    //for (; it != mInputTex.end(); it++)
    //{
    //    if (it->second.valid())
    //    {
            // apply local state if it differs from parent state 
    //        if (sState.getState()) sState.getState()->apply(sScreenQuad->getStateSet());
            
            // apply current opengl matrix
            glMatrixMode( GL_PROJECTION ); glLoadMatrixf(sProjectionMatrix.ptr());
            glMatrixMode( GL_MODELVIEW );  glLoadMatrixf(sModelviewMatrix.ptr());

            // call rendering method if apply was successfull
            if (applyBaseRenderParameters()) render();
    
            // this is enough to do this once, hence break here
    //        break;
    //    }
    //}

    // notice we are done
    noticeFinishRendering();
}

//--------------------------------------------------------------------------
bool Unit::applyBaseRenderParameters()
{
    // only update if active 
    if (!getActive()) return false;
    
    // the fx should be shown
    if (getBlendStartTime() <= mTime)
    {
        // blending factor
        float factor = 1.0;
        
        // if we get 0 as expire time, so the factor is the start value 
        if (getBlendFinalTime() < 0.0001)
            factor = 0;
        else
            factor = (mTime - getBlendStartTime()) / (getBlendFinalTime() - getBlendFinalTime());
        
        // compute blend value for the ppu
        float alpha = getBlendStartValue()*(1-factor) + factor*getBlendFinalValue();
        if (alpha > 1.0 ) { alpha = 1.0; setBlendFinalTime(0); setBlendStartValue(1.0); }
        if (alpha < 0.0 ) { alpha = 0.0; setBlendFinalTime(0); setBlendStartValue(0.0); }
        
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
void Unit::setUseBlendMode(bool enable)
{
    osg::StateSet* ss = sScreenQuad->getOrCreateStateSet();
    ss->setAttribute(mBlendFunc.get(), enable ? osg::StateAttribute::ON : osg::StateAttribute::OFF);
    ss->setMode(GL_BLEND, enable ? osg::StateAttribute::ON : osg::StateAttribute::OFF);
}

//--------------------------------------------------------------------------
bool Unit::getUseBlendMode() const
{
    osg::StateAttribute::GLModeValue mode = sScreenQuad->getOrCreateStateSet()->getMode(GL_BLEND);
    return (mode & osg::StateAttribute::ON) == osg::StateAttribute::ON;
}

//--------------------------------------------------------------------------
void Unit::assignInputTexture()
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
void Unit::assignShader()
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
void Unit::removeShader()
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
void Unit::setViewport(osg::Viewport* vp)
{
    // if viewport is valid and we have to ignore new settings
    if ((mViewport.valid() && getInputTextureIndexForViewportReference() >=0)
        || vp == NULL)
        return;

    // otherwise setup new viewport
    mViewport = new osg::Viewport(*vp);
    mbDirtyViewport = true;
    assignViewport();
}

//--------------------------------------------------------------------------
void Unit::assignViewport()
{
    if (mViewport.valid())
    {
        sScreenQuad->getOrCreateStateSet()->setAttribute(mViewport.get(), osg::StateAttribute::ON);// | osg::StateAttribute::OVERRIDE);
    }
}

//--------------------------------------------------------------------------
void Unit::setOutputInternalFormat(GLenum format)
{
    mOutputInternalFormat = format;
    
    // now generate output texture's and assign them to fbo 
    TextureMap::iterator it = mOutputTex.begin();
    for (;it != mOutputTex.end(); it++)
    {
        if (it->second.valid()){
            it->second->setInternalFormat(mOutputInternalFormat);
            it->second->setSourceFormat(Processor::createSourceTextureFormat(mOutputInternalFormat));
        }
    }

}

//------------------------------------------------------------------------------
void Unit::init()
{
    // check if input PPU is specified
    for (int i=0; i < (int)mInputPPU.size(); i++)
    {
        if (mInputPPU[i] != NULL)
        {
            setInputTexture(mInputPPU[i]->getOutputTexture(0), i);
        }
    }
    
    // update viewport if we have one bound
    if (mUseInputPPUViewport != NULL)
    {
        setViewport(mUseInputPPUViewport->getViewport());
    }

    // check if we have input reference size 
    setInputTextureIndexForViewportReference(getInputTextureIndexForViewportReference());

    // mark all textures as dirty, so that they get reassigned later
    mbDirtyInputTextures = true;
    mbDirtyOutputTextures = true;
}


}; // end namespace

