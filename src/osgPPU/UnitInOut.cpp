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

#include <osgPPU/UnitInOut.h>
#include <osgPPU/Processor.h>

#include <osg/Texture2D>

namespace osgPPU
{
    //------------------------------------------------------------------------------
    UnitInOut::UnitInOut(const UnitInOut& unit, const osg::CopyOp& copyop) :
        Unit(unit, copyop),
        mFBO(unit.mFBO),
        mNumLevels(unit.mNumLevels),
        mbUseMipmaps(unit.mbUseMipmaps),
        mbUseMipmapShader(unit.mbUseMipmapShader),
        mMipmapShader(unit.mMipmapShader),
        mMipmapFBO(unit.mMipmapFBO),
        mMipmapViewport(unit.mMipmapViewport)
    {
        setMipmappedInOut(unit.getMipmappedInOut());
    }
    
    //------------------------------------------------------------------------------
    UnitInOut::UnitInOut(osg::State* state) : Unit(state)
    {
        // disable mipmapping per default
        mbUseMipmaps = false;
        mbUseMipmapShader = false;
        mNumLevels = 0;
            
        // create FBO because we need it
        mFBO = new osg::FrameBufferObject();
    
        // if the input does have mipmaps, then they will be passed to the output
        setMipmappedInOut(false);
        mNumLevels = 0;
    }
    
    //------------------------------------------------------------------------------
    UnitInOut::UnitInOut() : Unit()
    {
        // disable mipmapping per default
        mbUseMipmaps = false;
        mbUseMipmapShader = false;
        mNumLevels = 0;
        
        // create FBO because we need it
        mFBO = new osg::FrameBufferObject();
    
        // if the input does have mipmaps, then they will be passed to the output
        setMipmappedInOut(false);
    }
    
    //------------------------------------------------------------------------------
    UnitInOut::~UnitInOut()
    {
    
    }
    
    //------------------------------------------------------------------------------
    void UnitInOut::assignFBO()
    {
        if (mFBO.valid())
        {
            // TODO: currently disabled, because there are some unresolved bugs if using fbo here
            //sScreenQuad->getOrCreateStateSet()->setAttribute(mFBO.get(), osg::StateAttribute::ON);
        }
    }

    //------------------------------------------------------------------------------
    void UnitInOut::init()
    {
        // setup output textures, which will change the size
        assignOutputTexture();
        assignFBO();

        // check if mipmapping is enabled, then enable mipmapping on output textures
        if (mbUseMipmaps) enableMipmapGeneration();
        
        // initialize all parts of the ppu
        Unit::init();
    }
    
    //--------------------------------------------------------------------------
    void UnitInOut::enableMipmapGeneration()
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
    void UnitInOut::generateMipmaps(osg::Texture* output, int mrt)
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
        if (dynamic_cast<osg::Texture2D*>(output) == NULL)
        {
            osg::notify(osg::WARN) << "Unit " << getName() << " cannot generate mipmaps because the output texture is not of type osg::Texture2D"<< std::endl;
            return;
        }
        
        // check if we have generated all the fbo's for each mipmap level
        int width = output->getTextureWidth();
        int height = output->getTextureHeight();
        int numLevel = 1 + (int)floor(log((float)std::max(width, height))/(float)M_LN2);
        
        // before we start generating of mipmaps we save some data
        osg::ref_ptr<osg::FrameBufferObject> currentFBO = mFBO;
        osg::ref_ptr<osg::Viewport> currentViewport = mViewport;
        osg::ref_ptr<osg::Texture> currentInputTex = mInputTex[0];
        osg::ref_ptr<Shader> currentShader = mShader;

