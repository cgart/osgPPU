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

namespace osgPPU
{
    //------------------------------------------------------------------------------
    UnitInOut::UnitInOut(const UnitInOut& unit, const osg::CopyOp& copyop) :
        Unit(unit, copyop),
        mFBO(unit.mFBO),
        mBypassedInput(unit.mBypassedInput)
    {
    }
    
    //------------------------------------------------------------------------------
    UnitInOut::UnitInOut() : Unit()
    {
        // create FBO because we need it
        mFBO = new osg::FrameBufferObject();
        mBypassedInput = -1;
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
        if (mBypassedInput >= 0 && mBypassedInput < getNumParents())
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
        mTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP);
        mTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP);
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
            // if the output texture is a 2D texture 
            if (dynamic_cast<osg::Texture2D*>(it->second.get()) != NULL)
            {
                // attach the 2D texture to the fbo
                mFBO->setAttachment(GL_COLOR_ATTACHMENT0_EXT + i, 
                    osg::FrameBufferAttachment(dynamic_cast<osg::Texture2D*>(it->second.get())));

            // if the output texture is NULL, hence generate one
            }else if (it->second.get() == NULL && mViewport.valid())
            {
                // preallocate the texture
                osg::Texture2D* mTex = dynamic_cast<osg::Texture2D*>(getOrCreateOutputTexture(it->first));
                mTex->setTextureSize(int(mViewport->width()), int(mViewport->height()) );
        
                // attach the texture to the fbo
                mFBO->setAttachment(GL_COLOR_ATTACHMENT0_EXT + i, osg::FrameBufferAttachment(mTex));
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
