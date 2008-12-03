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

#include <osgPPU/UnitBypass.h>
#include <osgPPU/Processor.h>
#include <osgPPU/BarrierNode.h>

namespace osgPPU
{

    //------------------------------------------------------------------------------
    UnitBypass::UnitBypass() : Unit()
    {
    }

    //------------------------------------------------------------------------------
    UnitBypass::UnitBypass(const UnitBypass& u, const osg::CopyOp& copyop) : 
        Unit(u, copyop) 
    {

    }
    
    //------------------------------------------------------------------------------
    UnitBypass::~UnitBypass()
    {

    }

    //------------------------------------------------------------------------------
    void UnitBypass::setupInputsFromParents()
    {
        // same as normal unit
        Unit::setupInputsFromParents();

        // now do set the output texture equal to the input
        mOutputTex = mInputTex;
    }

}; // end namespace
