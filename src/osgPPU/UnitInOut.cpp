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

// base includes handling gcc++ compiler issues
// with memset dependency up from version 4.3.x
#include <stdio.h>
#include <string.h>

#include <osgPPU/UnitInOut.h>
#include <osgPPU/Processor.h>
#include <osgPPU/Utility.h>

#include <osg/TextureCubeMap>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/GL2Extensions>

namespace osgPPU
{
    //------------------------------------------------------------------------------
    // Helper class for filling the generated texture with default pixel values
    //------------------------------------------------------------------------------
    class Subload3DCallback : public osg::Texture3D::SubloadCallback
    {
        public:
            // fill texture with default pixel values
            void load (const osg::Texture3D &texture, osg::State &state) const
            {
                // do only anything if such textures are supported
                osg::Texture3D::Extensions* ext = osg::Texture3D::getExtensions(state.getContextID(), true);
                if (ext && ext->isTexture3DSupported())
                {
                    // create temporary image which is initialized with 0 values
                    osg::ref_ptr<osg::Image> img = new osg::Image();
                    img->allocateImage(
                        texture.getTextureWidth() ? texture.getTextureWidth() : 1,
                        texture.getTextureHeight() ? texture.getTextureHeight() : 1,
                        texture.getTextureDepth() ? texture.getTextureDepth() : 1,
                        texture.getSourceFormat() ? texture.getSourceFormat() : texture.getInternalFormat(),
                        texture.getSourceType() ? texture.getSourceType() : GL_UNSIGNED_BYTE);

                    // fill the image with 0 values
                    memset(img->data(), 0, img->getTotalSizeInBytesIncludingMipmaps() * sizeof(unsigned char));

                    // create the texture in usual OpenGL way
                    ext->glTexImage3D( GL_TEXTURE_3D, 0, texture.getInternalFormat(),
                        texture.getTextureWidth(), texture.getTextureHeight(), texture.getTextureDepth(),
                        texture.getBorderWidth(), texture.getSourceFormat() ? texture.getSourceFormat() : texture.getInternalFormat(),
                        texture.getSourceType() ? texture.getSourceType() : GL_UNSIGNED_BYTE,
                        img->data());
                }
            }

            // no subload, because while we want to subload the texture should be already valid
            void subload (const osg::Texture3D &texture, osg::State &state) const
            {

            }
    };

