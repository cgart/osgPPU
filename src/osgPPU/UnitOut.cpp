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
    void UnitOut::init()
    {
        // init default
        Unit::init();

        // create a quad geometry
        mDrawable = createTexturedQuadDrawable();
        mGeode->removeDrawables(0, mGeode->getNumDrawables());
        mGeode->addDrawable(mDrawable.get());
    }

}; // end namespace
