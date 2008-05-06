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

#include <osgPPU/UnitInMipmapOut.h>
#include <osgPPU/Processor.h>

#include <osg/Texture2D>

namespace osgPPU
{
    //------------------------------------------------------------------------------
    UnitInMipmapOut::UnitInMipmapOut(const UnitInMipmapOut& unit, const osg::CopyOp& copyop) :
        UnitInOut(unit, copyop),
        mMipmapFBO(unit.mMipmapFBO),
        mMipmapViewport(unit.mMipmapViewport),
        mNumLevels(unit.mNumLevels),
        mGenerateMipmapInputIndex(unit.mGenerateMipmapInputIndex),
        mUseShader(unit.mUseShader),
        mOutputWidth(unit.mOutputWidth),
        mOutputHeight(unit.mOutputHeight)
    {
    }
    
    //------------------------------------------------------------------------------
    UnitInMipmapOut::UnitInMipmapOut() : UnitInOut()
    {
        mNumLevels = 0;
        mGenerateMipmapInputIndex = -1;
        mUseShader = true;
        mOutputWidth = 0;
        mOutputHeight = 0;
    }
    
    //------------------------------------------------------------------------------
    UnitInMipmapOut::~UnitInMipmapOut()
    {
    
    }
        
    //------------------------------------------------------------------------------
    void UnitInMipmapOut::init()
    {
        // default initialization
        UnitInOut::init();

        // if we have to create the output texture, then do it
        if (mGenerateMipmapInputIndex >= 0)
        {
            setOutputTexture(getInputTexture(mGenerateMipmapInputIndex));
            assignOutputTexture();
        }

        // enable mipmap levels in the output texture
        enableMipmapGeneration();

        // if generating of mipmaps done in software, then generate drawables
        if (mUseShader)
        {
            // disable standard fbo
            getOrCreateStateSet()->removeAttribute(mFBO.get());

            // this is the base level of mipmap generation
            // if the output texture is the same as input texture, then
            // we do not need to recompute the level 0, because it should 
            // stay the same.
            int baseLevel = (mGenerateMipmapInputIndex < 0 ? 0 : 1);

            // remove all drawables we currently have 
            // do not remove the drawable 0, since there is our base level stored
            mGeode->removeDrawables(0, mGeode->getNumDrawables());

            // attach a drawable for each mipmap level
            // however if we want to 
            for (int i = baseLevel; i < mNumLevels; i++)
            {
                // setup the drawable
                osg::Drawable* draw = createTexturedQuadDrawable();
                osg::StateSet* ss = draw->getOrCreateStateSet();
                ss->setAttribute(mMipmapViewport[i].get(), osg::StateAttribute::ON);
                ss->setAttribute(mMipmapFBO[i].get(), osg::StateAttribute::ON);
                mGeode->addDrawable(draw);

                // setup drawable uniforms
                osg::Uniform* w = ss->getOrCreateUniform(OSGPPU_VIEWPORT_WIDTH_UNIFORM, osg::Uniform::FLOAT);
                osg::Uniform* h = ss->getOrCreateUniform(OSGPPU_VIEWPORT_HEIGHT_UNIFORM, osg::Uniform::FLOAT);
                w->set((float)mMipmapViewport[i]->width());
                h->set((float)mMipmapViewport[i]->height());

                osg::Uniform* ln = ss->getOrCreateUniform(OSGPPU_MIPMAP_LEVEL_NUM_UNIFORM, osg::Uniform::FLOAT);
                ln->set((float)mNumLevels);

                osg::Uniform* le = ss->getOrCreateUniform(OSGPPU_MIPMAP_LEVEL_UNIFORM, osg::Uniform::FLOAT);
                le->set((float)i);
            }
        }
    }
    
    //--------------------------------------------------------------------------
    void UnitInMipmapOut::enableMipmapGeneration()
    {
        // enable mipmapping
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
                createAndAttachFBOs(it->second.get(), it->first);
            }
        }

    }
    
    //--------------------------------------------------------------------------
    void UnitInMipmapOut::createAndAttachFBOs(osg::Texture* output, int mrt)
    {
        // check if the texture is 2D texture
        if (dynamic_cast<osg::Texture2D*>(output) == NULL)
        {
            osg::notify(osg::WARN) << "osgPPU::UnitInMipmapOut::createAndAttachFBOs() - currently only 2D textures are supported" << std::endl;
            return;
        }

        // check if we have generated all the fbo's for each mipmap level
        int width = output->getTextureWidth();
        int height = output->getTextureHeight();
        int numLevel = 1 + (int)osg::round(log((float)std::max(width, height))/(float)M_LN2);

        // check if sizes are valid 
        if ((mOutputWidth != 0 && width != mOutputWidth)
          ||(mOutputHeight != 0 && height != mOutputHeight))
        {
            osg::notify(osg::FATAL) << "osgPPU::UnitInMipmapOut::createAndAttachFBOs() - output textures are not of the same size!" << std::endl;
            return;
        }

        // set new sizes
        mOutputWidth = width;
        mOutputHeight = height;
        mNumLevels = numLevel;

        // generate fbo and viewport for each mipmap level if not done before
        if ((int)mMipmapFBO.size() != numLevel)
        {
            // generate mipmap levels
            mMipmapFBO.clear();
            mMipmapViewport.clear();
            for (int i=0; i < numLevel; i++)
            {
                // generate viewport
                osg::ref_ptr<osg::Viewport> vp = new osg::Viewport();
                int w = std::max(1, (int)floor(float(width) / float(pow(2.0f, (float)i)) ));
                int h = std::max(1, (int)floor(float(height) / float(pow(2.0f, (float)i)) ));
                vp->setViewport(0,0, w, h);
                mMipmapViewport.push_back(vp);
                
                // generate fbo and assign a mipmap level to it
                osg::ref_ptr<osg::FrameBufferObject> fbo = new osg::FrameBufferObject();
                mMipmapFBO.push_back(fbo);
            }
        }

        // setup the attachments according to the outputs
        for (int i=0; i < numLevel; i++)
            mMipmapFBO[i]->setAttachment(GL_COLOR_ATTACHMENT0_EXT + mrt, osg::FrameBufferAttachment(dynamic_cast<osg::Texture2D*>(output), i));
    }


    //--------------------------------------------------------------------------
    void UnitInMipmapOut::noticeFinishRendering(osg::RenderInfo &renderInfo, const osg::Drawable* drawable)
    {
        // this method will be called everytime when a drawable of this unit
        // is rendered. Hence we check here if we are using hardware mipmap 
        // generation and apply this if neccessary

        // if we have used a shader to generate mipmaps, then we can safely return
        if (mUseShader) return;

        // get the fbo extensions
        osg::FBOExtensions* fbo_ext = osg::FBOExtensions::instance(renderInfo.getContextID(),true);

        // we don't use shader, that means that the mipmaps are generated in hardware, hence do this
        std::map<int, osg::ref_ptr<osg::Texture> >::iterator it = mOutputTex.begin();
        for (; it != mOutputTex.end(); it++)
        {
            renderInfo.getState()->applyTextureAttribute(0, it->second.get());
            fbo_ext->glGenerateMipmapEXT(it->second->getTextureTarget());
        }
    }
    
}; // end namespace
