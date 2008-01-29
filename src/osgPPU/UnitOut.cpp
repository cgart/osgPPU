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

#include <osgPPU/UnitOut.h>
#include <osgPPU/Processor.h>

namespace osgPPU
{
    //------------------------------------------------------------------------------
    UnitOut::UnitOut(const UnitOut& unit, const osg::CopyOp& copyop) :
        Unit(unit, copyop)
    {
    
    }
    
    //------------------------------------------------------------------------------
    UnitOut::~UnitOut()
    {
    
    }
    
    //------------------------------------------------------------------------------
    void UnitOut::init()
    {
        // assign the input texture and shader if they are valid
        //assignInputTexture();
        //assignShader();
        
        initializeBase();
    }
    
    
    //------------------------------------------------------------------------------
    void UnitOut::render(int mipmapLevel)
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
    }
}; // end namespace
