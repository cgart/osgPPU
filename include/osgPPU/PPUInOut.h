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

#ifndef _C_UNIT_IO_H_
#define _C_UNIT_IO_H_


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgPPU/Unit.h>

namespace osgPPU
{

//! Output the input to the frame buffer instead to the output texture
/**
 * Pass input texture to the frame buffer. Use this ppu
 * to render results of the previous ppus into the framebuffer. So it is usual that
 * this ppu is applied at the end of the pipeline
 **/
class UnitOut : public Unit {
    public:
    
        //! Create default ppfx 
        UnitOut(Processor* parent);
        
        //! Release it and used memory
        virtual ~UnitOut();
        
        //! Initialze the default Processoring unit
        virtual void init();
        
    protected:
        //! Apply the defule unit 
        virtual void render(int mipmapLevel = 0);
        
        //! Notice about end of rendering
        virtual void noticeFinishRendering() {}
    
        //! Viewport changed
        virtual void noticeChangeViewport() {}
};


//! Capture the content of the input texture to a file
/**
 * Screen capturing ppu. The input texture is captured into a file.
 * This ppu allows to render out in higher resolution than your
 * monitor supports. This can be only achieved if your rendering
 * is going completely through ppu pipeline, so renderer in offscreen mode.
 **/
class UnitOutCapture : public UnitOut {
    public:
    
        //! Create default ppfx 
        UnitOutCapture(Processor* parent);
        
        //! Release it and used memory
        virtual ~UnitOutCapture();
        
        //! Set path were to store the screenshots
        void setPath(const std::string& path) { mPath = path; }

        //! set extension   
        void setFileExtensions(const std::string& ext) { mExtension = ext; }

    protected:
        
        //! Here we are capturing the input to file
        void noticeFinishRendering();
    
        //! path were to store the files
        std::string mPath;

        //! Current number of the capture file 
        int mCaptureNumber;

        //! file extensions
        std::string mExtension;
};


//! Compute output texture based on the assigned shaders and input data
/**
 * InOut PPU, does render the content of input textures with applied shader 
 * to the output textures. Rendering is done in background, so no information
 * will leack to the frame buffer
 **/
class UnitInOut : public Unit {
    public:
    
        //! Create default ppfx 
        UnitInOut(Processor* parent);
        
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


//! Same as UnitInOut but do resampling inbetween
/**
 * Resample the input. This PPU will 
 * render the input data resampled to the output. Next PPU will work 
 * on the resampled one. NOTE: You loose information in your data after 
 * appling this PPU.
 **/
class UnitInResampleOut : public UnitInOut {
    public:
    
        //! Create default ppfx 
        UnitInResampleOut(Processor* parent);
        
        //! Release it and used memory
        virtual ~UnitInResampleOut();
        
        //! Set resampling factor
        void setFactor(float x, float h);

    protected:
        float mWidthFactor, mHeightFactor;
        bool mDirtyFactor;

        //! Overwritten render method
        void render(int mipmapLevel = 0);

};

};

#endif
