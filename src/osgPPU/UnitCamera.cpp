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

#include <osgPPU/UnitCamera.h>
#include <osgPPU/Processor.h>
#include <osgPPU/BarrierNode.h>

namespace osgPPU
{

    //------------------------------------------------------------------------------
    UnitCamera::UnitCamera() : UnitBypass()
    {
        mUseCameraAsChild = false;
    }

    //------------------------------------------------------------------------------
    UnitCamera::UnitCamera(const UnitCamera& u, const osg::CopyOp& copyop) : 
        UnitBypass(u, copyop),
        mCamera(u.mCamera),
        mUseCameraAsChild(u.mUseCameraAsChild)
    {
        if (mUseCameraAsChild) addChild(mCamera.get());
    }
    
    //------------------------------------------------------------------------------
    UnitCamera::~UnitCamera()
    {

    }

    //------------------------------------------------------------------------------
    void UnitCamera::setCamera(osg::Camera* camera, bool addAsChild)
    {
        if (addAsChild) addChild(camera);
        mCamera = camera;
        mUseCameraAsChild = addAsChild;
    }

}; // end namespace
