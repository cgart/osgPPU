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


/**
 * Default postprocess effect. Pass input texture to the frame buffer. Use this ppu 
 * to render results of the previous ppus into the framebuffer. So it is usual that
 * this ppu is applied at the end of the pipeline
 **/
class PostProcessUnitOut : public PostProcessUnit {
    public:
    
        //! Create default ppfx 
        //PostProcessUnitOut(osg::State* state, osg::StateSet* parentStateSet);
        PostProcessUnitOut(PostProcess* parent);
        
        //! Release it and used memory
        ~PostProcessUnitOut();
        
        //! Initialze the default postprocessing unit 
        virtual void init();
        
    private:
        //! Apply the defule unit 
        virtual void render(int mipmapLevel = 0);
        
        //! Notice about end of rendering
        virtual void noticeFinishRendering() {}
    
        //! Viewport changed
        virtual void noticeChangeViewport() {}
};

#if 0
/**
 * Screen capturing ppu. The input texture is captured into a file.
 * This ppu allows to render out in higher resolution than your
 * monitor supports. This can be only achieved if your rendering
 * is going completely through ppu pipeline, so renderer in offscreen mode.
 **/
class PostProcessUnitOutCapture : public PostProcessUnitOut {
    public:
    
        //! Create default ppfx 
        PostProcessUnitOutCapture(osg::State* state, osg::StateSet* parentStateSet);
        
        //! Release it and used memory
        ~PostProcessUnitOutCapture();
        
        //! Initialze the default postprocessing unit 
        void init();
        
    private:
        //! Do not do anything 
        void render(int mipmapLevel = 0);
        
        //! Here we are capturing the input to file
        void noticeFinishRendering();
    
};
#endif


/**
 * InOut PPU, does render the content of input image with applied shader 
 * to the output texture. Rendering is done in background, so no information
 * will leack to the frame buffer
 **/
class PostProcessUnitInOut : public PostProcessUnit {
    public:
    
        //! Create default ppfx 
        //PostProcessUnitInOut(osg::State* state, osg::StateSet* parentStateSet);
        PostProcessUnitInOut(PostProcess* parent);
        
        //! Release it and used memory
        virtual ~PostProcessUnitInOut();
        
        //! Initialze the default postprocessing unit 
        virtual void init();
        
        //! Set to true if in/out shoudl also be done for mipmap levels
        virtual void setMipmappedIO(bool b);
    
        //! Are we using IO also for mipmap-levels
        virtual bool getMipmappedIO() const { return mbMipmappedIO; }

    protected:
    
        //! Apply the defule unit 
        virtual void render(int mipmapLevel = -1);
        virtual void doRender(int mipmapLevel);
    
        //! Notice about end of rendering
        virtual void noticeFinishRendering() {}
        
        //! Viewport changed
        virtual void noticeChangeViewport();

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
class PostProcessUnitInResampleOut : public PostProcessUnit {
    public:
    
        //! Create default ppfx 
        //PostProcessUnitInResampleOut(osg::State* state, osg::StateSet* parentStateSet);
        PostProcessUnitInResampleOut(PostProcess* parent);
        
        //! Release it and used memory
        ~PostProcessUnitInResampleOut();
        
        //! Initialze the default postprocessing unit 
        void init();
            
        //! Set resampling factor
        void setFactor(float x, float h);

    private:
        float mWidthFactor, mHeightFactor;
        int mOrigWidth, mOrigHeight;
        bool mDirtyFactor;

        void render(int mipmapLevel = 0);
        
        //! Viewport changed
        void noticeChangeViewport();

};

#endif
