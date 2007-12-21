/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _C_POST_PROCESS_UNIT_IO_H_
#define _C_POST_PROCESS_UNIT_IO_H_


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgPPU/PostProcessUnit.h>

namespace osgPPU
{

/**
 * Default postprocess effect. Pass input texture to the frame buffer. Use this ppu 
 * to render results of the previous ppus into the framebuffer. So it is usual that
 * this ppu is applied at the end of the pipeline
 **/
class PostProcessUnitOut : public PostProcessUnit {
    public:
    
        //! Create default ppfx 
        PostProcessUnitOut(PostProcess* parent);
        
        //! Release it and used memory
        virtual ~PostProcessUnitOut();
        
        //! Initialze the default postprocessing unit 
        virtual void init();
        
    protected:
        //! Apply the defule unit 
        virtual void render(int mipmapLevel = 0);
        
        //! Notice about end of rendering
        virtual void noticeFinishRendering() {}
    
        //! Viewport changed
        virtual void noticeChangeViewport() {}
};

/**
 * Screen capturing ppu. The input texture is captured into a file.
 * This ppu allows to render out in higher resolution than your
 * monitor supports. This can be only achieved if your rendering
 * is going completely through ppu pipeline, so renderer in offscreen mode.
 **/
class PostProcessUnitOutCapture : public PostProcessUnitOut {
    public:
    
        //! Create default ppfx 
        PostProcessUnitOutCapture(PostProcess* parent);
        
        //! Release it and used memory
        virtual ~PostProcessUnitOutCapture();
        
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


/**
 * InOut PPU, does render the content of input image with applied shader 
 * to the output texture. Rendering is done in background, so no information
 * will leack to the frame buffer
 **/
class PostProcessUnitInOut : public PostProcessUnit {
    public:
    
        //! Create default ppfx 
        PostProcessUnitInOut(PostProcess* parent);
        
        //! Release it and used memory
        virtual ~PostProcessUnitInOut();
        
        //! Initialze the default postprocessing unit 
        virtual void init();
        
        //! Set to true if in/out shoudl also be done for mipmap levels
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


/**
 * Resample the input and output it to resultings image. This PPU will 
 * render the image resampled to an offscreenbuffer. Next PPU will work 
 * on the resampled one. NOTE: You loose information in your data after 
 * appling this PPU.
 **/
class PostProcessUnitInResampleOut : public PostProcessUnitInOut {
    public:
    
        //! Create default ppfx 
        PostProcessUnitInResampleOut(PostProcess* parent);
        
        //! Release it and used memory
        virtual ~PostProcessUnitInResampleOut();
        
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
