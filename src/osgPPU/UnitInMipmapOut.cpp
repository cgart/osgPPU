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
        mMipmapDrawable(unit.mMipmapDrawable),
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
        mProjectionMatrix = new osg::RefMatrix(osg::Matrix::ortho(0,1,0,1,0,1));
        mModelviewMatrix = new osg::RefMatrix(osg::Matrixf::identity());
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

        // if we have to create mipmaps for an input texture
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
            //getOrCreateStateSet()->removeAttribute(mFBO.get());
            getOrCreateStateSet()->removeAttribute(mViewport.get());

            // this is the base level of mipmap generation
            // if the output texture is the same as input texture, then
            // we do not need to recompute the level 0, because it should 
            // stay the same.
            int baseLevel = (mGenerateMipmapInputIndex < 0 ? 0 : 1);

            // attach a drawable for each mipmap level
            for (int i = baseLevel; i < mNumLevels; i++)
            {
                // setup the drawable
                osg::Drawable* draw = mMipmapDrawable[i];
                osg::StateSet* ss = draw->getOrCreateStateSet();

                // setup drawable uniforms
                osg::Uniform* w = ss->getOrCreateUniform(OSGPPU_VIEWPORT_WIDTH_UNIFORM, osg::Uniform::FLOAT);
                osg::Uniform* h = ss->getOrCreateUniform(OSGPPU_VIEWPORT_HEIGHT_UNIFORM, osg::Uniform::FLOAT);
                w->set((float)mMipmapViewport[i]->width());
                h->set((float)mMipmapViewport[i]->height());

                osg::Uniform* iw = ss->getOrCreateUniform(OSGPPU_VIEWPORT_INV_WIDTH_UNIFORM, osg::Uniform::FLOAT);
                osg::Uniform* ih = ss->getOrCreateUniform(OSGPPU_VIEWPORT_INV_HEIGHT_UNIFORM, osg::Uniform::FLOAT);
                iw->set(1.0f / (float)mMipmapViewport[i]->width());
                ih->set(1.0f / (float)mMipmapViewport[i]->height());

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
                else
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
        int mwh = std::max(width, height);
        int numLevel = 1 + static_cast<int>(floor(logf(mwh)/logf(2.0f)));

        // set new sizes
        mOutputWidth = width;
        mOutputHeight = height;
        mNumLevels = numLevel;

        // if we do not use shader, then return
        if (mUseShader == false) return;

        // generate fbo and viewport for each mipmap level if not done before
        if ((int)mMipmapFBO.size() != numLevel)
        {
            // generate mipmap levels
            mMipmapFBO.clear();
            mMipmapViewport.clear();
            mMipmapDrawable.clear();
            for (int i=0; i < numLevel; i++)
            {
                // generate viewport for the mipmap level
                osg::ref_ptr<osg::Viewport> vp = new osg::Viewport();
                int w = std::max(1, (int)floor(float(width) / float(pow(2.0f, (float)i)) ));
                int h = std::max(1, (int)floor(float(height) / float(pow(2.0f, (float)i)) ));
                vp->setViewport(0,0, (osg::Viewport::value_type)w, (osg::Viewport::value_type)h);
                mMipmapViewport.push_back(vp);
                
                // generate fbo and assign a mipmap level to it
				osg::ref_ptr<FrameBufferObject> fbo = new FrameBufferObject();
                fbo->setAttachment(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0 + mrt), osg::FrameBufferAttachment(dynamic_cast<osg::Texture2D*>(output), i));
                mMipmapFBO.push_back(fbo);

                // generate drawable which is responsible for this level
                osg::Drawable* draw = createTexturedQuadDrawable();
                osg::StateSet* ss = draw->getOrCreateStateSet();
                ss->setAttribute(vp, osg::StateAttribute::ON);
                //ss->setAttribute(fbo, osg::StateAttribute::ON);
                mMipmapDrawable.push_back(draw);
            }
        }
    }

    //--------------------------------------------------------------------------
    bool UnitInMipmapOut::noticeBeginRendering (osg::RenderInfo& info, const osg::Drawable* )
    {
        // if mipmaps generated not by shader, then do nothing
        if (mUseShader == false) return false;

        // setup matricies, they must be setted up correctly in order
        // to have correct rendering of mipmaps
        info.getState()->applyProjectionMatrix(mProjectionMatrix.get());
        info.getState()->applyModelViewMatrix(mModelviewMatrix.get());

        pushFrameBufferObject(*info.getState());

        // perform manual rendering of all drawables of this unit
        // the drawables are used for every mipmap level 
        for (unsigned i=(mGenerateMipmapInputIndex < 0 ? 0 : 1); i < mMipmapDrawable.size(); i++)
        {
            info.getState()->apply(mMipmapDrawable[i]->getStateSet());
            mMipmapFBO[i]->apply(*info.getState());
            mMipmapDrawable[i]->drawImplementation(info);
        }

        popFrameBufferObject(*info.getState());
        
        // return false, so that parent drawable will not be rendered
        // this unit does the handling of drawables manually
        return false;
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
            fbo_ext->glGenerateMipmap(it->second->getTextureTarget());
        }
    }
    
}; // end namespace
