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

#ifndef _C_PROCESSOR__H_
#define _C_PROCESSOR__H_


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgPPU/Unit.h>
#include <osg/Camera>
#include <osg/State>

#include <osgPPU/Export.h>
#include <osgPPU/Pipeline.h>

namespace osgPPU
{

//! Main ppu processor used to setup the ppu pipeline
/**
 * Processor should be called as a last step in your rendering pipeline.
 * This class manages the units which do form a graph.
 * To the Processor object a camera is attached.
 * The attached camera must provide a valid viewport and color attachment (texture)
 * which will be used as input for the pipeline.
 * 
 * The ppus are applied in a pipeline, so the output of one 
 * ppu is an input to the next one. At the end of the pipeline there should be
 * a bypassout ppu specified which do render the result into the frame buffer.
 * 
 * A processor can also be used to do some multipass computation on input data.
 * In that case it is not neccessary to output the resulting data on the screen, but
 * you can use the output texture of the last ppu for any other purpose.
 **/
class OSGPPU_EXPORT Processor : public osg::Object {
    public: 
    
        META_Object(osgPPU, Processor);

        /**
         * Initialize the ppu system.
         * @param state Specify a state which will be used to apply the post process statesets
        **/
        Processor(osg::State* state);
         
        Processor(const Processor&, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
    
        /**
         * Release the system. This will free used memory and close all ppus.
        **/
        virtual ~Processor();
        
        /**
         * This should be called every frame to update the system.
         * @param dTime Time (in seconds) of one frame.
        **/
        virtual void update(float dTime = 0.0f);
        
        /**
         * Add a camera which texture attachment can be used as input to the pipeline.
         * The camera object must be setted up to render into a texture.
         * A bypass ppu (Unit) as first in the pipeline can bypass
         * the camera attachment into the pipeline.
         * @param camera Camera object to use input from.
         **/
        void setCamera(osg::Camera* camera);
        
        /**
         * Get camera used for this pipeline. This method returns the camera object
         * specified with setCamera().
        **/
        inline osg::Camera* getCamera() { return mCamera.get(); }

        /**
         * This method do set up the pipeline. The ppus are inserted into the pipeline
         * according to their indices. Input and outputs are setted up according 
         * to the order of the ppus (if not specified explicitely).
         * Calling of this method will always clean up the pipeline first.
         * @param pipeline List of ppus to add into the pipeline.
        **/
        void setPipeline(const Pipeline& pipeline);
        
        //! Get currently used pipeline 
        const Pipeline& getPipeline() const { return mPipeline; }
 
        /**
         * Remove a ppu from the pipeline. An offline ppu is just removed.  
         * If an online ppu is removed then the input of the removed ppu 
         * is setted up as input to the next ppu in the pipeline.
         * @param ppuName Unique name of the ppu in the pipeline.
         * @return Iterator of the pipeline list which can be used to resetup the list manually.
        **/
        Pipeline::iterator removePPUFromPipeline(const std::string& ppuName);
    
        /**
         * Add new ppu to the pipeline. The ppu will be sorted in to the pipeline according
         * to its index. So call Unit::setIndex() before calling this method.
         * Inputs and outputs are setted up acoordingly. A ppu will not be initialized,
         * hence do this after you have added it into a pipeline.
         * @param ppu Pointer to the ppu 
        **/
        void addPPUToPipeline(Unit* ppu);
        
        /**
         * Get stateset of the post processor. This is the working stateset. You can 
         * change its content to change default behaviour of the pipeline. However 
         * it is not recomended.
        **/
        osg::StateSet* getOrCreateStateSet();

        /**
         * Set current time. Use this to setup reference time. 
         * A time is needed to be able to animate blending of ppus.
         * @param t Current time in seconds
        **/
        inline void setTime(float t) { mTime = t; }

        /**
         * Get a ppu.
         * @param name Unique name of the ppu in the pipeline
        **/
        Unit* getPPU(const std::string& name);

        /**
         * Setup a post draw callback to update post processor.
         * @param camera Camera which post draw callback will be used to update the 
         * post processor. This can either be the same camera as specified in 
         * setCamera() method or a different one.
        **/
        void initPostDrawCallback(osg::Camera* camera);
        
        /**
         * Utility function to derive source texture format from the internal format.
         * For example GL_RGB16F_ARB corresponds to GL_FLOAT
        **/
        static GLenum createSourceTextureFormat(GLenum internalFormat);

        /**
         * Return current state assigned with the post process.
        **/
        inline osg::State* getState() { return mState.get(); }

    protected:

        //! Empty constructor is defined as protected to prevent of creating non-valid post processors
        Processor() {printf("osgPPU::Processor::Processor() - How get there?\n");}
        
        /**
         * Callback function for derived classes, which is called before ppu is applied.
         * @param ppu Pointer to the ppu, which was applied 
         * @return The caller should return true if to apply the ppu or false if not
         **/
        virtual bool onPPUApply(Unit* ppu) {return true;}

        /**
        * Callback function which will be called, if ppu is initialized
        * @return The caller should return true if to continue to initialize the ppu or false if not
        **/
        virtual bool onPPUInit(Unit* ppu) {return true;};


        struct Callback : osg::Camera::DrawCallback
        {
            Callback(Processor* parent) : mParent(parent) {}
            
            void operator () (const osg::Camera&) const
            {
                mParent->update();
            }

            Processor* mParent;
        };


        osg::ref_ptr<osg::State>    mState;
        osg::ref_ptr<osg::StateSet> mStateSet;
        osg::ref_ptr<osg::Camera> mCamera;
        Pipeline  mPipeline;
        float mTime;

};


};

#endif
