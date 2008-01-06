/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _C_POST_PROCESS_UNIT_H_
#define _C_POST_PROCESS_UNIT_H_


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
#include <osg/Uniform>

#include <osgPPU/Shader.h>

namespace osgPPU
{

class PostProcess;

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

        META_Object(osgPPU,PostProcessUnit)
        typedef std::map<int, osg::ref_ptr<osg::Texture> > TextureMap;
        typedef std::map<int, std::string > UniformMap;
        
        /**
         * Create empty ppu. If you use this ppu in your graph you will
         * do not encounter anything, since this ppu do just copy the input 
         * data to its output, by setting output textures equal to the input 
         * @param parent Parent post processing class from where we borrow the state and stateset
         **/
        PostProcessUnit(PostProcess* parent);

        /**
         * Copy constructor.
        **/
        PostProcessUnit(const PostProcessUnit&, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

        /**
        * Release used memory by the ppu. 
        **/        
        virtual ~PostProcessUnit();
        
        //! Set state, which is used to apply the ppu on
        void setState(osg::State* state);

        //! Get framebuffer object 
        osg::FrameBufferObject* getFBO() { return mFBO.get(); }

        /**
        * Set a texture as input texture.
        * @param inTex Texture which is handled as input 
        * @param inputIndex Index, will be used as texture unit to apply the input on
        **/        
        void setInputTexture(osg::Texture* inTex, int inputIndex);
        
        //! Return a texture of a certain index
        osg::Texture* getInputTexture(int inputIndex) { return mInputTex[inputIndex].get(); }

        //! Return complete index to texture mapping
        const TextureMap& getInputTextureMap() const {return mInputTex;}

        //! Assign input textures directly by a index to texture map
        void setInputTextureMap(const TextureMap& map) { mInputTex = map; }

        /**
        * Input textures for a ppu can be used in a shader program. To accomplish
        * that we have to specify the uniforms which are set to the input texture index value.
        * This method allows you to specify uniform name in the program to which  
        * the input texture from the given index is associated.
        * @param index Index which will be set as a value to this uniform. The index
        *        value must be the same as the index of your input texture.
        * @param name Unique name of the uniform of the input texture 
        **/
        void bindInputTextureToUniform(int index, const std::string& name);

        /**
        * Instead of adding uniforms you can set the complete list of uniforms.
        * The list should contain the uniforms for the texture samples with correct values.
        * If you call afterwards setInputTextureUniformName() the list will be cleared
        * and setted up again.
        **/
        //void setUniformList(const osg::StateSet::UniformList& list);
        //const osg::StateSet::UniformList& getUniformList() { return mUniforms; }

        /**
        * Set an output texture.
        * @param outTex Texture used as output of this ppu 
        * @param mrt MRT (multiple rendering target) index of this output
        **/
        void setOutputTexture(osg::Texture* outTex, int mrt = 0);
        
        //! Get output texture of certain MRT index
        osg::Texture* getOutputTexture(int mrt = 0);

        //! Get mrt index to texture mapping
        const TextureMap& getOutputTextureMap() const {return mOutputTex;}
                
        /**
        * Set new input ppu. The order of adding the ppus corresponds to their 
        * input indicies.
        * @param ppu PPU which will be used as input.
        * @param bUsePPUsViewport Should we use the viewport of that ppu
        **/
        inline void addInputPPU(PostProcessUnit* ppu, bool bUsePPUsViewport = false)
        {
            mInputPPU.push_back(ppu);
            if (bUsePPUsViewport)
            {
                mUseInputPPUViewport = ppu;
                setInputTextureIndexForViewportReference(-1);
            }
        }

        //! Return the index of this postprocessing unit
        int getIndex() const { return mIndex; }
        
        //! Set index of this ppu
        void setIndex(int i) { mIndex = i; }
        
        /**
        * Initialze the postprocessing unit. This method should be overwritten by the 
        * derived classes to support non-standard initialization routines.
        **/
        virtual void init();
        
        /**
        * Apply the unit to the input data and store them at the output.
        * This will bind the ppu and apply the shader on its input.
        * Shader parameter will be specified by the input textures.
        * Shader output will be stored in the output textures.
        * @param dTime Time difference between two calls of apply method. 
        *               The current time will be derived from this value.
        **/
        virtual void apply(float dTime = 0.0f);
        
        //! Set the current time directly
        void setTime(float f) { mTime = f; }

        //! Comparison operator for sorting. Compare by index value
        bool operator < (const PostProcessUnit& b)
        {
               return mIndex < b.mIndex;
        }
        
        //! Set viewport which is used for this PPU while rendering 
        void setViewport(osg::Viewport* vp);

        //! Get viewport of this unit
        osg::Viewport* getViewport() { return mViewport.get(); }
        
        //! Set camera which is used for this ppu. The camera attachments might be used as inputs
        void setCamera(osg::Camera* cam) { mCamera = cam; }
        osg::Camera* getCamera() { return mCamera.get(); }

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
		* rendering is done offline in the manner of ppu graph.
		**/
		inline void setOfflineMode(bool mode) {mbOfflinePPU = mode;}

		//! Check whenever this ppu runs in an offline mode 
		inline bool getOfflineMode() const { return mbOfflinePPU; }

        //! Set internal format which will be used by creating the textures
        void setOutputInternalFormat(GLenum format);

        //! Get internal format which is used by the output textures
        GLenum getOutputInternalFormat() const { return mOutputInternalFormat; }

        //! Utility function to derive source texture format from the internal format
        static GLenum createSourceTextureFormat(GLenum internalFormat);

        //! Set new shader 
        void setShader(Shader* sh)
        //void setShader(osg::Program* sh)
        { 
            mShader = sh;
            mbDirtyShader = true;
            //assignShader(); 
        }

        //! Get currently bounded shader program
        Shader* getShader() { return mShader.get(); }
        //osg::Program* getShader() { return mShader.get(); }

        //! Set mipmap shader 
        void setMipmapShader(Shader* sh) { mMipmapShader = sh; mbUseMipmapShader = (sh != NULL); }
        inline Shader* getMipmapShader() { return mMipmapShader.get(); }
        
        //! Shall we use mipmap shader
        void setUseMipmapShader(bool b) { mbUseMipmapShader = b; }

        //! Shall we use mipmaps
        void setUseMipmaps(bool b) { mbUseMipmaps = b; }

        //! Enable mipmap generation on all output textures
        void enableMipmapGeneration();

        //! Set user data
        void setUserData(void* data) { mUserData = data; }

        //! get user data
        void* getUserData() { return mUserData; }

        /**
        * Set index of an input texture which size is used as reference 
        * for the viewport size. The viewport size will be changed according
        * to the texture size. If you change the input texture the size will
        * be also changed. Specify -1 if you do not want to have this behaviour.
        * If -1, then by next change of the input texture the viewport size 
        * will not be changed.
        **/
        void setInputTextureIndexForViewportReference(int index);

        //! get index of the viewport reference texture 
        int getInputTextureIndexForViewportReference() const { return mInputTexIndexForViewportReference; }

        //! Get stateset of the ppu 
        osg::StateSet* getStateSet() { return sScreenQuad->getOrCreateStateSet(); }

    protected:
        //! We do not want the user to use this method directly
        PostProcessUnit();

        //! it is good to have friends
        friend class PostProcess;

        //! Notice derived units about end of rendering
        virtual void noticeFinishRendering() {}

        //! Notice underlying classes, that viewport size is changed
        virtual void noticeChangeViewport() {}
        
        //! Notice derived classes, when inpu ttexture was changed.
        virtual void noticeChangeInput() {}

        //! Notice derived classes, that new shader is assigned
        virtual void noticeAssignShader() {}

        //! Notice derived classes, that no shader is assigned now
        virtual void noticeRemoveShader() {}

        //! Unit specific rendering function 
        virtual void render(int mipmapLevel = -1);
                    
        //! Assign the input texture to the quad object 
        virtual void assignInputTexture();

        //! Assign output textures (is handled only in derived classes)
		virtual void assignOutputTexture() {};
        
        //! Callback when ppu is applied (can be used to update the ppu)
        virtual void noticeOnApply() {};

        //! Apply base parameters (should be called from derived render method ).
        bool applyBaseRenderParameters();
        
        //! Assign a shader to the input texture to the quad object 
        void assignShader();
        
        //! disable shader
        void removeShader();

        //! Call this method to initilization the base class properties (i.e. in init() before return)
        void initializeBase();
        
        //! Call this method after the apply method in derived classes
        void applyBase();
        
        //! Generate mipmaps (for specified output texture)
        void generateMipmaps(osg::Texture* output, int mrt);
        
        //! Each ppfx use its own framebuffer object where results are written
        osg::ref_ptr<osg::FrameBufferObject>    mFBO;
        
        //! Input texture
        TextureMap  mInputTex;
        
        //! Output textures
        TextureMap  mOutputTex;

        //! Uniform map which maps uniform to texture index
        //UniformMap mUniformMap;

        //! List of all uniforms
        //osg::StateSet::UniformList mUniforms;
        
        //! Shader which will be used for rendering
        osg::ref_ptr<Shader>   mShader;
        
        //! Uniform to set the mipmap level 
        //osg::ref_ptr<osg::Uniform> mShaderMipmapLevelUniform;

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
        osg::ref_ptr<Shader> mMipmapShader;
        
        //! FBOs for different mipmap levels
        std::vector<osg::ref_ptr<osg::FrameBufferObject> > mMipmapFBO;
        
        //! Viewports for each mipmap level 
        std::vector<osg::ref_ptr<osg::Viewport> > mMipmapViewport;
        
        //! Camera which is used for this post process
        osg::ref_ptr<osg::Camera> mCamera;

        //! Mark if the viewport is dirty (so have changed)
        bool mbDirtyViewport;

		//! mark if input textures are dirty
		bool mbDirtyInputTextures;

		//! Mark if output textures are dirty
		bool mbDirtyOutputTextures;

        //! Dirty Shader
        bool mbDirtyShader;

        //! Current color fo the geometry quad
        osg::ref_ptr<osg::Vec4Array> mScreenQuadColor;

		//! This ppu is marked as not to be included into the rendering graph 
		bool mbOfflinePPU;

        //! Internal format of the output texture
        GLenum mOutputInternalFormat;

        //! Pointer to the parent post process 
        osg::ref_ptr<PostProcess> mParent;

        //! Index of the input texture which size is used as viewport
        int mInputTexIndexForViewportReference;

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
        void initialize(PostProcess* parent);//osg::State* parentState, osg::StateSet* parentStateSet);

        //! current time
        float mTime;

        //! user data 
        void* mUserData;
};

};

#endif
