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
    // Class in order to correctly handle FBOs of the mipmap
    //------------------------------------------------------------------------------
    class MipmapDrawCallback : public osg::Drawable::DrawCallback
    {
    private:
        UnitMipmapInMipmapOut* _parent;
        osg::FrameBufferObject* _fbo;

        mutable osg::buffered_value<GLuint> _fboID;

    public:
        MipmapDrawCallback(UnitMipmapInMipmapOut* parent, osg::FrameBufferObject* fbo) : _parent(parent), _fbo(fbo) {}

        void drawImplementation (osg::RenderInfo& info, const osg::Drawable* drawable) const
        {
            _parent->pushFrameBufferObject(*info.getState());

            drawable->drawImplementation(info);

            _parent->popFrameBufferObject(*info.getState());
        }
    };


    //------------------------------------------------------------------------------
    UnitMipmapInMipmapOut::UnitMipmapInMipmapOut(const UnitMipmapInMipmapOut& unit, const osg::CopyOp& copyop) :
        UnitInOut(unit, copyop),
        mIOMipmapViewport(unit.mIOMipmapViewport),
        mIOMipmapFBO(unit.mIOMipmapFBO)
    {
    }
    
    //------------------------------------------------------------------------------
    UnitMipmapInMipmapOut::UnitMipmapInMipmapOut() : UnitInOut()
    {
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
    
        // remove all drawables we currently have 
        mGeode->removeDrawables(0, mGeode->getNumDrawables());

        // attach a drawable for each mipmap level
        for (unsigned int i=0; i < mIOMipmapViewport.size(); i++)
        {
            osg::Drawable* draw = createTexturedQuadDrawable();
            osg::StateSet* ss = draw->getOrCreateStateSet();
            ss->setAttribute(mIOMipmapViewport[i].get(), osg::StateAttribute::ON);
            //ss->setAttribute(mIOMipmapFBO[i].get(), osg::StateAttribute::ON);
            draw->setDrawCallback(new MipmapDrawCallback(this, mIOMipmapFBO[i].get()));
            mGeode->addDrawable(draw);

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
                        osg::notify(osg::FATAL) << "osgPPU::UnitInOut::checkIOMipmappedData() - " << getName() << ": output textures has different dimensions" << std::endl;
                        return; 
                    }
        
                    // set fbo of current level with to this output         
                    fbo->setAttachment(osg::Camera::BufferComponent(osg::Camera::COLOR_BUFFER0 + mrt), osg::FrameBufferAttachment(output.get(), level));
                }
    
                // store fbo
                mIOMipmapFBO.push_back(fbo);
            }
        }
    }
        
}; // end namespace
