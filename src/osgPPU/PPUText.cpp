/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include <osgPPU/PPUText.h>

namespace osgPPU
{
    //------------------------------------------------------------------------------
    PostProcessUnitText::PostProcessUnitText(osgPPU::PostProcess* parent) : osgPPU::PostProcessUnitInOut(parent)
    {
        // Create text for statistics
        mText = new Text();
        mX = 0;
        mY = 0;
        mWidth = 1;
        mHeight = 1;
        mSize = 26.0;
    }
    
    //------------------------------------------------------------------------------
    PostProcessUnitText::~PostProcessUnitText()
    {

    }
    
    //------------------------------------------------------------------------------
    void PostProcessUnitText::init()
    {
        // initialize text
        mText->setColor(1,1,1,1);
        mText->getTextPtr()->setStateSet(sScreenQuad->getOrCreateStateSet());
    
        // init inout ppu
        mOutputTex[0] = mInputTex[0];
        osgPPU::PostProcessUnitInOut::init();
    
        // setup projection matrix
        sProjectionMatrix = osg::Matrix::ortho2D(0,1,0,1);
    }
    
    
    //------------------------------------------------------------------------------
    void PostProcessUnitText::render(int mipmapLevel)
    {
        // return if we do not get valid state
        if (!sState.getState()) return;
        
        // can only be done on valid data 
        if (mFBO.valid() && mViewport.valid())
        {
            // we take the width 640 as reference width for the size of characters
            mText->setSize(mSize * (float(getViewport()->width()) / 640.0));
    
            // aplly stateset
            sState.getState()->apply(sScreenQuad->getStateSet());
    
            // apply framebuffer object, this will bind it, so we can use it
            mFBO->apply(*sState.getState());
    
            // apply viewport
            mViewport->apply(*sState.getState());
            
            // compute new color 
            osg::Vec4 color = mText->getColor();
            color.a() = getCurrentBlendValue();
    
            // get string and print it to the output
            mText->setColor(color);
            sState.getState()->apply(mText->getTextPtr()->getStateSet());
    
            glEnable(GL_BLEND);
            mText->getTextPtr()->draw(sState);
            glDisable(GL_BLEND);
        }
    }

}; // end namespace

