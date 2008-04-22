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

#include <osgPPU/Processor.h>
#include <osgPPU/Visitor.h>
#include <osg/Texture2D>
#include <osg/Depth>
#include <osg/Notify>
#include <osg/ClampColor>

#include <assert.h>


namespace osgPPU
{

unsigned int Processor::_lastGivenID = 0;

//------------------------------------------------------------------------------
Processor::Processor()
{
    // set some variables
    mbDirty = true;
    mbDirtyUnitGraph = true;
    _lastGivenID ++;
    mID = _lastGivenID;

    // first we have to create a render bin which will hold the units
    // of the subgraph.
    char binName[128];
    sprintf(binName, "osgPPU_Pipeline[%d]", mID);
    mPipeline = new Pipeline();
    mPipeline->setName(binName);
    osgUtil::RenderBin::addRenderBinPrototype(binName, mPipeline.get());

    // create an instance of the visitor for the unit subgraph
    mVisitor = new Visitor(this);

    // no culling
    setCullingActive(false);
}

//------------------------------------------------------------------------------
Processor::Processor(const Processor& pp, const osg::CopyOp& copyop) :
    osg::Group(pp, copyop),
    mCamera(pp.mCamera),
    mPipeline(pp.mPipeline),
    mVisitor(pp.mVisitor),
    mbDirty(pp.mbDirty),
    mbDirtyUnitGraph(pp.mbDirtyUnitGraph),
    mID(pp.mID)
{
}

//------------------------------------------------------------------------------
Processor::~Processor()
{
    // remove render bin prototype
    osgUtil::RenderBin::removeRenderBinPrototype(mPipeline.get());
}

//------------------------------------------------------------------------------
void Processor::init()
{
    // create default state for post processing effects
    osg::StateSet* mStateSet = getOrCreateStateSet();
    mStateSet->clear();

    // the processor's stateset have to be activated as first in the pipeline
    mStateSet->setRenderBinDetails(0, getPipelineName());
    
    // setup default state set 
    mStateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
    mStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
    mStateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
    mStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);

    // we should not write to the depth buffer 
    osg::Depth* ds = new osg::Depth();
    ds->setWriteMask(false);
    mStateSet->setAttribute(ds, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    // the processor is of fixed function pipeline, however the childs (units) might not 
    mStateSet->setAttribute(new osg::Program());

    // disable color clamping, because we want to work on real hdr values
    osg::ClampColor* clamp = new osg::ClampColor();
    clamp->setClampVertexColor(GL_FALSE);
    clamp->setClampFragmentColor(GL_FALSE);
    clamp->setClampReadColor(GL_FALSE);

    // make it protected and override, so that it is done for the whole rendering pipeline
    mStateSet->setAttribute(clamp, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    // not dirty anymore
    mbDirty = false;

    // add as drawable
    setNumChildrenRequiringUpdateTraversal(1);
}


//------------------------------------------------------------------------------
void Processor::setCamera(osg::Camera* camera)
{
    // dirty subgraph if camera changed
    if (mCamera.get() != camera)
        dirtyUnitSubgraph();

    // setup camera
    mCamera = camera;
}

//------------------------------------------------------------------------------
Unit* Processor::findUnit(const std::string& name)
{
    if (!mbDirtyUnitGraph)
        return mVisitor->findUnit(name, this);
    return NULL;
}

//------------------------------------------------------------------------------
void Processor::traverse(osg::NodeVisitor& nv)
{
    // if not initialized before, then do it
    if (mbDirty) init();

    // if we are about to do the cull traversal, then
    if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR && mVisitor->getMode() == Visitor::NONE)
    {
        // setup frame stamp
        mVisitor->setFrameStamp(const_cast<osg::FrameStamp*>(nv.getFrameStamp()));
        mVisitor->setCullVisitor(&nv);

        if (mbDirtyUnitGraph)
        {
            mbDirtyUnitGraph = false;

            // we have first to optimize the unit graph, so that it
            // removes all the problematic and unused units
            mVisitor->perform(Visitor::RESOLVE_CYCLES, this);

            // debug information
            osg::notify(osg::INFO) << "--------------------------------------------------------------------" << std::endl;
            osg::notify(osg::INFO) << "BEGIN " << mPipeline->getName() << std::endl;        
    
            // use the osgppu's default visitor to init the subgraph
            mVisitor->perform(Visitor::INIT_UNIT_GRAPH, this);
            
            osg::notify(osg::INFO) << "END " << mPipeline->getName() << std::endl;
            osg::notify(osg::INFO) << "--------------------------------------------------------------------" << std::endl;
    
            // optimize subgraph
            mVisitor->perform(Visitor::OPTIMIZE, this);
        }

        // perform updating traversion
        mVisitor->perform(Visitor::UPDATE, this);
    
        // perform cull traversal
        mVisitor->perform(Visitor::CULL, this);

    // perform traversing only if the graph is valid
    }else if (mbDirtyUnitGraph == false)
        osg::Group::traverse(nv);
}


}; //end namespace




