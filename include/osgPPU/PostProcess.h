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

/**
 * PostProcess should be called as a last step in your rendering pipeline.
 * This class manages the post processing units which do form a graph.
 **/
class PostProcess : public osg::Object {
    public: 
    
        //! Meta information
        META_Object(osgPPU, PostProcess)
    
        //! Initialize the postprocessing system
        PostProcess();
    
        //! Copy constructor
        PostProcess(const PostProcess&, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
    
        //! Release the system
        virtual ~PostProcess();
        
        //! This should be called every frame to update the system
        virtual void update();
        
        //! Add the camera object
        void setCamera(osg::Camera* camera);
        
        //! List of multipass post rendering shaders
        typedef std::list<osg::ref_ptr<PostProcessUnit> > FXPipeline;
    
        //! Update the pipeline by retrieving all postprocessing effects and recombine them into the pipeline
        void setPipeline(const FXPipeline& pipeline);
        
        //! Setup osg state which can be used as global one
        void setState(osg::State* state);
    
        //! Remove one ppu from the pipeling
        FXPipeline::iterator removePPUFromPipeline(const std::string& ppuName);
    
        //! Add new ppu to the pipeline
        void addPPUToPipeline(PostProcessUnit* ppu);
        
    private:
        
        //! Store here global state for all ppus
        osg::ref_ptr<osg::State>    mState;
        
        //! State set for global state for all ppus 
        osg::ref_ptr<osg::StateSet> mStateSet;
        
        //! camera
        osg::ref_ptr<osg::Camera> mCamera;
        
        //! FX pipeline used for post processing
        FXPipeline  mFXPipeline;
};

#endif
