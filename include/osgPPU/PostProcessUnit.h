/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _C_POST_PROCESS_FX_UNITS_H_
#define _C_POST_PROCESS_FX_UNITS_H_


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osg/Texture>
#include <osg/Camera>
#include <osg/State>
#include <osg/FrameBufferObject>
#include <osg/Geometry>
#include <osg/BlendFunc>
#include <osg/BlendColor>
#include <osg/Material>
#include <osg/Program>


/**
 * PostProcessing Effect. This is a postprocessing effect. The effects
 * are used by PostProcess in their order to apply some
 * post processing effects onto the frame buffer.
 * 
 * The effects are applied in a pipeline, so the output of the 
 * effect is an input for another effect. At the end of the pipeline there should be
 * a bypass effect which does render the result into the frame buffer.
 * 
 * Per defualt the class PostProcessUnit is an empty effect, hence it just bypass
 * all the data (direct connection from input to output).
 **/
class PostProcessUnit : public osg::Object {
    public:

        //! Meta object information
        META_Object(osgPPU,PostProcessUnit)

        //! Create empty ppu
        PostProcessUnit();
        
        //! Create a postprocess effect 
        PostProcessUnit(osg::State* parentState, osg::StateSet* parentStateSet);

        //! Copy one ppu to another
        PostProcessUnit(const PostProcessUnit&, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
        
        //! Remove ppfx and release used memory
        virtual ~PostProcessUnit();
        
        //! Set state
        void setState(osg::State* state);

        //! Get framebuffer object 
        osg::FrameBufferObject* getFBO() { return mFBO.get(); }
        
        //! Set input texture as used for input from previous pass 
        void setInputTexture(osg::Texture* inTex, int inputIndex);
        osg::Texture* getInputTexture(int inputIndex) { return mInputTex[inputIndex].get(); }
        const std::map<int, osg::ref_ptr<osg::Texture> >& getInputTextureMap() const {return mInputTex;}
        void setInputTextureMap(const std::map<int, osg::ref_ptr<osg::Texture> >& map) { mInputTex = map; }
        
        //! Set output texture to specified mrt 
        void setOutputTexture(osg::Texture* outTex, int mrt = 0);
        osg::Texture* getOutputTexture(int mrt = 0);
        const std::map<int, osg::ref_ptr<osg::Texture> >& getOutputTextureMap() const {return mOutputTex;}
                
        //! Set new input ppu 
        inline void addInputPPU(PostProcessUnit* ppu, bool bUsePPUsViewport = false)
        {
            mInputPPU.push_back(ppu);
            if (bUsePPUsViewport) mUseInputPPUViewport = ppu;
        }

        //! get index of this postprocessing unit
        int getIndex() const { return mIndex; }
        
        //! Set index of this pp unit
        void setIndex(int i) { mIndex = i; }
        
        //! Initialze the postprocessing unit 
        virtual void init();
        
        //! Apply the unit to the input data and store them at the resulting output 
        virtual void apply(float dTime = 0.0f);
        
        //! Comparison operator for sorting. Compare by index value
        bool operator < (const PostProcessUnit& b)
        {
               return mIndex < b.mIndex;
        }
        
        //! Set viewport of the camera to which one this PPU is applied 
        void setViewport(osg::Viewport* vp);

        //! Get viewport of this unit
        osg::Viewport* getViewport() { return mViewport.get(); }
        
        //! Set camera which is used for this ppu
        void setCamera(osg::Camera* cam) { mCamera = cam; }

        // Set/get expire time of this ppu
        inline void setExpireTime(float time) { mExpireTime = time; }
        inline float getExpireTime() const { return mExpireTime; }

        // set/get startign time
        inline void setStartTime(float time) { mStartTime = time; }
        inline float getStartTime() const { return mStartTime ; }

        // set/get starting blend value
        inline void setStartBlendValue(float alpha) { mStartBlendValue = alpha; }
        inline float getStartBlendValue() const { return mStartBlendValue; }

        // set/get finish blend value
        inline void setEndBlendValue(float alpha) { mEndBlendValue = alpha; }
        inline float getEndBlendValue() const { return mEndBlendValue; }
        
        // enable/disbael blending mode 
        void setBlendMode(bool enable);
        bool useBlendMode(); 
        
        // activate or deactive the ppu 
        inline void setActive(bool b) { mbActive = b; }
        inline bool isActive() const { return mbActive; }
        
        // get current blend value 
        float getCurrentBlendValue() const { return mCurrentBlendValue; }

        //! Change drawing position and size of this ppu. This changes the projection matrix
        void setRenderingPosAndSize(float left, float top, float right, float bottom);
        
		/**
		* Set this ppu in the mode, so that it is not combined into the rendering graph.
		* This means its output will not be connected to input of the next ppu. Thus the 
		* rendering is done offline
		**/
		inline void setOfflineMode(bool mode) {mbOfflinePPU = mode;}

		//! Check whenever this ppu runs in an offline mode 
		inline bool getOfflineMode() const { return mbOfflinePPU; }

        //! Set internal format which will be used by creating the textures
        void setOutputInternalFormat(GLenum format);

        //! Get internal format which is used by the output textures
        GLenum getOutputInternalFormat() const { return mOutputInternalFormat; }

        static GLenum createSourceTextureFormat(GLenum internalFormat);

    protected:
        friend class PostProcess;

        //! Notice derived units about end of rendering
        virtual void noticeFinishRendering() {}

        //! Notice underlying classes, that viewport size is changed
        virtual void noticeChangeViewport() {}
        
        //! Notice derived classes, when inpu ttexture was changed.
        virtual void noticeChangeInput() {}

        virtual void noticeAssignShader() {}
        virtual void noticeRemoveShader() {}

        //! Enable mipmap generation on all output textures
        void enableMipmapGeneration();
        
        /**
         * apply base parameters (should be called from derived render method ).
         * if return true, so PPU can be rendered otherwise do not render
         **/
        bool applyBaseRenderParameters();
        
        //! Unit specific rendering function 
        virtual void render(int mipmapLevel = -1);
            
        //! Assign the input texture to the quad object 
        void assignInputTexture();
		virtual void assignOutputTexture() {};
        
        //! Assign a shader to the input texture to the quad object 
        void assignShader();
        
        //! disable shader
        void removeShader();
        
        //! Each ppfx use its own framebuffer object where results are written
        osg::ref_ptr<osg::FrameBufferObject>    mFBO;
        
        //! Input texture
        std::map<int, osg::ref_ptr<osg::Texture> >  mInputTex;
        
        //! Output textures
        std::map<int, osg::ref_ptr<osg::Texture> >  mOutputTex;
        
        //! Shader which will be used for rendering
        osg::ref_ptr<osg::Program>   mShader;
        
        //! Uniform to set the mipmap level 
        osg::ref_ptr<osg::Uniform> mShaderMipmapLevelUniform;

        //! Index of this postprocessing unit in the pipeline
        int mIndex;
        
        //! Here we store a screen sized quad, so it can be used for rendering 
        osg::ref_ptr<osg::Geometry> sScreenQuad;
        
        //! Projection matrix of the ppu (default: 2D ortho view)
        osg::Matrixf sProjectionMatrix;
        
        //! Modelview matrix of the ppu (default: identity matrix)
        osg::Matrixf sModelviewMatrix;
         
        //! Local state of the ppu
        osg::RenderInfo      sState;
        
        //! Store here the viewport of the camera, to which one this PPUs are applied 
        osg::ref_ptr<osg::Viewport> mViewport;
        
        //! Resource pointer to the ppu which is used as input (if such is specified)
        std::vector<osg::ref_ptr<PostProcessUnit> > mInputPPU;
        
        //! Resource pointer to the ppu which is used as output (if such is specified)
        std::vector<osg::ref_ptr<PostProcessUnit> > mOutputPPU;
                
        //! Name of the ppu wich viewport should be used
        PostProcessUnit* mUseInputPPUViewport;
        
        //! Should we use mipmapping on the output texture
        bool mbUseMipmaps;
        
        //! Should we use our own mipmapping shader 
        bool mbUseMipmapShader;
        
        //! Pointer to the shader which is used to generate mipmaps
        osg::ref_ptr<osg::Program> mMipmapShader;
        
        //! FBOs for different mipmap levels
        std::vector<osg::ref_ptr<osg::FrameBufferObject> > mMipmapFBO;
        
        //! Viewports for each mipmap level 
        std::vector<osg::ref_ptr<osg::Viewport> > mMipmapViewport;
        
        //! Call this method to initilization the base class properties (i.e. in init() before return)
        void initializeBase();
        
        //! Call this method after the apply method in derived classes
        void applyBase();
        
        //! Generate mipmaps (for specified output texture)
        void generateMipmaps(osg::Texture* output, int mrt);

        //! Camera which is used for this post process
        osg::ref_ptr<osg::Camera> mCamera;

        //! Mark if the viewport is dirty (so have changed)
        bool mbDirtyViewport;

		//! mark if input textures are dirty
		bool mbDirtyInputTextures;

		//! Mark if output textures are dirty
		bool mbDirtyOutputTextures;

        //! Current color fo the geometry quad
        osg::ref_ptr<osg::Vec4Array> mScreenQuadColor;

		//! This ppu is marked as not to be included into the rendering graph 
		bool mbOfflinePPU;

        //! Internal format of the output texture
        GLenum mOutputInternalFormat;

    private:
        //! Flag to setup if PPU is active or not 
        bool mbActive;
        
        //! Expire time
        float mExpireTime;

        //! Start time, when this ppu should start running
        float mStartTime;

        //! Starting alpha value (for blending)
        float mStartBlendValue;

        //! Finish alpha value
        float mEndBlendValue;
        
        //! Current alpha value 
        float mCurrentBlendValue;
        
        //osg::ref_ptr<osg::BlendColor> mBlendColor;
        //osg::ref_ptr<osg::Material> mBlendMaterial;
        osg::ref_ptr<osg::BlendFunc> mBlendFunc;        

        // initialize default vairables
        void initialize(osg::State* parentState, osg::StateSet* parentStateSet);

        //! current time
        float mTime;
};

#if 0
/**
 * Empty postprocess effect. Pass input to the output. So do nothing.
 **/
class PostProcessFXEmpty : public PostProcessFX {
    public:
    
