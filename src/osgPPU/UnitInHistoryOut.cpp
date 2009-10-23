/***************************************************************************
 *   Copyright (c) 2009   Art Tevs                                         *
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

#include <osgPPU/UnitInHistoryOut.h>
#include <osgPPU/Processor.h>

namespace osgPPU
{
    //------------------------------------------------------------------------------
    UnitInHistoryOut::UnitInHistoryOut(const UnitInHistoryOut& unit, const osg::CopyOp& copyop) :
        UnitInOut(unit, copyop),
        _historySize(1)
    {
        setOutputTextureType(UnitInOut::TEXTURE_2D_ARRAY);        
    }

    //------------------------------------------------------------------------------
    UnitInHistoryOut::UnitInHistoryOut() : UnitInOut()
    {
        osg::notify(osg::FATAL) << "UnitInHistoryOut is currently not implemented!!!" << std::endl;
    }
    
    //------------------------------------------------------------------------------
    UnitInHistoryOut::~UnitInHistoryOut()
    {
    
    }
    
    //------------------------------------------------------------------------------
    void UnitInHistoryOut::init()
    {
        UnitInOut::init();

    }

    //------------------------------------------------------------------------------
    void UnitInHistoryOut::update()
    {
        UnitInOut::update();

    }

    //------------------------------------------------------------------------------
    osg::Texture* UnitInHistoryOut::getOrCreateOutputTexture(int mrt)
    {
        osg::notify(osg::FATAL) << "UnitInHistoryOut is currently not implemented!!!" << std::endl;
        return NULL;
    }
    
    //------------------------------------------------------------------------------
    void UnitInHistoryOut::assignOutputTexture()
    {

    }

    
}; // end namespace
