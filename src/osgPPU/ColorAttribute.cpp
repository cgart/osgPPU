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

#include <osgPPU/ColorAttribute.h>
#include <osg/State>

namespace osgPPU
{

//------------------------------------------------------------------------------
ColorAttribute::ColorAttribute() : 
    osg::StateAttribute(),
    mColor(osg::Vec4(1,1,1,1)),
    mTime(0),
    mStartTime(0),
    mEndTime(0),
    mStartColor(osg::Vec4(1,1,1,1)),
    mEndColor(osg::Vec4(1,1,1,1))
{
    setUpdateCallback(new UpdateCallback());
}

//------------------------------------------------------------------------------
ColorAttribute::ColorAttribute(const ColorAttribute& bm, const osg::CopyOp& copyop) :
    osg::StateAttribute(bm, copyop),
    mColor(bm.mColor),
    mTime(bm.mTime),
    mStartTime(bm.mStartTime),
    mEndTime(bm.mEndTime),
    mStartColor(bm.mStartColor),
    mEndColor(bm.mEndColor)
{

}

//------------------------------------------------------------------------------
ColorAttribute::~ColorAttribute()
{

}

//------------------------------------------------------------------------------
void ColorAttribute::apply(osg::State& state) const
{
    glColor4fv(mColor.ptr());
}


//------------------------------------------------------------------------------
void ColorAttribute::UpdateCallback::operator()(osg::StateAttribute* sa, osg::NodeVisitor* nv)
{
    // the given parameter must be of type ColorAttribute
    ColorAttribute* bm = dynamic_cast<ColorAttribute*>(sa);

    // if given right values and end time is bigger then 0
    if (bm && nv && bm->mEndTime > 0.00001)
    {
        bm->mTime = nv->getFrameStamp()->getReferenceTime();

        // if current time is passed the start time
        if (bm->mStartTime <= bm->mTime)
        {
            // compute interpolation factors
            float factor = (bm->mTime - bm->mStartTime) / (bm->mEndTime - bm->mStartTime);

            // compute linear interpolated color
            bm->mColor = bm->mStartColor * (1.0 - factor) + bm->mEndColor * factor;
        }

        // if current time has passed the end time, then just fix the values
        if (bm->mEndTime <= bm->mTime)
        {
            bm->mColor = bm->mEndColor;
        }

        // if the end time is specified as 0, then set to start value 
        if (bm->mEndTime < 0.00001)
        {
            bm->mColor = bm->mStartColor;
        }
        
    }
}

};