    //------------------------------------------------------------------------------
    // Helper class for filling the generated texture with default pixel values
    //------------------------------------------------------------------------------
    class Subload2DCallback : public osg::Texture2D::SubloadCallback
    {
        public:
            // fill texture with default pixel values
            void load (const osg::Texture2D &texture, osg::State &state) const
            {
                // create temporary image which is initialized with 0 values
                osg::ref_ptr<osg::Image> img = new osg::Image();
                img->allocateImage(
                    texture.getTextureWidth() ? texture.getTextureWidth() : 1,
                    texture.getTextureHeight() ? texture.getTextureHeight() : 1,
                    1,
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

            // no subload, because while we want to subload the texture should be already valid
            void subload (const osg::Texture2D &texture, osg::State &state) const
            {

            }
    };

    //------------------------------------------------------------------------------
    // Helper class for filling the generated texture with default pixel values
    //------------------------------------------------------------------------------
    class SubloadCubeMapCallback : public osg::TextureCubeMap::SubloadCallback
    {

        public:
            // fill texture with default pixel values
            void load (const osg::TextureCubeMap &texture, osg::State &state) const
            {
                GLenum faceTarget[6] =
                {
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
                    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
                    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
                    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
                };

                // create temporary image which is initialized with 0 values
                osg::ref_ptr<osg::Image> img = new osg::Image();
                img->allocateImage(
                    texture.getTextureWidth() ? texture.getTextureWidth() : 1,
                    texture.getTextureHeight() ? texture.getTextureHeight() : 1,
                    1,
                    texture.getSourceFormat() ? texture.getSourceFormat() : texture.getInternalFormat(),
                    texture.getSourceType() ? texture.getSourceType() : GL_UNSIGNED_BYTE);

                // fill the image with 0 values
                memset(img->data(), 0, img->getTotalSizeInBytesIncludingMipmaps() * sizeof(unsigned char));

                // create the texture in usual OpenGL way
                for (int n=0; n<6; n++)
                {
                    glTexImage2D( faceTarget[n], 0, texture.getInternalFormat(),
                        texture.getTextureWidth(), texture.getTextureHeight(), texture.getBorderWidth(),
                        texture.getSourceFormat() ? texture.getSourceFormat() : texture.getInternalFormat(),
                        texture.getSourceType() ? texture.getSourceType() : GL_UNSIGNED_BYTE,
                        img->data());
                }
            }

            // no subload, because while we want to subload the texture should be already valid
            void subload (const osg::TextureCubeMap &texture, osg::State &state) const
            {

            }
    };

    //------------------------------------------------------------------------------
    UnitInOut::UnitInOut(const UnitInOut& unit, const osg::CopyOp& copyop) :
        Unit(unit, copyop),
        mFBO(unit.mFBO),
        mBypassedInput(unit.mBypassedInput),
        mOutputCubemapFace(unit.mOutputCubemapFace),
        mOutputZSlice(unit.mOutputZSlice),
        mOutputDepth(unit.mOutputDepth),
        mOutputType(unit.mOutputType),
        mOutputInternalFormat(unit.mOutputInternalFormat)
    {
    }

    //------------------------------------------------------------------------------
    UnitInOut::UnitInOut() : Unit(),
        mBypassedInput(-1),
        mOutputCubemapFace(0),
        mOutputDepth(1),
        mOutputType(TEXTURE_2D),
        mOutputInternalFormat(GL_RGBA16F_ARB)
    {
        mFBO = new osg::FrameBufferObject();

        // add empty mrt=0 output texture
        setOutputTexture(NULL, 0);
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

        // setup unfiroms
        if (mOutputType == TEXTURE_CUBEMAP)
        {
            osg::Uniform* faceUniform = mDrawable->getOrCreateStateSet()->getOrCreateUniform(OSGPPU_CUBEMAP_FACE_UNIFORM, osg::Uniform::INT);
            faceUniform->set((int)mOutputCubemapFace);
        }else if (mOutputType == TEXTURE_3D)
        {
            osg::Uniform* sliceCount = mDrawable->getOrCreateStateSet()->getOrCreateUniform(OSGPPU_3D_SLICE_NUMBER, osg::Uniform::INT);
            sliceCount->set((int)mOutputDepth);

            for (OutputSliceMap::const_iterator it = getOutputZSliceMap().begin(); it != getOutputZSliceMap().end(); it++)
            {
                osg::Uniform* sliceIndex = mDrawable->getOrCreateStateSet()->getOrCreateUniform(OSGPPU_3D_SLICE_INDEX, osg::Uniform::INT, getOutputZSliceMap().size());
                sliceIndex->setElement(it->first, (int)it->second);

                if (getOutputZSliceMap().size() <= it->first)
                {
                    osg::notify(osg::WARN) << "osgPPU::UnitInOut::init() - specified mrt index " << it->first << " is not valid. Results might be wrong!" << std::endl;
                }
            }
        }

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
    void UnitInOut::setOutputTexture(osg::Texture* outTex, int mrt)
    {
        if (outTex)
            mOutputTex[mrt] = outTex;
        else
            mOutputTex[mrt] = osg::ref_ptr<osg::Texture>(NULL);

        dirty();
    }

    //--------------------------------------------------------------------------
    void UnitInOut::setOutputInternalFormat(GLenum format)
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
    void UnitInOut::setOutputDepth(unsigned int depth)
    {
        mOutputDepth = depth;
        dirty();

        // TODO: maybe some differentiation between MRT outputs is here required
        // However no idea if different slices of different 3D textures could be
        // used as MRT output. Have to check that, art 08. July, 2008
    }

    //------------------------------------------------------------------------------
    osg::Texture* UnitInOut::getOrCreateOutputTexture(int mrt)
    {
        // if already exists, then return back
        osg::Texture* tex = mOutputTex[mrt].get();
        if (tex) return tex;

        // if not exists, then do allocate it
        osg::Texture* mTex = NULL;
        if (mOutputType == TEXTURE_2D)
        {
            mTex = new osg::Texture2D();
            dynamic_cast<osg::Texture2D*>(mTex)->setSubloadCallback(new Subload2DCallback());
            if (mViewport.valid())
                dynamic_cast<osg::Texture2D*>(mTex)->setTextureSize(int(mViewport->width()), int(mViewport->height()) );
        }else if (mOutputType == TEXTURE_CUBEMAP)
        {
            mTex = new osg::TextureCubeMap();
            dynamic_cast<osg::TextureCubeMap*>(mTex)->setSubloadCallback(new SubloadCubeMapCallback());
            if (mViewport.valid())
                dynamic_cast<osg::TextureCubeMap*>(mTex)->setTextureSize(int(mViewport->width()), int(mViewport->height()) );
        }else if (mOutputType == TEXTURE_3D)
        {
            mTex = new osg::Texture3D();
            dynamic_cast<osg::Texture3D*>(mTex)->setSubloadCallback(new Subload3DCallback());
            if (mViewport.valid())
                dynamic_cast<osg::Texture3D*>(mTex)->setTextureSize(int(mViewport->width()), int(mViewport->height()), mOutputDepth);
        }else
        {
            osg::notify(osg::FATAL) << "osgPPU::UnitInOut::getOrCreateOutputTexture() - " << getName() << " non-supported texture type specified!" << std::endl;
            return NULL;
        }

        // setup texture parameters
        mTex->setResizeNonPowerOfTwoHint(false);
        mTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        mTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        mTex->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
        mTex->setBorderColor(osg::Vec4(0,0,0,0));
        mTex->setInternalFormat(getOutputInternalFormat());
        mTex->setSourceFormat(createSourceTextureFormat(getOutputInternalFormat()));

        // check if the input texture was in nearest mode
        if (getInputTexture(0) && getInputTexture(0)->getFilter(osg::Texture2D::MIN_FILTER) == osg::Texture2D::NEAREST)
            mTex->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::NEAREST);
        else
            mTex->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);

        if (getInputTexture(0) && getInputTexture(0)->getFilter(osg::Texture2D::MAG_FILTER) == osg::Texture2D::NEAREST)
            mTex->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::NEAREST);
        else
            mTex->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);

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
            // get output texture
            osg::Texture* texture = it->second.get();

