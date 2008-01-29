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

#ifndef _C_UNIT_INOUT_H_
#define _C_UNIT_INOUT_H_


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgPPU/Export.h>
#include <osgPPU/Unit.h>

namespace osgPPU
{
    //! Compute output texture based on the assigned shaders and input data
    /**
    * InOut PPU, does render the content of input textures with applied shader 
    * to the output textures. Rendering is done in background, so no information
    * will leack to the frame buffer
    **/
    class OSGPPU_EXPORT UnitInOut : public Unit {
        public:
            //virtual const char* className() const { return "UnitInOut" ;} 
            META_Object(osgPPU,UnitInOut);
        
            //! Create default ppfx 
            UnitInOut(osg::State* parent);
            UnitInOut();
            UnitInOut(const UnitInOut&, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
            
            //! Release it and used memory
            virtual ~UnitInOut();
            
            //! Initialze the default Processoring unit
            virtual void init();
            
            /**
            * Set to true if in/out should also be done for mipmap levels.
            * This would mean that the shader will be called on 
            * each mipmap level of the input texture. The generated
            * result will be stored in the according level of the output 
            * texture. This method assumes the input and output textures 
            * do have the same number of mipmap levels.
            **/
            void setMipmappedIO(bool b);
        
            //! Are we using IO also for mipmap-levels
            bool getMipmappedIO() const { return mbMipmappedIO; }
    
        protected:
        
            //! Apply the defule unit 
            virtual void render(int mipmapLevel = -1);
            virtual void doRender(int mipmapLevel);
        
            //! Notice about end of rendering
            virtual void noticeFinishRendering() {}
            
            //! Viewport changed
            virtual void noticeChangeViewport();
    
            //! called when input textures are changed
            virtual void noticeChangeInput() {}
    
            //! Reassign fbo if output textures changes
            virtual void assignOutputTexture();
    
            virtual void noticeAssignShader() {}
            virtual void noticeRemoveShader() {}
    
            //! regenerate io mapmapped data structures
            void checkIOMipmappedData();
    
            //! Should we do in/out also on mipmap levels
            bool mbMipmappedIO;
            
            //! Viewports for each mipmap level 
            std::vector<osg::ref_ptr<osg::Viewport> > mIOMipmapViewport;
            
            //! Fbos for each mipmap level 
            std::vector<osg::ref_ptr<osg::FrameBufferObject> > mIOMipmapFBO;
    
            //! Store number of mipmap levels
            int mNumLevels;
    };

};

#endif
