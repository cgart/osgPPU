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


#include <osgPPU/PPUText.h>

namespace osgPPU
{
    //------------------------------------------------------------------------------
    PostProcessUnitText::PostProcessUnitText(osgPPU::PostProcess* parent) : osgPPU::PostProcessUnitInOut(parent)
    {
        // Create text for statistics
        //mText = new Text();
        mSize = 26.0;
    }
    
    //------------------------------------------------------------------------------
    PostProcessUnitText::~PostProcessUnitText()
    {

    }
    

    //------------------------------------------------------------------------------
    /*PostProcessUnitText::setFont()
    {
        osg::ref_ptr<osgText::Font> font = osgText::readFontFile(fontFile);
        if (font.valid()) setFont(mFont.get());
        return font.valid();
    }*/

    //------------------------------------------------------------------------------
    void PostProcessUnitText::init()
    {
        // initialize text
        setColor(osg::Vec4(1,1,1,1));
        setStateSet(sScreenQuad->getOrCreateStateSet());

        // setup some defaults parameters
        setLayout(osgText::Text::LEFT_TO_RIGHT);
        setCharacterSizeMode(osgText::Text::SCREEN_COORDS);
        
        // setup stateset
        osg::StateSet* stateSet = sScreenQuad->getOrCreateStateSet();
        stateSet->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
        stateSet->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
        stateSet->setMode(GL_BLEND,osg::StateAttribute::ON);
    
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
            //mText->setSize(mSize * (float(getViewport()->width()) / 640.0));
            setCharacterSize(mSize * (float(getViewport()->width()) / 640.0), 1.0);

            // compute new color, change alpha acording to the blend value
            _color.a() = getCurrentBlendValue();

            // aplly stateset
            sState.getState()->apply(sScreenQuad->getStateSet());
    
            // apply framebuffer object, this will bind it, so we can use it
            mFBO->apply(*sState.getState());
    
            // apply viewport
            mViewport->apply(*sState.getState());

            // draw the text                
            draw(sState);
        }
    }

}; // end namespace

