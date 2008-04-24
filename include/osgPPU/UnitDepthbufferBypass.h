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

#ifndef _C_UNIT_DEPTHBUFFER_BYPASS_H_
#define _C_UNIT_DEPTHBUFFER_BYPASS_H_


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgPPU/UnitBypass.h>

namespace osgPPU
{
    //! Bypass teh depthbuffer attachment of the camera to the output
    /**
    * This unit do not perform any rendering, however it do
    * bypass the depthbuffer attachment of the camera of the parent processor
    * to its output texture.
    *
    * This unit has to be placed directly under the processor, so that the unit
    * get access to the processor's attachments.
    **/
    class OSGPPU_EXPORT UnitDepthbufferBypass : public UnitBypass {
        public:
            META_Node(osgPPU,UnitDepthbufferBypass);
        
            UnitDepthbufferBypass();
            UnitDepthbufferBypass(const UnitDepthbufferBypass& u, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
            
            ~UnitDepthbufferBypass();
            
            void init();
        
        private:
            void setupInputsFromParents();
    };
};

#endif
