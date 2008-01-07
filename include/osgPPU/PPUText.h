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
#include <osgPPU/Text.h>

namespace osgPPU
{
    /**
    * Same as PostProcessFXOut but shows some text on the screen.
    * The text is displayed in 2D ortho mode.
    **/
    class PostProcessUnitText : public PostProcessUnitInOut
    {
        public:
        
            //! Create default ppfx 
            PostProcessUnitText(osgPPU::PostProcess* parent);
            
            //! Release it and used memory
            ~PostProcessUnitText();
            
            //! Initialze the default postprocessing unit 
            virtual void init();
        
            //! Set size of the characters (relative to viewport.width / 640)
            void setSize(float size) { mSize= size;}
            float getSize() const { return mSize; }
    
            //! get text 
            Text* getText(){ return mText.get(); }
    
        protected:

            //! Apply the defule unit 
            virtual void render(int mipmapLevel = 0);
    
            //! Text holding our statistics
            osg::ref_ptr<Text> mText;
            
            //! Size of the font
            float mSize;
    };

}; // end namespace

#endif
