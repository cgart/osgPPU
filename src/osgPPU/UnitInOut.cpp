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
#include <osgPPU/Utility.h>

#include <osg/Texture2D>
#include <osg/GL2Extensions>

namespace osgPPU
{
    //------------------------------------------------------------------------------
    // Helper class for filling the generated texture with default pixel values
    //------------------------------------------------------------------------------
    class SubloadCallback : public osg::Texture2D::SubloadCallback
    {
        public:
            // fill texture with default pixel values 
            void load (const osg::Texture2D &texture, osg::State &state) const
            {
                // if fbo is supported, then perform quick clearing of texture
                #if 0
                osg::FBOExtensions* fbo_ext = osg::FBOExtensions::instance(state.getContextID(),true);
                if (false && fbo_ext && fbo_ext->isSupported())
                {
                    // create the texture in usual OpenGL way
                    glTexImage2D( GL_TEXTURE_2D, 0, texture.getInternalFormat(),
                        texture.getTextureWidth(), texture.getTextureHeight(), texture.getBorderWidth(),
                        texture.getSourceFormat() ? texture.getSourceFormat() : texture.getInternalFormat(),
                        texture.getSourceType() ? texture.getSourceType() : GL_UNSIGNED_BYTE,
                        NULL);          

                    // create temporary fbo
                    GLuint fboID = 0;
                    fbo_ext->glGenFramebuffersEXT(1, &fboID);
                    if (fboID == 0)
                    {
                        osg::notify(osg::WARN) << "osgPPU::UnitInOut::SubloadCallback() - FrameBufferObject: could not create the FBO" << std::endl;
                        return;
                    }

                    // get texture object
                    osg::Texture::TextureObject *tobj = texture.getTextureObject(state.getContextID());
                    if (!tobj || tobj->_id == 0)
                    {
                        osg::notify(osg::WARN) << "osgPPU::UnitInOut::SubloadCallback() - Could not get the according texture object" << std::endl;
                        return;
                    }

                    // bind fbo and attach the texture
                    fbo_ext->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboID);
                    fbo_ext->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tobj->_id, 0);
                    GLenum fbostatus = fbo_ext->glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
                    if (fbostatus != GL_FRAMEBUFFER_COMPLETE_EXT)
                    {
                        printf("STATUS %d\n", fbostatus);
                        printf("%dx%d - %d, %d\n",texture.getTextureWidth(), texture.getTextureHeight(),
                        texture.getSourceFormat() ? texture.getSourceFormat() : texture.getInternalFormat(),
                        texture.getSourceType() ? texture.getSourceType() : GL_UNSIGNED_BYTE);
                    }

                    // push current opengl state and prepare for clearing
                    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);
                    glViewport(0,0,texture.getTextureWidth(),texture.getTextureHeight());

                    // clear the texture, this will force to write 0s to the texture
                    glClearColor(1,0,0,0);
                    glClear(GL_COLOR_BUFFER_BIT);

                    // restore opengl state
                    glPopAttrib();

                    // cleanup the fbo 
                    fbo_ext->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
                    fbo_ext->glDeleteFramebuffersEXT(1, &fboID);