        // generate fbo for each mipmap level
        if ((int)mMipmapFBO.size() != numLevel)
        {
            // generate mipmap levels
            mMipmapFBO.clear();
            mMipmapViewport.clear();
            for (int i=0; i < numLevel; i++)
            {
                // generate viewport
                osg::Viewport* vp = new osg::Viewport();
                int w = std::max(1, (int)floor(float(width) / float(pow(2.0f, (float)i)) ));
                int h = std::max(1, (int)floor(float(height) / float(pow(2.0f, (float)i)) ));
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

        // set global shader variable
        if (mShader.valid()) mShader->set("g_MipmapLevelNum", float(numLevel));

        // now we render the result in a loop to generate mipmaps
        for (int i=1; i < numLevel; i++)
        {
            // set mipmap level
            if (mShader.valid()) mShader->set("g_MipmapLevel", float(i));

            // assign new viewport
            mViewport = mMipmapViewport[i];
            assignViewport();

            // assign new fbo
            mFBO = mMipmapFBO[i];
            assignFBO();

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
        assignViewport();

        mFBO = currentFBO;
        assignFBO();
    }
    
    //------------------------------------------------------------------------------
    void UnitInOut::noticeFinishRendering()
    {    
        // generate mipmaps if such are required (for each output texture)
        if (mbUseMipmaps)
        {
            std::map<int, osg::ref_ptr<osg::Texture> >::iterator it = mOutputTex.begin();
            for (; it != mOutputTex.end(); it++)
            {
                generateMipmaps(it->second.get(), it->first);
            }
        }

        // TODO: remove this later. Have no real idea why it is required, it seems that somehwere it is not restored properly        
        if (sState.getState())
        {
            osg::FBOExtensions* fbo_ext = osg::FBOExtensions::instance(sState.getContextID(),true);
            fbo_ext->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

            // set default texture unit to 0
            sState.getState()->setActiveTextureUnit(0);
        }
    }
     
    //------------------------------------------------------------------------------
    void UnitInOut::assignOutputTexture()
    {
        if (mFBO.valid())
        {
            // now generate output texture's and assign them to fbo 
            std::map<int, osg::ref_ptr<osg::Texture> >::iterator it = mOutputTex.begin();
            for (int i = 0; it != mOutputTex.end(); it++, i++)
            {
                // initialze the resulting texture
                osg::Texture2D* mTex = dynamic_cast<osg::Texture2D*>(it->second.get());
                if (mTex == NULL)
                {
                    mTex = new osg::Texture2D();
                    mTex->setTextureSize(int(mViewport->width()), int(mViewport->height()) );
                    mTex->setResizeNonPowerOfTwoHint(false);
                    mTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP);
                    mTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP);               
                    mTex->setInternalFormat(getOutputInternalFormat());
                    mTex->setSourceFormat(Processor::createSourceTextureFormat(getOutputInternalFormat()));
    
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
            }
    
            // clean mipmap data for output
            mMipmapFBO.clear();
            mMipmapViewport.clear();
    
            // recheck the mipmap bypass data structures
            checkIOMipmappedData();
    
        }
        mbDirtyOutputTextures = false;
    }
    
    
    //------------------------------------------------------------------------------
    void UnitInOut::checkIOMipmappedData()
    {
        if (mFBO.valid() && mOutputTex.size() > 0 && mbMipmappedIO)
        {
            // do only proceed if output texture is valid
            if (mOutputTex.begin()->second == NULL) return;
    
            // clean viewport data
            mIOMipmapViewport.clear();
            mIOMipmapFBO.clear();
        
            // get dimensions of the output data
            int width = (mOutputTex.begin()->second)->getTextureWidth();
            int height = (mOutputTex.begin()->second)->getTextureHeight();
            int numLevels = 1 + (int)floor(log((float)std::max(width, height))/(float)M_LN2);
            mNumLevels = numLevels;
    
            // generate fbo for each mipmap level 
            for (int level=0; level < numLevels; level++)
            {
                // generate viewport for this level
                osg::Viewport* vp = new osg::Viewport();
                int w = std::max(1, (int)floor(float(width) / float(pow(2.0f, (float)level)) ));
                int h = std::max(1, (int)floor(float(height) / float(pow(2.0f, (float)level)) ));
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
        
                    // check if we have generated all the fbo's for each mipmap level
                    int _width = output->getTextureWidth();
                    int _height = output->getTextureHeight();
    
                    // if width and height are not equal, then we give some error and stop here 
                    if ((_width != width || _height != height))
                    {
                        printf("UnitInOut %s: output textures has different dimensions\n", getName().c_str());
                        return; 
                    }
        
                    // set fbo of current level with to this output         
                    fbo->setAttachment(GL_COLOR_ATTACHMENT0_EXT + mrt, osg::FrameBufferAttachment(output.get(), level));
                }
    
                // store fbo
                mIOMipmapFBO.push_back(fbo);
            }
        }
    }
    
    
    //------------------------------------------------------------------------------
    void UnitInOut::render(int mipmapLevel)
    {
        // if we do generate mipmaps
        if (mipmapLevel >= 0)
        {
            doRender(mipmapLevel);
            return;
        }
    
        // do following only if we have mipmapped io
        if (getMipmappedInOut())
        {
            // store current viewport 
            osg::Viewport* currentViewport = mViewport.get();
            osg::FrameBufferObject* currentFBO = mFBO.get();
    
            // otherwise we want to bypass mipmaps, thus do following for each mipmap level
            for (int i=0; i < mNumLevels; i++)
            {
                // assign new viewport and fbo
                mViewport = mIOMipmapViewport[i].get();
                mFBO = mIOMipmapFBO[i].get();

                assignViewport();
                assignFBO();

                // render the content
                doRender(i);
            }
        
            // restore current viewport and fbo 
            mFBO = currentFBO;
            mViewport = currentViewport;

            assignViewport();
            assignFBO();
    
        // otherwise just render
        }else{
            doRender(mipmapLevel);
        }
    }
    