        //! Create default ppfx 
        PostProcessFXEmpty();
        
        //! Release it and used memory
        ~PostProcessFXEmpty();
                
    
};


/**
 * Realyy bypass ppu. Pass input to the output. So do nothing.
 **/
class PostProcessFXBypass : public PostProcessFXEmpty {
    public:
    
        //! Create default ppfx 
        PostProcessFXBypass(osg::State* state, osg::StateSet* parentStateSet);
        
        //! Release it and used memory
        ~PostProcessFXBypass();

        //! Initialze the default postprocessing unit 
        void init();
        
    private:
        //! Apply the defule unit 
        void render(int mipmapLevel = 0);

};

/**
 * Camera DepthMap - Bypass. Do pass the camera depth map texture 
 * as output texture of this ppu. The ppu can be used
 * to access depthmap texture from other ppu's or shaders.
 **/
class PostProcessFXDepthmapBypass : public PostProcessFX {
    public:
    
        //! Create default ppfx 
        PostProcessFXDepthmapBypass(osg::State* state, osg::StateSet* parentStateSet);
        
        //! Release it and used memory
        ~PostProcessFXDepthmapBypass();

        //! Initilize the ppu 
        void init();
        
    private:
        //! Apply the defule unit 
        void render(int mipmapLevel = 0);

};

/**
 * Default postprocess effect. Pass input texture to the frame buffer. Use this ppu 
 * to render results of the previous ppus into the framebuffer. So it is usual that
 * this ppu is applied at the end of the pipeline
 **/
class PostProcessFXOut : public PostProcessFX {
    public:
    
