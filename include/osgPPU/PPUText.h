/***************************************************************************
 *   Copyright (c) 2008   Art Tevs                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 ***************************************************************************/

#ifndef _C_POST_PROCESS_UNITS_TEXT_HUD_H_
#define _C_POST_PROCESS_UNITS_TEXT_HUD_H_


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgPPU/PPUInOut.h>
#include <osgText/Text>

namespace osgPPU
{
    //! Smae as PostProcessUnitInOut but do render a text onto the output
    /**
    * The text is displayed in 2D ortho mode. This class is also derived from the osgText::Text
    * class. Hence check it for more information about the text support.
    **/
    class PostProcessUnitText : public PostProcessUnitInOut, public osgText::Text
    {
        public:
        
            //! Create default ppfx 
            PostProcessUnitText(osgPPU::PostProcess* parent);
            
            //! Release it and used memory
            ~PostProcessUnitText();
            
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
                PostProcessUnitInOut::setName(name);
            }

        protected:

            //! Apply the defule unit 
            virtual void render(int mipmapLevel = 0);
            
            //! Size of the font
            float mSize;
    };

}; // end namespace

#endif
