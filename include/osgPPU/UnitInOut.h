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
            void setMipmappedInOut(bool b);
        
            /**
            * Check whenever we using IO also for mipmap-levels
            **/
            bool getMipmappedInOut() const { return mbMipmappedIO; }
    
            /**
            * Assign a mipmap shader. A mipmap shader is used when generating mipmaps
            * on the output data. Hence this shader is only applied to all the mipmap levels
            * except of level 0, where a normal shader specified by setShader() is applied.
            **/
            inline void setGenerateMipmapsShader(Shader* sh) { mMipmapShader = sh; mbUseMipmapShader = (sh != NULL); }

            /**
            * Return currently used mipmap shader.
            **/
            inline Shader* getGenerateMipmapsShader() const { return mMipmapShader.get(); }
            
            /**
            * Shall we use mipmap shader to generate mipmaps
            **/
            inline void setUseGenerateMipmapsShader(bool b) { mbUseMipmapShader = b; }
            inline bool getUseGenerateMipmapsShader() const { return mbUseMipmapShader; }

            /**
            * Shall we use mipmapping at all, hence shall we generate mipmaps on the output textures.
            **/
            inline void setUseMipmaps(bool b) { mbUseMipmaps = b; if (b) enableMipmapGeneration(); }
            inline bool getUseMipmaps() const { return mbUseMipmaps; }
    
            /**
            * Get framebuffer object used by this ppu. 
            **/
            inline osg::FrameBufferObject* getFrameBufferObject() { return mFBO.get(); }

            /**
            * Specify the number of active draw buffers during the rendering.
            * In the normal case only the rendering targets are active for those an output
            * texture was specified. However you can overwrite this behaviour and
            * enable a fix number of mrt outputs. For example specify here 3 and
            * the MRT 0, 1 and 2 will be activated.
            * @param mrtCount Number of active mrt (-1 for default)
            **/
            inline void setMRTNumber(int mrtCount) { mMRTCount = mrtCount; }

            /**
            * Return current value setted with setMRTNumber()
            **/
            inline int getMRTNumber() const { return mMRTCount; }

        protected:
        
            //! Apply the defule unit 
            virtual void render(int mipmapLevel = -1);
            virtual void doRender(int mipmapLevel);
        
            //! Notice about end of rendering
            virtual void noticeFinishRendering();
            
            //! Viewport changed
            virtual void noticeChangeViewport();
    
            //! called when input textures are changed
            virtual void noticeChangeInput() {}
    
            //! Reassign fbo if output textures changes
            virtual void assignOutputTexture();
    
            virtual void noticeAssignShader() {}
            virtual void noticeRemoveShader() {}

            void assignFBO();

            //! regenerate io mapmapped data structures
            void checkIOMipmappedData();

            //! Generate mipmaps (for specified output texture)
            void generateMipmaps(osg::Texture* output, int mrt);
            
            //! Enable mipmap generation on all output textures
            void enableMipmapGeneration();
    
            //! Should we do in/out also on mipmap levels
            bool mbMipmappedIO;
            
            //! Viewports for each mipmap level 
            std::vector<osg::ref_ptr<osg::Viewport> > mIOMipmapViewport;
            
            //! Fbos for each mipmap level 
            std::vector<osg::ref_ptr<osg::FrameBufferObject> > mIOMipmapFBO;
            
            //! Framebuffer object where results are written
            osg::ref_ptr<osg::FrameBufferObject>    mFBO;
    
            //! Number of active draw buffers
            int mMRTCount;

            //! Store number of mipmap levels
            int mNumLevels;
    
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

    };

};

#endif
