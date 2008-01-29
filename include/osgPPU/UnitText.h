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

#ifndef _C_UNITS_TEXT_HUD_H_
#define _C_UNITS_TEXT_HUD_H_


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgText/Text>

#include <osgPPU/Export.h>
#include <osgPPU/UnitInOut.h>

namespace osgPPU
{
    //! Smae as UnitInOut but do render a text onto the output
    /**
    * The text is displayed in 2D ortho mode. This class is also derived from the osgText::Text
    * class. Hence check it for more information about the text support.
    **/
    class OSGPPU_EXPORT UnitText : public UnitInOut, public osgText::Text
    {
        public:
            virtual const char* className() const { return "UnitText" ;} 
            virtual osg::Object* cloneType() const { return dynamic_cast<UnitInOut*>(new UnitText());} 
            virtual osg::Object* clone(const osg::CopyOp& copyop) const { UnitText* u = new UnitText(*this, copyop); return dynamic_cast<UnitInOut*>(u);} 

            //! Create default ppfx 
            UnitText(osg::State* state);
            UnitText();
            UnitText(const UnitText&, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
            
            //! Release it and used memory
            ~UnitText();
            
            //! Initialze the default postprocessing unit 
            virtual void init();
        
            //! Set size of the characters (relative to viewport.width / 640)
            inline void setSize(float size) { mSize= size;}

            //! Get current size 
            inline float getSize() const { return mSize; }
        
            //! Set 2D Screen positon
            inline void setPosition(float x, float y) { osgText::Text::setPosition(osg::Vec3(x,y,0)); }

            //! Redefine the setName method, because of diamond polymorphysm against osg::Object
            inline void setName(const std::string& name)
            {
                UnitInOut::setName(name);
            }

        protected:

            //! Apply the defule unit 
            virtual void render(int mipmapLevel = 0);
            
            //! Size of the font
            float mSize;
    };

}; // end namespace

#endif
