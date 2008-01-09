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

#include <osgPPU/PPUInOut.h>
#include <osgPPU/PostProcess.h>

#include <osg/Texture2D>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osg/Image>
#include <assert.h>
#include <osgUtil/UpdateVisitor>

#include <iostream>

namespace osgPPU
{

//--------------------------------------------------------------------------
// PostProcessUnitOut Implementation
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
PostProcessUnitOut::PostProcessUnitOut(PostProcess* parent) : PostProcessUnit(parent)
//osg::State* s, osg::StateSet* ss) : PostProcessUnit(s,ss)
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
    //assignInputTexture();
    //assignShader();
    
    initializeBase();
}


//------------------------------------------------------------------------------
void PostProcessUnitOut::render(int mipmapLevel)
{
    // return if we do not get valid state
    if (!sState.getState()) return;

    // setup shader values
    if (mShader.valid()) 
    {
        mShader->set("g_ViewportWidth", (float)mViewport->width());
        mShader->set("g_ViewportHeight", (float)mViewport->height());
        mShader->set("g_MipmapLevel", float(mipmapLevel));
        mShader->update();
    }

    // aplly stateset
    sState.getState()->apply(sScreenQuad->getStateSet());

    // HACK: need this sometimes otherwise the wrong mipmap level is readed
    if (mipmapLevel > 0)
    {
        for (TextureMap::iterator it = mInputTex.begin();it!= mInputTex.end(); it++)
        {
            sState.getState()->applyTextureAttribute((*it).first, (*it).second.get());

            GLenum target = (*it).second->getTextureTarget();
    
            glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, mipmapLevel-1);
            glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, mipmapLevel-1);
        }
    }

    // apply viewport if such is valid
    if (mViewport.valid()) mViewport->apply(*sState.getState());

    // render the content of the input texture into the frame buffer
    if (useBlendMode())
    {
        glEnable(GL_BLEND);
        glColor4f(1,1,1, getCurrentBlendValue());
    }
    sScreenQuad->draw(sState);
    if (useBlendMode())
    {
        glDisable(GL_BLEND);
        glColor4f(1,1,1,1);
    }   


    // HACK
    if (mipmapLevel > 0)
    {
        for (TextureMap::iterator it = mInputTex.begin();it!= mInputTex.end(); it++)
        {
            GLenum target = (*it).second->getTextureTarget();
    
            glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 1000);
        }
    }

}


//--------------------------------------------------------------------------
// PostProcessUnitOutCapture Implementation
//--------------------------------------------------------------------------
//------------------------------------------------------------------------------
PostProcessUnitOutCapture::PostProcessUnitOutCapture(PostProcess* parent) : PostProcessUnitOut(parent)
{
    mPath = ".";
    mCaptureNumber = 0;
    mExtension = "png";
}

//------------------------------------------------------------------------------
PostProcessUnitOutCapture::~PostProcessUnitOutCapture()
{
}


//------------------------------------------------------------------------------
void PostProcessUnitOutCapture::noticeFinishRendering()
{
    if (isActive() && sState.getState())
    {
        // if we want to capture the framebuffer
        char filename[256];
        
        // for each input texture do
        for (unsigned int i=0; i < mInputTex.size(); i++)
        {
            // create file name
            sprintf( filename, "%s/%d_%04d.%s", mPath.c_str(), i, mCaptureNumber, mExtension.c_str());
            std::cout << "Capture " << mCaptureNumber << " frame to " << filename << " ...";
            std::cout.flush();
           
            mCaptureNumber++;

            // input texture 
            osg::Texture* input = getInputTexture(i);

            // bind input texture, so that we can get image from it
            if (input != NULL) input->apply(*sState.getState());
            
            // retrieve texture content
            osg::ref_ptr<osg::Image> img = new osg::Image();
            img->readImageFromCurrentTexture(sState.getContextID(), false); 
            //img->readPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE);
            osgDB::ReaderWriter::WriteResult res = osgDB::Registry::instance()->writeImage(*img, filename, NULL);
            //write_png(filename, img->data(), w, h, 4, 8, PNG_COLOR_TYPE_RGBA, 1);
            if (res.success())
                std::cout << " OK" << std::endl;
            else
                std::cout << " failed! (" << res.message() << ")" << std::endl;
                        
            // unbind the texture back 
            if (input != NULL)
                sState.getState()->applyTextureMode(0, input->getTextureTarget(), false);
        }
    }
}


//--------------------------------------------------------------------------
// PostProcessUnitInOut Implementation
//--------------------------------------------------------------------------

