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

#ifndef _C_UNIT_TEXTURE_H_
#define _C_UNIT_TEXTURE_H_


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgPPU/Export.h>
#include <osgPPU/Unit.h>

namespace osgPPU
{
    //! Texture unit is used to setup external textures in the unit graph
    /**
    * If you like to have an external texture as input to any unit in the unit graph,
    * then you have to setup this behaviour with the help of this unit. Place
    * this unit as a parent of any other unit and its output, the texture,
    * will became input to that unit.
    **/
    class OSGPPU_EXPORT UnitTexture : public Unit {
        public:
            META_Node(osgPPU,UnitTexture);
        
            UnitTexture();
            UnitTexture(const UnitTexture& u, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
            UnitTexture(osg::Texture* tex);
            
            ~UnitTexture();
            
            void init();

            /**
            * Set a texture which is used as output of this unit.
            * The children will get this texture as input atomatically.
            **/
            void setTexture(osg::Texture* tex);

            /**
            * Get texture which is used as output of this unit.
            **/
            inline osg::Texture* getTexture() { return getOutputTexture(0); }

        private:
            class DrawCallback;
    };
};

#endif