    //------------------------------------------------------------------------------
    void UnitInOut::setMipmappedInOut(bool b)
    {
        mbDirtyOutputTextures = b;
        mbMipmappedIO = b;
        if (b) enableMipmapGeneration();
    }
    
    //------------------------------------------------------------------------------
    void UnitInOut::doRender(int mipmapLevel)
    {
        // return if we do not get valid state
        if (!sState.getState()) return;
    
        // can only be done on valid data 
        if (mFBO.valid() && mViewport.valid())
        {
            // just to be sure to pass correct values
            if (mipmapLevel < 0) mipmapLevel = 0;
    
            // update shaders manually, because they are do not updated from scene graph
            if (mShader.valid()) 
            {
                mShader->set("g_ViewportWidth", (float)mViewport->width());
                mShader->set("g_ViewportHeight", (float)mViewport->height());
                mShader->set("g_MipmapLevel", float(mipmapLevel));
                mShader->update();
            }
    
            // aplly stateset
            sState.getState()->apply(sScreenQuad->getStateSet());

            // TODO: should be removed here and be handled by the stateset directly (see assignFBO() )
            sState.getState()->applyAttribute(mFBO.get());

            // render the content of the input texture into the frame buffer
            if (getUseBlendMode() && getOfflineMode() == false)
            {
                glEnable(GL_BLEND);
                glColor4f(1,1,1, getBlendValue());
                sScreenQuad->draw(sState);
                glDisable(GL_BLEND);
                glColor4f(1,1,1,1);
            }else{
                glDisable(GL_BLEND);
                glColor4f(1,1,1,1);
                sScreenQuad->draw(sState);
                glColor4f(1,1,1,1);
            }
        }    
    }
    
    
    //------------------------------------------------------------------------------
    void UnitInOut::noticeChangeViewport()
    {
        // change size of the result texture according to the viewport
        TextureMap::iterator it = mOutputTex.begin();
        for (; it != mOutputTex.end(); it++)
        {
            if (it->second.valid())
            {
                // currently we are working only with 2D textures
                if (dynamic_cast<osg::Texture2D*>(it->second.get()) == NULL)
                {
                    osg::notify(osg::WARN)<<"Unit " << getName() << " support only Texture2D " << std::endl;    
                }else{
                    // change size
                    osg::Texture2D* mTex = dynamic_cast<osg::Texture2D*>(it->second.get());
                    mTex->setTextureSize(int(mViewport->width()), int(mViewport->height()) );
                }
            }
        }
    }

}; // end namespace