        //! Create default ppfx 
        PostProcessFXOut(osg::State* state, osg::StateSet* parentStateSet);
        
        //! Release it and used memory
        ~PostProcessFXOut();
        
        //! Initialze the default postprocessing unit 
        virtual void init();
        
        //! Check for settings for this PPU
        virtual bool parseXmlSettings(TiXmlElement* root) { return PostProcessFX::parseXmlSettings(root); }

    private:
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
class PostProcessFXOutCapture : public PostProcessFXOut {
    public:
    
        //! Create default ppfx 
        PostProcessFXOutCapture(osg::State* state, osg::StateSet* parentStateSet);
        
        //! Release it and used memory
        ~PostProcessFXOutCapture();
        
        //! Initialze the default postprocessing unit 
        void init();
        
    private:
        //! Do not do anything 
        void render(int mipmapLevel = 0);
        
        //! Here we are capturing the input to file
        void noticeFinishRendering();
    
};



/**
 * InOut PPU, does render the content of input image with applied shader 
 * to the output texture. Renderign is done in background, so no information
 * will leack to the frame buffer
 **/
class PostProcessFXInOut : public PostProcessFX {
    public:
    
        //! Create default ppfx 
        PostProcessFXInOut(osg::State* state, osg::StateSet* parentStateSet);
        
