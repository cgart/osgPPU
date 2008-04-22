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

#ifndef _C_UNIT_INOUT_H_
#define _C_UNIT_INOUT_H_


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgPPU/Export.h>
#include <osgPPU/Unit.h>

#include <osg/FrameBufferObject>

#define OSGPPU_MIPMAP_LEVEL_UNIFORM "osgppu_MipmapLevel"
#define OSGPPU_MIPMAP_LEVEL_NUM_UNIFORM "osgppu_MipmapLevelNum"

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
            META_Node(osgPPU,UnitInOut);
        
            //! Create default ppfx 
            UnitInOut();
            UnitInOut(const UnitInOut&, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
            
            //! Release it and used memory
            virtual ~UnitInOut();
            
            //! Initialze the default Processoring unit
            virtual void init();
            
            /**
            * Get framebuffer object used by this ppu. 
            **/
            inline osg::FrameBufferObject* getFrameBufferObject() { return mFBO.get(); }

            /**
            * Return output texture for the specified MRT index.
            * If no such exists, then it will be allocated.
            **/
            virtual osg::Texture* getOrCreateOutputTexture(int mrt = 0);

            /**
            * UnitInOut can also be used to bypass the input texture to the output
            * and perform a rendering on it. This is differently to the UnitBypass which
            * do not perform any rendering but bypasses the data. 
            * Specify here the index of the input unit,
            * to bypass the input to the output.
            * @param index Index of an input unit to bypass to output. Specify -1, to 
            * disable this feature.
            **/
            void setInputBypass(int index);

            /**
            * Get bypassed input texture index.
            **/
            int getInputBypass() const { return mBypassedInput; }

        protected:
        
            //! Notice about end of rendering
            virtual void noticeFinishRendering(osg::RenderInfo &renderInfo, const osg::Drawable* drawable) {};

            //! Notice about begin of rendering
            virtual void noticeBeginRendering(osg::RenderInfo &renderInfo, const osg::Drawable* drawable) {};
            
            //! Viewport changed
            virtual void noticeChangeViewport();
    
            //! called when input textures are changed
            virtual void noticeChangeInput() {}
    
            //! Reassign fbo if output textures changes
            virtual void assignOutputTexture();
    
            virtual void noticeAssignShader() {}
            virtual void noticeRemoveShader() {}

            void assignFBO();

            //! Framebuffer object where results are written
            osg::ref_ptr<osg::FrameBufferObject>    mFBO;    

            //! index of the bypassed input
            int mBypassedInput;
    };

};

#endif
