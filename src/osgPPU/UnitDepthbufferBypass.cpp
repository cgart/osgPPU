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

#include <osgPPU/UnitDepthbufferBypass.h>
#include <osgPPU/Processor.h>
#include <osgPPU/BarrierNode.h>

namespace osgPPU
{

    //------------------------------------------------------------------------------
    UnitDepthbufferBypass::UnitDepthbufferBypass() : UnitCameraAttachmentBypass()
    {
        setBufferComponent(osg::Camera::DEPTH_BUFFER);
    }

    //------------------------------------------------------------------------------
    UnitDepthbufferBypass::UnitDepthbufferBypass(const UnitDepthbufferBypass& u, const osg::CopyOp& copyop) :
        UnitCameraAttachmentBypass(u, copyop)
    {

    }
    
    //------------------------------------------------------------------------------
    UnitDepthbufferBypass::~UnitDepthbufferBypass()
    {

    }


}; // end namespace
