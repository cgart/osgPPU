/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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
    
            //! Set x position
            void setX(float x) { mX = x;}
            float getX() const { return mX; }
    
            //! Set y position
            void setY(float y) { mY = y;}
            float getY() const { return mY; }
        
            //! Set width
            void setWidth(float width) { mWidth= width;}
            float getWidth() const { return mWidth; }
    
            //! Set height
            void setHeight(float height) { mHeight= height;}
            float getHeight() const { return mHeight; }
    
            //! Set size
            void setSize(float size) { mSize= size;}
            float getSize() const { return mSize; }
    
            //! get text 
            Text* getText(){ return mText.get(); }
    
        protected:

            //! Apply the defule unit 
            virtual void render(int mipmapLevel = 0);
    
            //! Text holding our statistics
            osg::ref_ptr<Text> mText;
            
            //! Sizes of the ppu 
            float mX, mY, mWidth, mHeight, mSize;
    };

}; // end namespace

#endif
