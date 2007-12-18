/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <osgPPU/PPUInOut.h>
#include <osg/Texture2D>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osg/Image>
#include <assert.h>
#include <osgUtil/UpdateVisitor>

//--------------------------------------------------------------------------
// PostProcessUnitOut Implementation
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
PostProcessUnitOut::PostProcessUnitOut(osg::State* s, osg::StateSet* ss) : PostProcessUnit(s,ss)
{

}

//------------------------------------------------------------------------------
PostProcessUnitOut::~PostProcessUnitOut()
{

}

//------------------------------------------------------------------------------
void PostProcessUnitOut::init()
{
    // assign the input texture and shader if they are valid
    assignInputTexture();
    assignShader();
    
    initializeBase();
}


//------------------------------------------------------------------------------
void PostProcessUnitOut::render(int mipmapLevel)
{
    // return if we do not get valid state
    if (!sState.getState()) return;
        
    // apply viewport if such is valid
    if (mViewport.valid()) mViewport->apply(*sState.getState());
        
    // render the content of the input texture into the frame buffer
    if (useBlendMode())
    {
        //glEnable(GL_BLEND);
        glColor4f(1,1,1, getCurrentBlendValue());
    }
    sScreenQuad->draw(sState);
    if (useBlendMode())
    {
        //glDisable(GL_BLEND);
        glColor4f(1,1,1,1);
    }   
}


//--------------------------------------------------------------------------
// PostProcessUnitOutCapture Implementation
//--------------------------------------------------------------------------
#if 0
//------------------------------------------------------------------------------
PostProcessUnitOutCapture::PostProcessUnitOutCapture(osg::State* s, osg::StateSet* ss) : PostProcessUnitOut(s,ss)
{
}

//------------------------------------------------------------------------------
PostProcessUnitOutCapture::~PostProcessUnitOutCapture()
{
}

//------------------------------------------------------------------------------
void PostProcessUnitOutCapture::init()
{
    // do initialization in the base
    PostProcessUnitOut::init();
}


//------------------------------------------------------------------------------
void PostProcessUnitOutCapture::render(int mipmapLevel)
{
}

//------------------------------------------------------------------------------
void PostProcessUnitOutCapture::noticeFinishRendering()
{
    if (isActive() && sState.getState())
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
        }
    }
}
#endif


//--------------------------------------------------------------------------
// PostProcessUnitInOut Implementation
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
PostProcessUnitInOut::PostProcessUnitInOut(osg::State* s, osg::StateSet* ss) : PostProcessUnit(s,ss)
{
    // create FBO because we need it
    mFBO = new osg::FrameBufferObject();

    // if the input does have mipmaps, then they will be passed to the output
    setMipmappedIO(false);
    mNumLevels = 0;

}

//------------------------------------------------------------------------------
PostProcessUnitInOut::~PostProcessUnitInOut()
{

}

//------------------------------------------------------------------------------
void PostProcessUnitInOut::init()
{
    initializeBase();

    assignShader();
    assignOutputTexture();
}

//------------------------------------------------------------------------------
void PostProcessUnitInOut::assignOutputTexture()
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
                mTex->setSourceFormat(PostProcessUnit::createSourceTextureFormat(getOutputInternalFormat()));

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
void PostProcessUnitInOut::checkIOMipmappedData()
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
        int numLevels = 1 + (int)floor(log2(std::max(width, height)));
        mNumLevels = numLevels;

        // generate fbo for each mipmap level 
        for (int level=0; level < numLevels; level++)
        {
            // generate viewport for this level
            osg::Viewport* vp = new osg::Viewport();
            int w = std::max(1, (int)floor(float(width) / float(pow(2,level)) ));
            int h = std::max(1, (int)floor(float(height) / float(pow(2,level)) ));
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
                    printf("PostProcessUnitInOut %s: output textures has different dimensions\n", getName().c_str());
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


//------------------------------------------------------------------------------
void PostProcessUnitInOut::render(int mipmapLevel)
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

        // otherwise we want to bypass mipmaps, thus do following for each mipmap level
        for (int i=0; i < mNumLevels; i++)
        {
            // set mipmap level
            if (mShaderMipmapLevelUniform.valid()) mShaderMipmapLevelUniform->set("g_MipmapLevel",float(i));
    
            // assign new viewport and fbo
            mViewport = mIOMipmapViewport[i];
            mFBO = mIOMipmapFBO[i];
    
            //printf("io-mipmap %s %d, (%dx%d) \n", getResourceName().c_str(), i, (int)mViewport->width(), (int)mViewport->height());
    
            // render the content
            doRender(i);
        }
    
        // restore current viewport and fbo 
        mFBO = currentFBO;
        mViewport = currentViewport;

    // otherwise just render
    }else{
        doRender(mipmapLevel);
    }
}

