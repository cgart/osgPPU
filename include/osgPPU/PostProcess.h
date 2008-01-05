/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _C_POST_PROCESS__H_
#define _C_POST_PROCESS__H_


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgPPU/PostProcessUnit.h>
#include <osg/Camera>

namespace osgPPU
{

/**
 * PostProcess should be called as a last step in your rendering pipeline.
 * This class manages the post processing units which do form a graph.
 * To the PostProcess object a camera is attached. The camera's subgraph
 * should contain the scene to apply the post processing on. Also
 * the camera must provide a valid viewport and color attachment (texture).
 * The update of the post processing is done by using post draw callback
 * of the attached camera, hence you have to take care before using
 * your own post draw callbacks on the attached camera.
 **/
class PostProcess : public osg::Object {
    public: 
    
        META_Object(osgPPU, PostProcess)
        typedef std::list<osg::ref_ptr<PostProcessUnit> > FXPipeline;
        
        //! Initialize the postprocessing system
        PostProcess();
    
        //! Copy constructor
        PostProcess(const PostProcess&, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
    
        //! Release the system
        virtual ~PostProcess();
        
        //! This should be called every frame to update the system
        virtual void update(float dTime = 0.0f);
        
        //! Add the camera object
        void setCamera(osg::Camera* camera);
        
        //! Get camera
        inline osg::Camera* getCamera() { return mCamera.get(); }

        //! Update the pipeline by retrieving all postprocessing effects and recombine them into the pipeline
        void setPipeline(const FXPipeline& pipeline);
        
        //! Setup osg state which can be used as global one
        void setState(osg::State* state);
    
        //! Remove one ppu from the pipeling
        FXPipeline::iterator removePPUFromPipeline(const std::string& ppuName);
    
        //! Add new ppu to the pipeline
        void addPPUToPipeline(PostProcessUnit* ppu);
        
        //! Get state of the post processing class 
        inline osg::State* getState() { return mState.get(); }

        //! Get stateset of this object
        osg::StateSet* getOrCreateStateSet();

        //! Set current time. Use this to setup reference time
        inline void setTime(float t) { mTime = t; }

        //! This is the callback class to update the post process
        struct Callback : osg::Camera::DrawCallback
        {
            Callback(PostProcess* parent) : mParent(parent) {}
            
            void operator () (const osg::Camera&) const
            {
                mParent->update();
            }

            PostProcess* mParent;
        };

    protected:
        
        //! Store here global state for all ppus
        osg::ref_ptr<osg::State>    mState;
        
        //! State set for global state for all ppus 
        osg::ref_ptr<osg::StateSet> mStateSet;
        
        //! camera
        osg::ref_ptr<osg::Camera> mCamera;
        
        //! FX pipeline used for post processing
        FXPipeline  mFXPipeline;

        //! here we store our current time 
        float mTime;

        /**
         * Callback function for derived classes, which if ppu was applied.
         * @param ppu Pointer to the ppu, which was applied 
         * @return The caller should return true if to apply the ppu or false if not
         **/
        virtual bool onPPUApply(PostProcessUnit* ppu) {return true;}

        /**
        * Callback function which will be called, if ppu is initialized
        * @return The caller should return true if to continue to initialize the ppu or false if not
        **/
        virtual bool onPPUInit(PostProcessUnit* ppu) {return true;};

};

};

#endif