                // fbo is not supported, then do fill the texture with values in classical way
                }else
                #endif
                {
                    // create temporary image which is initialized with 0 values
                    osg::ref_ptr<osg::Image> img = new osg::Image();
                    img->allocateImage(texture.getTextureWidth(), texture.getTextureHeight(), 1, 
                        texture.getSourceFormat() ? texture.getSourceFormat() : texture.getInternalFormat(), 
                        texture.getSourceType() ? texture.getSourceType() : GL_UNSIGNED_BYTE);

                    // fill the image with 0 values
                    memset(img->data(), 0, img->getTotalSizeInBytesIncludingMipmaps() * sizeof(unsigned char));

                    // create the texture in usual OpenGL way
                    glTexImage2D( GL_TEXTURE_2D, 0, texture.getInternalFormat(),
                        texture.getTextureWidth(), texture.getTextureHeight(), texture.getBorderWidth(),
                        texture.getSourceFormat() ? texture.getSourceFormat() : texture.getInternalFormat(),
                        texture.getSourceType() ? texture.getSourceType() : GL_UNSIGNED_BYTE,
                        img->data());          
                }
            }

            // no subload, because while we want to subload the texture should be already valid
            void subload (const osg::Texture2D &texture, osg::State &state) const 
            {

            }
    };


    //------------------------------------------------------------------------------
    UnitInOut::UnitInOut(const UnitInOut& unit, const osg::CopyOp& copyop) :
        Unit(unit, copyop),
        mFBO(unit.mFBO),
        mBypassedInput(unit.mBypassedInput)
    {
    }
    
    //------------------------------------------------------------------------------
    UnitInOut::UnitInOut() : Unit(),
        mBypassedInput(-1)
    {
        mFBO = new osg::FrameBufferObject();
    }
    
    //------------------------------------------------------------------------------
    UnitInOut::~UnitInOut()
    {
    
    }
    
    //------------------------------------------------------------------------------
    void UnitInOut::assignFBO()
    {
        // TODO: currently disabled, because there are some unresolved bugs if using fbo here
        getOrCreateStateSet()->setAttribute(mFBO.get(), osg::StateAttribute::ON);
    }

    //------------------------------------------------------------------------------
    void UnitInOut::init()
    {
        // initialize all parts of the ppu
        Unit::init();

        // setup a geode and the drawable as childs of this unit
        mDrawable = createTexturedQuadDrawable();
        mGeode->removeDrawables(0, mGeode->getNumDrawables());
        mGeode->addDrawable(mDrawable.get());

        // setup bypassed output if required
        if (mBypassedInput >= 0 && mBypassedInput < (int)getNumParents())
        {
            Unit* input = dynamic_cast<Unit*>(getParent(mBypassedInput));
            if (!input)
            {
                osg::notify(osg::WARN) << "osgPPU::UnitInOut::init() - cannot initialize bypassed input, because no input unit found" << std::endl;
                return;
            }
            mOutputTex[0] = input->getOrCreateOutputTexture(0);
        }

        // setup output textures and fbo
        assignOutputTexture();
        assignFBO();
    }
    
    //------------------------------------------------------------------------------
    void UnitInOut::setInputBypass(int index)
    {
        // nothing to do if not
        if (index == mBypassedInput) return;
        mBypassedInput = index;

        // if we want to remove bypassed index
        if (index < 0)
        {
            mOutputTex[0] = NULL;
            getOrCreateOutputTexture(0);

        // if we want to setup bypassed texture
        }else
        {
            mOutputTex[0] = getInputTexture(index);
        }

        dirty();
    }

    //------------------------------------------------------------------------------
    osg::Texture* UnitInOut::getOrCreateOutputTexture(int mrt)
    {
        // if already exists, then return back
        osg::Texture* tex = mOutputTex[mrt].get();
        if (tex) return tex;

        // if not exists, then do allocate it
        osg::Texture2D* mTex = new osg::Texture2D();
        mTex->setResizeNonPowerOfTwoHint(false);
        mTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        mTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        mTex->setInternalFormat(getOutputInternalFormat());
        mTex->setSourceFormat(createSourceTextureFormat(getOutputInternalFormat()));
        mTex->setBorderColor(osg::Vec4(0,0,0,0));

        // check if the input texture was in nearest mode
        if (getInputTexture(0) && getInputTexture(0)->getFilter(osg::Texture2D::MIN_FILTER) == osg::Texture2D::NEAREST)
            mTex->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::NEAREST);
        else
            mTex->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);

        if (getInputTexture(0) && getInputTexture(0)->getFilter(osg::Texture2D::MAG_FILTER) == osg::Texture2D::NEAREST)
            mTex->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::NEAREST);
        else
            mTex->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);

        // setup the upload callback to be applied when the texture is generated
        mTex->setSubloadCallback(new SubloadCallback());

        // set new texture
        mOutputTex[mrt] = mTex;

        return mTex;
    }

    //------------------------------------------------------------------------------
    void UnitInOut::assignOutputTexture()
    {
        // now generate output texture's and assign them to fbo 
        std::map<int, osg::ref_ptr<osg::Texture> >::iterator it = mOutputTex.begin();
        for (int i = 0; it != mOutputTex.end(); it++, i++)
        {
            // check whenever the output texture is a 2D texture 
            osg::Texture2D* mTex = dynamic_cast<osg::Texture2D*>(it->second.get());
            if (it->second.get() && mTex == NULL)
            {
                osg::notify(osg::WARN) << "osgPPU::UnitInOut::assignOutputTexture() - " << getName() << " currently only 2D output textures are supported" << std::endl;
                continue;
            }

            // if the output texture is NULL, hence generate one
            if (it->second.get() == NULL)
            {
                // preallocate the texture
                mTex = dynamic_cast<osg::Texture2D*>(getOrCreateOutputTexture(it->first));

                if (mViewport.valid())
                    mTex->setTextureSize(int(mViewport->width()), int(mViewport->height()) );        
                else
                {
                    osg::notify(osg::FATAL) << "osgPPU::UnitInOut::assignOutputTexture() - " << getName() << " cannot set output texture size, because viewport is invalid" << std::endl;
                    continue;
                }
            }

            // attach the texture to the fbo
            if (mTex)
                mFBO->setAttachment(GL_COLOR_ATTACHMENT0_EXT + i, osg::FrameBufferAttachment(mTex));
            else
                osg::notify(osg::FATAL) << "osgPPU::UnitInOut::assignOutputTexture() - " << getName() << " cannot attach output texture to FBO" << std::endl;
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
                if (dynamic_cast<osg::Texture2D*>(it->second.get()) != NULL)
                {
                //    osg::notify(osg::WARN)<<"Unit " << getName() << " support only Texture2D " << std::endl;    
                //}else{
                    // change size
                    osg::Texture2D* mTex = dynamic_cast<osg::Texture2D*>(it->second.get());
                    mTex->setTextureSize(int(mViewport->width()), int(mViewport->height()) );
                }
            }
        }
    }

}; // end namespace