//------------------------------------------------------------------------------
void PostProcessUnitInOut::setMipmappedIO(bool b)
{
    mbDirtyOutputTextures = b;
    mbMipmappedIO = b;
}

//------------------------------------------------------------------------------
void PostProcessUnitInOut::doRender(int mipmapLevel)
{
    // return if we do not get valid state
    if (!sState.getState()) return;

    // can only be done on valid data 
    if (mFBO.valid() && mViewport.valid())
    {
        // update shaders manually, because they are do not updated from scene graph
        if (mShaderMipmapLevelUniform.valid() && mShaderMipmapLevelUniform->getUpdateCallback())
        {
            (*mShaderMipmapLevelUniform->getUpdateCallback())(mShaderMipmapLevelUniform.get(), new osgUtil::UpdateVisitor());
        }

        // aplly stateset
        sState.getState()->apply(sScreenQuad->getStateSet());
        
        // apply framebuffer object, this will bind it, so we can use it
        mFBO->apply(*sState.getState());
        
        // apply viewport
        mViewport->apply(*sState.getState());

        // render the content of the input texture into the frame buffer
        if (useBlendMode() && getOfflineMode() == false)
        {
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
    }
    
}


//------------------------------------------------------------------------------
void PostProcessUnitInOut::noticeChangeViewport()
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
        }
    }
}


//--------------------------------------------------------------------------
// PostProcessUnitInResampleOut Implementation
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
PostProcessUnitInResampleOut::PostProcessUnitInResampleOut(osg::State* s, osg::StateSet* ss) : PostProcessUnit(s,ss)
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
PostProcessUnitInResampleOut::~PostProcessUnitInResampleOut()
{

}

//------------------------------------------------------------------------------
void PostProcessUnitInResampleOut::setFactor(float w, float h)
{
    mWidthFactor = w;
    mHeightFactor = h;
    mDirtyFactor = true;
}


//------------------------------------------------------------------------------
void PostProcessUnitInResampleOut::init()
{
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
                mTex->setSourceFormat(PostProcessUnit::createSourceTextureFormat(getOutputInternalFormat()));
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
}

//------------------------------------------------------------------------------
void PostProcessUnitInResampleOut::noticeChangeViewport()
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
void PostProcessUnitInResampleOut::render(int mipmapLevel)
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

        // return if we do not get valid state
        if (!sState.getState()) return;
    
        // can only be done on valid data 
        if (mFBO.valid() && mViewport.valid())
        {
            // setup shader global properties
            //if (mShader.valid()) mShader->set("g_TextureWidth", (float)(mViewport->x() + mViewport->width()));
            //if (mShader.valid()) mShader->set("g_TextureHeight",(float)(mViewport->y() + mViewport->height()));
            
            // update shaders manually, because they are do not updated from scene graph
            //if (mShader.valid()) mShader->update();
            
            // aplly stateset
            sState.getState()->apply(sScreenQuad->getStateSet());
                
            // apply framebuffer object, this will bind it, so we can use it
            mFBO->apply(*sState.getState());
    
            // apply viewport
            mViewport->apply(*sState.getState());
    
            // render the content of the input texture into the frame buffer
            glColor4f(1,1,1,1);
            sScreenQuad->draw(sState);      
        }

}


