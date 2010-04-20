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

#include <osgPPU/UnitMipmapInMipmapOut.h>
#include <osgPPU/Processor.h>

#include <osg/Texture2D>

namespace osgPPU
{
    //------------------------------------------------------------------------------
    UnitMipmapInMipmapOut::UnitMipmapInMipmapOut(const UnitMipmapInMipmapOut& unit, const osg::CopyOp& copyop) :
        UnitInOut(unit, copyop),
        mIOMipmapViewport(unit.mIOMipmapViewport),
        mIOMipmapFBO(unit.mIOMipmapFBO),
        mIOMipmapDrawable(unit.mIOMipmapDrawable)
    {
    }
    
    //------------------------------------------------------------------------------
    UnitMipmapInMipmapOut::UnitMipmapInMipmapOut() : UnitInOut()
    {
        mProjectionMatrix = new osg::RefMatrix(osg::Matrix::ortho(0,1,0,1,0,1));
        mModelviewMatrix = new osg::RefMatrix(osg::Matrixf::identity());
    }
    
    //------------------------------------------------------------------------------
    UnitMipmapInMipmapOut::~UnitMipmapInMipmapOut()
    {
    
    }
    
    //------------------------------------------------------------------------------
    void UnitMipmapInMipmapOut::init()
    {
        // initialize all parts of the ppu
        UnitInOut::init();

        // recheck the mipmap bypass data structures
        checkIOMipmappedData();    
    
        // attach a drawable for each mipmap level
        for (unsigned int i=0; i < mIOMipmapViewport.size(); i++)
        {
            osg::Drawable* draw = mIOMipmapDrawable[i];
            osg::StateSet* ss = draw->getOrCreateStateSet();
            ss->setAttribute(mIOMipmapViewport[i].get(), osg::StateAttribute::ON);

            // setup drawable uniforms
            osg::Uniform* w = ss->getOrCreateUniform(OSGPPU_VIEWPORT_WIDTH_UNIFORM, osg::Uniform::FLOAT);
            osg::Uniform* h = ss->getOrCreateUniform(OSGPPU_VIEWPORT_HEIGHT_UNIFORM, osg::Uniform::FLOAT);
            w->set((float)mIOMipmapViewport[i]->width());
            h->set((float)mIOMipmapViewport[i]->height());

            osg::Uniform* ln = ss->getOrCreateUniform(OSGPPU_MIPMAP_LEVEL_NUM_UNIFORM, osg::Uniform::FLOAT);
            ln->set((float)mIOMipmapViewport.size());

            osg::Uniform* le = ss->getOrCreateUniform(OSGPPU_MIPMAP_LEVEL_UNIFORM, osg::Uniform::FLOAT);
            le->set((float)i);
        }
    }
    
    
    //------------------------------------------------------------------------------
    void UnitMipmapInMipmapOut::checkIOMipmappedData()
    {
        if (mOutputTex.size() > 0)
        {
            // do only proceed if output texture is valid
            if (mOutputTex.begin()->second == NULL) return;
    
            // clean viewport data
            mIOMipmapViewport.clear();
            mIOMipmapFBO.clear();
            mIOMipmapDrawable.clear();

            // get dimensions of the output data
            int width = (mOutputTex.begin()->second)->getTextureWidth();
            int height = (mOutputTex.begin()->second)->getTextureHeight();
            int mwh = std::max(width, height);
            int numLevels = 1 + static_cast<int>(floor(logf(mwh)/logf(2.0f)));
    
            // generate fbo for each mipmap level 
            for (int level=0; level < numLevels; level++)
            {
                // generate viewport for this level
                osg::ref_ptr<osg::Viewport> vp = new osg::Viewport();
                int w = std::max(1, (int)floor(float(width) / float(pow(2.0f, (float)level)) ));
                int h = std::max(1, (int)floor(float(height) / float(pow(2.0f, (float)level)) ));
                vp->setViewport(0,0, (osg::Viewport::value_type)w, (osg::Viewport::value_type)h);
                mIOMipmapViewport.push_back(vp);
    
                // this is the fbo for this level 
				osg::ref_ptr<FrameBufferObject> fbo = new FrameBufferObject();
    
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
                        osg::notify(osg::FATAL) << "osgPPU::UnitInOut::checkIOMipmappedData() - " << getName() << ": output textures has different dimensions" << std::endl;
                        return; 
                    }
        
                    // set fbo of current level with to this output         
                    fbo->setAttachment(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0 + mrt), osg::FrameBufferAttachment(output.get(), level));
                }
    
                // store fbo
                mIOMipmapFBO.push_back(fbo);

                // generate mipmap drawables
                osg::Drawable* draw = createTexturedQuadDrawable();
                osg::StateSet* ss = draw->getOrCreateStateSet();
                ss->setAttribute(vp, osg::StateAttribute::ON);
                //ss->setAttribute(fbo, osg::StateAttribute::ON);
                mIOMipmapDrawable.push_back(draw);
            }
        }
    }

    //--------------------------------------------------------------------------
    bool UnitMipmapInMipmapOut::noticeBeginRendering (osg::RenderInfo& info, const osg::Drawable* )
    {
        // setup matricies, they must be setted up correctly in order
        // to have correct rendering of mipmaps
        info.getState()->applyProjectionMatrix(mProjectionMatrix.get());
        info.getState()->applyModelViewMatrix(mModelviewMatrix.get());

        pushFrameBufferObject(*info.getState());

        // perform manual rendering of all drawables of this unit
        // the drawables are used for every mipmap level 
        for (unsigned i=0; i < mIOMipmapDrawable.size(); i++)
        {
            info.getState()->apply(mIOMipmapDrawable[i]->getStateSet());
            mIOMipmapFBO[i]->apply(*info.getState());
            mIOMipmapDrawable[i]->drawImplementation(info);
        }

        popFrameBufferObject(*info.getState());
        
        // return false, so that parent drawable will not be rendered
        // this unit does the handling of drawables manually
        return false;
    }
        
}; // end namespace