        //! Release it and used memory
        virtual ~PostProcessFXInOut();
        
        //! Initialze the default postprocessing unit 
        virtual void init();
        
        //! Check for settings for this PPU
        virtual bool parseXmlSettings(TiXmlElement* root);

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
 * Same as PostProcessFXInOut but can copy values from the resulting texture to
 * variables or another textures
 **/
class PostProcessFXInOutCopy : public PostProcessFXInOut {
    public:
    
        //! Create default ppfx 
        PostProcessFXInOutCopy(osg::State* state, osg::StateSet* parentStateSet);
        
        //! Release it and used memory
        ~PostProcessFXInOutCopy();
        
        //! Initialze the default postprocessing unit 
        void init();
        
    private:
        //! Notice about end of rendering
        void noticeFinishRendering();

        //! Apply the defule unit
        void render(int mipmapLevel = 0);
        
        //! Check for settings for this PPU
        bool parseXmlSettings(TiXmlElement* root);

        //! Name of the variable where to copy the result of last mipmap level
        std::string mCopyLastMipmapTo;

        //! Level of the mipmap to copy from
        int mMipmapLevel;

        //! Image holding our data
        osg::ref_ptr<osg::Image> mMipmapImage;
        
};

/**
 * Same as PostProcessFXOut but shows some text on the screen.
 * The shown text could be bound to a property
 **/
class PostProcessFXText : public PostProcessFXInOut {
    public:
    
        //! Create default ppfx 
        PostProcessFXText(osg::State* state, osg::StateSet* parentStateSet);
        
        //! Release it and used memory
        ~PostProcessFXText();
        
        //! Initialze the default postprocessing unit 
        void init();
        
        //! Check for settings for this PPU
        bool parseXmlSettings(TiXmlElement* root);

    private:
        //! Apply the defule unit 
        void render(int mipmapLevel = 0);

        //! Text holding our statistics
        SharedPtr<Text> mText;
        
        //! Notice about end of rendering
        void noticeFinishRendering();
        
        //! Property name where to get the text from
        std::string mPropertyName;
        
        //! Sizes of the ppu 
        float mX, mY, mWidth, mHeight, mSize;

        ScriptFunctionDef(setText);
        ScriptFunctionDef(setProperty);
        ScriptFunctionDef(setSize);
        ScriptFunctionDef(setPosition);
        ScriptFunctionDef(setColor);

};

/**
 * Resample the input and output it to resultings image. This PPU will 
 * render the image resampled to an offscreenbuffer. Next PPU will work 
 * on the resampled one. NOTE: You loose information in your data after 
 * appling this PPU.
 **/
class PostProcessFXInResampleOut : public PostProcessFX {
    public:
    
        //! Create default ppfx 
        PostProcessFXInResampleOut(osg::State* state, osg::StateSet* parentStateSet);
        
        //! Release it and used memory
        ~PostProcessFXInResampleOut();
        
        //! Initialze the default postprocessing unit 
        void init();
        
        //! Check for settings for this PPU
        bool parseXmlSettings(TiXmlElement* root);
    
        //! Set resampling factor
        void setFactor(float x, float h);

    private:
        float mWidthFactor, mHeightFactor;
        int mOrigWidth, mOrigHeight;
        bool mDirtyFactor;

        void render(int mipmapLevel = 0);
        
        //! Viewport changed
        void noticeChangeViewport();

        ScriptFunctionDef(setResamplingFactor);

};
#endif

#endif
