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
    void UnitInResampleOut::dirty()
    {
        UnitInOut::dirty();
        mDirtyFactor = true;
    }
    
    //------------------------------------------------------------------------------
    void UnitInResampleOut::init()
    {
        // do initialize as usual
        UnitInOut::init();

        // if we have to reset the resampling factor
        if (mDirtyFactor)
        {
            mDirtyFactor = false;
            float width = (float)mViewport->width();
            float height = (float)mViewport->height();

            // force new viewport to be used
            osg::ref_ptr<osg::Viewport> oldVp = new osg::Viewport(*mViewport);
            osg::ref_ptr<osg::Viewport> newVp = new osg::Viewport(oldVp->x(), oldVp->y(), oldVp->width() * mWidthFactor, oldVp->height() * mHeightFactor);

            mViewport = newVp;
            assignViewport();
            mViewport = oldVp;

            noticeChangeViewport(newVp);
        }    
    }

}; // end namespace