//------------------------------------------------------------------------------
//PostProcessUnitInOut::PostProcessUnitInOut(osg::State* s, osg::StateSet* ss) : PostProcessUnit(s,ss)
PostProcessUnitInOut::PostProcessUnitInOut(PostProcess* parent) : PostProcessUnit(parent)
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
    //assignShader();

    // assigne input textures
    //assignInputTexture();

    // now force to reset the viewport, since input should be ok now
    //setInputTextureIndexForViewportReference(getInputTextureIndexForViewportReference());

    // setup output textures, which will change the size
    assignOutputTexture();

    // initialize all parts of the ppu
    initializeBase();
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
                mTex->setTextureSize(int(mViewport->width()), int(mViewport->height()) );
                mTex->setResizeNonPowerOfTwoHint(false);
                mTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP);
                mTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP);               
                mTex->setInternalFormat(getOutputInternalFormat());
                mTex->setSourceFormat(PostProcess::createSourceTextureFormat(getOutputInternalFormat()));

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
            //setViewport(*mViewport);
        }
        //sScreenQuad->getOrCreateStateSet()->setAttribute(mViewport.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

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
            // assign new viewport and fbo
            mViewport = mIOMipmapViewport[i];
            mFBO = mIOMipmapFBO[i];
    
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
    if (b) enableMipmapGeneration();
}

//------------------------------------------------------------------------------
void PostProcessUnitInOut::doRender(int mipmapLevel)
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

        /*printf("render %s-%s: %dx%d (%d)\n", getName().c_str(),
            mShader.valid() ? mShader->getName().c_str() : "nil", (int)mViewport->width(),
            (int)mViewport->height(), mipmapLevel);
        */

        // aplly stateset
        sState.getState()->apply(sScreenQuad->getStateSet());

        // apply framebuffer object, this will bind it, so we can use it
        mFBO->apply(*sState.getState());

        // HACK: need this sometimes otherwise the wrong mipmap level is readed
        if (mipmapLevel > 0)
        {
            for (TextureMap::iterator it = mInputTex.begin();it!= mInputTex.end(); it++)
            {
                sState.getState()->applyTextureAttribute((*it).first, (*it).second.get());
    
                GLenum target = (*it).second->getTextureTarget();
        
                glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, mipmapLevel-1);
                glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, mipmapLevel-1);
            }
        }
        
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

        // HACK
        if (mipmapLevel > 0)
        {
            for (TextureMap::iterator it = mInputTex.begin();it!= mInputTex.end(); it++)
            {
                GLenum target = (*it).second->getTextureTarget();
        
                glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
                glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 1000);
            }
        }

        #if 0
        // TEST
        if (getName() == "AdaptedLuminance")
        {
            // get maximal level count
            int width = mOutputTex[0]->getTextureWidth();
            int height = mOutputTex[0]->getTextureHeight();
            int numLevels = 1 + (int)floor(log2(std::max(width, height)));
        
            // the mipmap level we want to read from is
            int level = numLevels  - 1;

            // create image to hold result, if no such already exists
            static osg::ref_ptr<osg::Image> mMipmapImage = new osg::Image();
        
            // compute the w and h based on the level
            int w = std::max(1, (int)floor(float(width) / float(pow(2,level)) ));
            int h = std::max(1, (int)floor(float(height) / float(pow(2,level)) ));
            
            // read out
            mFBO->apply(*sState.getState());
            glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
            mMipmapImage->readPixels(0, 0, w, h, GL_RGBA, GL_FLOAT);
    
            // store result in variable
            float r = ((float*)(mMipmapImage->data()))[0];
            float g = ((float*)(mMipmapImage->data()))[1];
            float b = ((float*)(mMipmapImage->data()))[2];
            float a = ((float*)(mMipmapImage->data()))[3];
    
            printf("read out %f %f %f %f\n", r,g,b,a);
    
            // disable fbo, since it was disabled before 
            osg::FBOExtensions* fbo_ext = osg::FBOExtensions::instance(sState.getContextID(),true);
            fbo_ext->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        }
        #endif
    }    
}


//------------------------------------------------------------------------------
void PostProcessUnitInOut::noticeChangeViewport()
{
    // change size of the result texture according to the viewport
    TextureMap::iterator it = mOutputTex.begin();
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
PostProcessUnitInResampleOut::PostProcessUnitInResampleOut(PostProcess* parent) : PostProcessUnitInOut(parent)
{
    // setup default values 
    mWidthFactor = 1.0;
    mHeightFactor = 1.0;
    mDirtyFactor = true;
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
void PostProcessUnitInResampleOut::render(int mipmapLevel)
{
    // if we have to reset the resampling factor
    if (mDirtyFactor)
    {
        // there must be an viewport
        assert(mViewport.valid());

        // force to reset the input referrence size 
        setInputTextureIndexForViewportReference(getInputTextureIndexForViewportReference());

        // setup new viewport size
        mViewport->width() *= mWidthFactor;
        mViewport->height() *= mHeightFactor;
        mDirtyFactor = false;

        // notice that we changed the viewport
        noticeChangeViewport();
    }

    // do rendering as usual
    PostProcessUnitInOut::render();
}

}; // end namespace
