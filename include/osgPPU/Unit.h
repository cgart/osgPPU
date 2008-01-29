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

#ifndef _C_UNIT_H_
#define _C_UNIT_H_


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

#include <osgPPU/Export.h>
#include <osgPPU/Shader.h>

namespace osgPPU
{

// Forward declaration to simplify the work
class Processor;

//! Base class of any ppu which do work as a simple bypass ppu per default
/**
 * PostProcessing Effect. The effects
 * are used by PostProcess in their order to apply some
 * processing computation onto the input data.
 * 
 * Per defualt the class PostProcessUnit is an empty effect, hence it just bypass
 * all the data (direct connection from input to output).
 **/
class OSGPPU_EXPORT Unit : public osg::Object {
    public:

        META_Object(osgPPU,Unit);
        typedef std::map<int, osg::ref_ptr<osg::Texture> > TextureMap;
        
        /**
        * Empty constructor. The unit will be initialized with default values.
        * Call setState() or setParentProcessor() afterwards to provide the unit with correct 
        * state data otherwise strange things can happen.
        **/
        Unit();

        /**
         * Initialze the unit and setup the state which is used for rendering.
         **/
        Unit(osg::State* state);

        /**
         * Copy constructor.
        **/
        Unit(const Unit&, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

        /**
        * Release used memory by the ppu. 
        **/        
        virtual ~Unit();
        
        /**
         * Get framebuffer object used by this ppu. Almost every ppu do work 
         * with fbos. If there is no use in fbo, then NULL is returned.
        **/
        inline osg::FrameBufferObject* getFBO() { return mFBO.get(); }

        /**
        * Set a texture as input texture.
        * @param inTex Texture which is handled as input 
        * @param inputIndex Index, will be used as texture unit to apply the input on
        **/        
        void setInputTexture(osg::Texture* inTex, int inputIndex);
        
        /**
         * Return an input texture of a certain index.
         * @param inputIndex Index of the input texture (index is equal to the texture unit)
        **/
        inline osg::Texture* getInputTexture(int inputIndex) { return mInputTex[inputIndex].get(); }

        //! Assign input textures directly by a index to texture map
        inline void setInputTextureMap(const TextureMap& map) { mInputTex = map; }

        //! Return complete index to texture mapping
        const TextureMap& getInputTextureMap() const {return mInputTex;}

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
        * Set an output texture.
        * @param outTex Texture used as output of this ppu 
        * @param mrt MRT (multiple rendering target) index of this output
        **/
        void setOutputTexture(osg::Texture* outTex, int mrt = 0);
        
        //! Get output texture of certain MRT index
        inline osg::Texture* getOutputTexture(int mrt = 0) { return mOutputTex[mrt].get(); }

        //! Set a mrt to texture map for output textures
        inline void setOutputTextureMap(const TextureMap& map) { mOutputTex = map;}

        //! Get mrt index to texture mapping
        inline const TextureMap& getOutputTextureMap() const {return mOutputTex;}
                
        /**
        * Set new input ppu. The order of adding the ppus corresponds to their 
        * input indicies. The output of the ppu added as second will be mapped
        * to the input indices 1 + number of output textures.
        * @param ppu PPU which will be used as input.
        * @param bUsePPUsViewport Should we use the viewport of that ppu to setup our own.
        **/
        inline void addInputPPU(Unit* ppu, bool bUsePPUsViewport = false)
        {
            mInputPPU.push_back(ppu);
            if (bUsePPUsViewport)
            {
                mUseInputPPUViewport = ppu;
                setInputTextureIndexForViewportReference(-1);
            }
        }

        //! Return the index of this unit
        inline int getIndex() const { return mIndex; }
        
        //! Set index of this ppu
        inline void setIndex(int i) { mIndex = i; }
        
        /**
        * Initialze the unit. This method should be overwritten by the
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
        
        /**
         * Set the current time directly.
         * @param f Time in seconds.
        **/
        inline void setTime(float f) { mTime = f; }

        //! Comparison operator for sorting. Compare by index value
        inline bool operator < (const Unit& b)
        {
               return mIndex < b.mIndex;
        }
        
        //! Set viewport which is used for this PPU while rendering 
        void setViewport(osg::Viewport* vp);

        //! Get viewport of this unit
        inline osg::Viewport* getViewport() const { return mViewport.get(); }
        
        inline void setEndBlendTime(float time) { mExpireTime = time; }
        inline void setBlendDuration(float time) { mExpireTime = mStartTime + time; }
        inline float getEndBlendTime() const { return mExpireTime; }

        inline void setStartBlendTime(float time) { mStartTime = time; }
        inline void setStartBlendTimeToCurrent() { mStartTime = mTime; }
        inline float getStartBlendTime() const { return mStartTime ; }

        inline void setStartBlendValue(float alpha) { mStartBlendValue = alpha; }
        inline float getStartBlendValue() const { return mStartBlendValue; }

        inline void setEndBlendValue(float alpha) { mEndBlendValue = alpha; }
        inline float getEndBlendValue() const { return mEndBlendValue; }
        inline float getCurrentBlendValue() const { return mCurrentBlendValue; }

        //! Should the blend mode be activated or not        
        void setBlendMode(bool enable);

        //! Do we currently use blend mode
        bool useBlendMode() const; 
        
        /**
         * Activate or deactive the ppu. An active ppu is updated during the update 
         * of the post processor.
         * @param b True to activate, false to deactive
        **/
        inline void setActive(bool b) { mbActive = b; }

        //! Check active status
        inline bool isActive() const { return mbActive; }
        
        /**
         * Change drawing position and size of this ppu. This changes the projection matrix,
         * therefor it is better not to change this parameters until you really 
         * need this. If you just want to place the ppu on another position, then just 
         * play with the viewport.
        **/
        void setRenderingPosAndSize(float left, float top, float right, float bottom);
        
		/**
		* Set this ppu in the mode, so that it is not combined into the rendering graph.
		* This means its output will not be connected to input of the next ppu. Thus the 
		* rendering is done offline in the manner of ppu graph. You have to setup
        * the input and the output of the offline ppus by yourself. Offline ppus
        * are used for a pipeline independent computation on the input data.
		**/
		inline void setOfflineMode(bool mode) {mbOfflinePPU = mode;}

		//! Check whenever this ppu runs in an offline mode 
		inline bool getOfflineMode() const { return mbOfflinePPU; }

        /**
         * Set internal format which will be used by creating the textures. The format
         * specified here will be passed along to the osg::Texture::setInternalFormat()
         * method when creating output textures of a corresponding ppu.
        **/
        void setOutputInternalFormat(GLenum format);

        //! Get internal format which is used by the output textures
        inline GLenum getOutputInternalFormat() const { return mOutputInternalFormat; }

        /**
         * Assign a shader used when computing the output data of the ppu.
         * Shaders are one of the main aspects of the ppu rendering.
         * @param sh Shader used by this ppu to generate output from the input.
        **/
        inline void setShader(Shader* sh)
        { 
            mShader = sh;
            mbDirtyShader = true;
            //assignShader(); 
        }

        //! Get currently assigned shader
        inline Shader* getShader() const { return mShader.get(); }

        /**
         * Assign a mipmap shader. A mipmap shader is used when generating mipmaps
         * on the output data. Hence this shader is only applied to all the mipmap levels
         * except of level 0, where a normal shader specified by setShader() is applied.
        **/
        inline void setMipmapShader(Shader* sh) { mMipmapShader = sh; mbUseMipmapShader = (sh != NULL); }

        //! Return current mipmap shader
        inline Shader* getMipmapShader() const { return mMipmapShader.get(); }
        
        //! Shall we use mipmap shader to generate mipmaps
        inline void setUseMipmapShader(bool b) { mbUseMipmapShader = b; }
        inline bool getUseMipmapShader() const { return mbUseMipmapShader; }

        //! Shall we use mipmaps at all 
        inline void setUseMipmaps(bool b) { mbUseMipmaps = b; if (b) enableMipmapGeneration(); }
        inline bool getUseMipmaps() const { return mbUseMipmaps; }

        //! Set user data
        inline void setUserData(void* data) { mUserData = data; }

        //! get user data
        inline void* getUserData() { return mUserData; }

        /**
        * Set index of an input texture which size is used as reference 
        * for the viewport size. The viewport size will be changed according
        * to the texture size. If you change the input texture the size will
        * be also changed. Specify -1 if you do not want to have this behaviour.
        * If -1, then by next change of the input texture the viewport size 
        * will not be changed.
        **/
        void setInputTextureIndexForViewportReference(int index);

        //! Get index of the viewport reference texture 
        inline int getInputTextureIndexForViewportReference() const { return mInputTexIndexForViewportReference; }

        //! Get stateset of the ppu 
        inline osg::StateSet* getStateSet() { return sScreenQuad->getOrCreateStateSet(); }

        /**
        * Set state which is used to render the unit. 
        **/
        inline void setState(osg::State* state)
        {
            if (state) sState.setState(state);
        }

    protected:

        //! it is good to have friends
        friend class Processor;

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
        
        //! Enable mipmap generation on all output textures
        void enableMipmapGeneration();

        /**
         * Set camera which is used for this ppu. The camera attachments might be used as inputs.
         * However it is up to the definition of the ppu to use camera inputs or not.
         * @param camera Camera for a potential use of inputs from.
        **/
        inline void setCamera(osg::Camera* camera) { mCamera = camera; }

        //! Return camera associated with this ppu.
        inline osg::Camera* getCamera() { return mCamera.get(); }

        //! Each ppfx use its own framebuffer object where results are written
        osg::ref_ptr<osg::FrameBufferObject>    mFBO;
        
        //! Input texture
        TextureMap  mInputTex;
        
        //! Output textures
        TextureMap  mOutputTex;
        
        //! Shader which will be used for rendering
        osg::ref_ptr<Shader>   mShader;
        
        //! Index of this unit in the pipeline
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
        std::vector<osg::ref_ptr<Unit> > mInputPPU;
        
        //! Resource pointer to the ppu which is used as output (if such is specified)
        std::vector<osg::ref_ptr<Unit> > mOutputPPU;
                
        //! Name of the ppu wich viewport should be used
        Unit* mUseInputPPUViewport;
        
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

		//! This ppu is marked as not to be included into the rendering graph 
		bool mbOfflinePPU;

        //! Internal format of the output texture
        GLenum mOutputInternalFormat;

        //! Index of the input texture which size is used as viewport
        int mInputTexIndexForViewportReference;

        //! Check if unit does contain a valid state (hence i
        bool isValidState() const { return sState.getState() != NULL; }

    private:
        bool mbActive;        
        float mExpireTime;
        float mStartTime;
        float mStartBlendValue;
        float mEndBlendValue;
        float mCurrentBlendValue;
        osg::ref_ptr<osg::BlendFunc> mBlendFunc;        
        float mTime;
        void* mUserData;

        void initialize();

};

};

#endif
