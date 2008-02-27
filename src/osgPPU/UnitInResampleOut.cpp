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

#include <osgPPU/UnitInResampleOut.h>
#include <osgPPU/Processor.h>

namespace osgPPU
{
    //------------------------------------------------------------------------------
    UnitInResampleOut::UnitInResampleOut(const UnitInResampleOut& unit, const osg::CopyOp& copyop) :
        UnitInOut(unit, copyop),
        mWidthFactor(unit.mWidthFactor),
        mHeightFactor(unit.mHeightFactor),
        mDirtyFactor(unit.mDirtyFactor)
    {
    }
    //------------------------------------------------------------------------------
    UnitInResampleOut::UnitInResampleOut(osg::State* state) : UnitInOut(state)
    {
        // setup default values 
        mWidthFactor = 1.0;
        mHeightFactor = 1.0;
        mDirtyFactor = true;
    }
    //------------------------------------------------------------------------------
    UnitInResampleOut::UnitInResampleOut() : UnitInOut()
    {
        // setup default values 
        mWidthFactor = 1.0;
        mHeightFactor = 1.0;
        mDirtyFactor = true;
    }
    
    //------------------------------------------------------------------------------
    UnitInResampleOut::~UnitInResampleOut()
    {
    
    }
    
    //------------------------------------------------------------------------------
    void UnitInResampleOut::setFactorX(float w)
    {
        mWidthFactor = w;
        mDirtyFactor = true;
    }
    
    //------------------------------------------------------------------------------
    void UnitInResampleOut::setFactorY(float h)
    {
        mHeightFactor = h;
        mDirtyFactor = true;
    }
    
    //------------------------------------------------------------------------------
    void UnitInResampleOut::render(int mipmapLevel)
    {
        // if we have to reset the resampling factor
        if (mDirtyFactor)
        {
            // force to reset the input referrence size 
            setInputTextureIndexForViewportReference(getInputTextureIndexForViewportReference());
    
            // setup new viewport size
            mViewport->width() *= mWidthFactor;
            mViewport->height() *= mHeightFactor;
            mDirtyFactor = false;
    
            // notice that we changed the viewport
            noticeChangeViewport();
        }
    
        // do rendering as usual
        UnitInOut::render();
    }

}; // end namespace