            // if the output texture is NULL, hence generate one
            if (texture == NULL)
            {
                // preallocate the texture
                texture = getOrCreateOutputTexture(it->first);

                // check that the viewport must be valid at this time
                // we check it here, because we only want to set the size, if texture is fresh
                if (!mViewport.valid())
                {
                    osg::notify(osg::FATAL) << "osgPPU::UnitInOut::assignOutputTexture() - " << getName() << " cannot set output texture size, because viewport is invalid" << std::endl;
                    continue;
                }
            }

            // check whenever the output texture is a 2D texture
            osg::Texture2D* tex2D = dynamic_cast<osg::Texture2D*>(texture);
            if (tex2D != NULL)
            {
                mFBO->setAttachment(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0 + it->first), osg::FrameBufferAttachment(tex2D));
                continue;
            }

            // check if the output texture is a cubemap texture
            osg::TextureCubeMap* cubemapTex = dynamic_cast<osg::TextureCubeMap*>(texture);
            if (cubemapTex != NULL)
            {
                mFBO->setAttachment(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0 + it->first), osg::FrameBufferAttachment(cubemapTex, mOutputCubemapFace));
                continue;
            }

            // check whenever the output texture is a 3D texture
            osg::Texture3D* tex3D = dynamic_cast<osg::Texture3D*>(texture);
            if (tex3D != NULL)
            {
                // for each mrt to slice mapping do
                for (OutputSliceMap::const_iterator jt = getOutputZSliceMap().begin(); jt != getOutputZSliceMap().end(); jt++)
                {
                    mFBO->setAttachment(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0 + jt->first), osg::FrameBufferAttachment(tex3D, jt->second));
                }
                continue;

            }

            // if we are here, then output texture type is not supported, hence give some warning
            osg::notify(osg::FATAL) << "osgPPU::UnitInOut::assignOutputTexture() - " << getName() << " cannot attach output texture to FBO because output texture type is not supported" << std::endl;
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
                // if texture type is a 2d texture
                if (dynamic_cast<osg::Texture2D*>(it->second.get()) != NULL)
                {
                    // change size
                    osg::Texture2D* mTex = dynamic_cast<osg::Texture2D*>(it->second.get());
                    mTex->setTextureSize(int(mViewport->width()), int(mViewport->height()) );
                }
                // if texture type is a cubemap texture
                else if (dynamic_cast<osg::TextureCubeMap*>(it->second.get()) != NULL)
                {
                    // change size
                    osg::TextureCubeMap* mTex = dynamic_cast<osg::TextureCubeMap*>(it->second.get());
                    mTex->setTextureSize(int(mViewport->width()), int(mViewport->height()) );
                }
            }
        }
    }

}; // end namespace
