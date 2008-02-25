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
    
        // render the content of the input texture into the frame buffer
        if (getUseBlendMode())
        {
            glEnable(GL_BLEND);
            glColor4f(1,1,1, getBlendValue());
        }

        sScreenQuad->draw(sState);
        if (getUseBlendMode())
        {
            glDisable(GL_BLEND);
            glColor4f(1,1,1,1);
        }   
    }
}; // end namespace
