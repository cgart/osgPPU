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

#include <osgPPU/Unit.h>
#include <osgPPU/UnitInOut.h>
#include <osgPPU/Processor.h>
#include <osgPPU/Visitor.h>
#include <osgPPU/BarrierNode.h>
#include <osgPPU/Utility.h>

#include <osg/Texture2D>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osg/Image>
#include <osg/Program>
#include <osg/FrameBufferObject>
#include <osg/Geometry>
#include <math.h>

namespace osgPPU
{

//------------------------------------------------------------------------------
Unit::Unit() : osg::Group(),
    mbDirty(true),
    mInputTexIndexForViewportReference(0),
    mbActive(true),
    mbUpdateTraversed(false),
    mbCullTraversed(false)
{
    // set default name
    setName("__Nameless_PPU_");

    // create default geode
    mGeode = new osg::Geode();
    mGeode->setCullingActive(false);
    addChild(mGeode.get());

    // initialze projection matrix
    sProjectionMatrix = new osg::RefMatrix(osg::Matrix::ortho(0,1,0,1,0,1));

    // setup default modelview matrix
    sModelviewMatrix = new osg::RefMatrix(osg::Matrixf::identity());

    // setup default empty fbo and empty program, so that in default mode
    // we do not use any fbo or program
    getOrCreateStateSet()->setAttribute(new osg::Program(), osg::StateAttribute::ON);
    getOrCreateStateSet()->setAttribute(new osg::FrameBufferObject(), osg::StateAttribute::ON);

    // we also setup empty textures so that this unit do not get any input texture
    // as long as one is not defined
    for (unsigned int i=0; i < 16; i++)
    {
        getOrCreateStateSet()->setTextureAttribute(i, new osg::Texture2D());
    }

    // no culling, because we do not need it
    setCullingActive(false);

    // set default color attribute
    setColorAttribute(new ColorAttribute());
}

//------------------------------------------------------------------------------
Unit::Unit(const Unit& ppu, const osg::CopyOp& copyop) :
    osg::Group(ppu, copyop),
    mInputTex(ppu.mInputTex),
    mOutputTex(ppu.mOutputTex),
    mIgnoreList(ppu.mIgnoreList),
    mInputToUniformMap(ppu.mInputToUniformMap),
    mShader(ppu.mShader),
    mDrawable(ppu.mDrawable),
    sProjectionMatrix(ppu.sProjectionMatrix),
    sModelviewMatrix(ppu.sModelviewMatrix),
    mViewport(ppu.mViewport),
    mGeode(ppu.mGeode),
    mColorAttribute(ppu.mColorAttribute),
    mbDirty(ppu.mbDirty),
    mInputTexIndexForViewportReference(ppu.mInputTexIndexForViewportReference),
    mbActive(ppu.mbActive),
    mbUpdateTraversed(ppu.mbUpdateTraversed),
    mbCullTraversed(ppu.mbCullTraversed)
{

}

//------------------------------------------------------------------------------
Unit::~Unit()
{
}

//------------------------------------------------------------------------------
void Unit::setColorAttribute(ColorAttribute* ca)
{
    // remove the old one
    if (mColorAttribute.valid())
        getOrCreateStateSet()->removeAttribute(mColorAttribute.get());

    // set new color attribute
    mColorAttribute = ca;
    getOrCreateStateSet()->setAttribute(ca, osg::StateAttribute::ON);
}

//------------------------------------------------------------------------------
osg::Drawable* Unit::createTexturedQuadDrawable(const osg::Vec3& corner,const osg::Vec3& widthVec,const osg::Vec3& heightVec, float l, float b, float r, float t)
{
    osg::Geometry* geom = osg::createTexturedQuadGeometry(corner, widthVec, heightVec, l,b,r,t);

    // setup default state set
    geom->setStateSet(new osg::StateSet());
    geom->setUseDisplayList(false);
    //geom->setDataVariance(osg::Object::DYNAMIC);
    //geom->getOrCreateStateSet()->setDataVariance(osg::Object::DYNAMIC);
    geom->setDrawCallback(new Unit::DrawCallback(this));

    // remove colors from the geometry
    geom->setColorBinding(osg::Geometry::BIND_OFF);

    return geom;
}


//------------------------------------------------------------------------------
void Unit::setRenderingFrustum(float left, float top, float right, float bottom)
{
    sProjectionMatrix = new osg::RefMatrix(osg::Matrix::ortho2D(left, right, bottom, top));
}

//------------------------------------------------------------------------------
void Unit::setInputTextureIndexForViewportReference(int index)
{
    if (index != mInputTexIndexForViewportReference)
    {
        if (index < 0)
            mInputTexIndexForViewportReference = -1;
        else
            mInputTexIndexForViewportReference = index;

        dirty();
    }
}

//------------------------------------------------------------------------------
bool Unit::setInputToUniform(Unit* parent, const std::string& uniform, bool add)
{
    if (parent == NULL || uniform.length() < 1) return false;

    // add this unit as a child of the parent if required
    if (add && !parent->containsNode(this)) parent->addChild(this);

    // check if this is a valid parent of this node
    unsigned int index = getNumParents();
    for (unsigned int i=0; i < getNumParents(); i++)
        if (getParent(i) == parent)
        {
            index = i;
            break;
        }

    if (index == getNumParents()) return false;

    // add the uniform
    mInputToUniformMap[parent] = std::pair<std::string, unsigned int>(uniform, index);

    dirty();

    return true;
}

//--------------------------------------------------------------------------
bool Unit::setInputToUniform(int index, const std::string& uniform)
{
    osg::notify(osg::FATAL) << "osgPPU::Unit::setInputToUniform(int,string) - currently not implemented" << std::endl;
    return false;
}

//--------------------------------------------------------------------------
void Unit::removeInputToUniform(const std::string& uniform, bool del)
{
    // search for this uniform
    InputToUniformMap::iterator it = mInputToUniformMap.begin();
    for (; it != mInputToUniformMap.end(); it++)
    {
        if (it->second.first == uniform)
        {
            // remove from the stateset
            mGeode->getOrCreateStateSet()->removeUniform(uniform);

            // if we have to remove the parent
            if (del) it->first->removeChild(this);

            // and finally remove the element from the list
            mInputToUniformMap.erase(it);

            dirty();
            break;
        }
    }
}

//--------------------------------------------------------------------------
void Unit::removeInputToUniform(Unit* parent, bool del)
{
    // search for this uniform
    InputToUniformMap::iterator it = mInputToUniformMap.begin();
    for (; it != mInputToUniformMap.end(); it++)
        if (it->first.get() == parent)
        {
            removeInputToUniform(it->second.first, del);
            break;
        }
}

//--------------------------------------------------------------------------
void Unit::assignInputTexture()
{
    // here the textures will be applied
    osg::StateSet* ss = getOrCreateStateSet();

    // for all entries
    TextureMap::iterator it = mInputTex.begin();
    for (; it != mInputTex.end(); it++)
    {
        // set texture if it is valid
        if (it->second.valid())
        {
            ss->setTextureAttributeAndModes(it->first, it->second.get(), osg::StateAttribute::ON);
        }
    }
}

//--------------------------------------------------------------------------
void Unit::assignShader()
{
    osg::StateSet* ss = getOrCreateStateSet();

    // set shader if it is valid
    if (mShader.valid())
    {
        // enable shader
        mShader->enable(ss);
        ss->setAttributeAndModes(mShader->getProgram(), osg::StateAttribute::ON);

        // notice about changes in the shader assignment
        noticeAssignShader();
    }
}

//--------------------------------------------------------------------------
void Unit::removeShader()
{
    osg::StateSet* ss = getOrCreateStateSet();

    // set shader if it is valid
    if (mShader.valid())
    {
        mShader->disable(ss);
        noticeRemoveShader();
    }
}

//--------------------------------------------------------------------------
void Unit::setViewport(osg::Viewport* vp)
{
    // if viewport is valid and we have to ignore new settings
    if (vp == NULL) return;

    // otherwise setup new viewport
    mViewport = new osg::Viewport(*vp);
    assignViewport();

    dirty();
}

//--------------------------------------------------------------------------
void Unit::assignViewport()
{
    if (mViewport.valid())
    {
        getOrCreateStateSet()->setAttribute(mViewport.get(), osg::StateAttribute::ON);
    }
}

//--------------------------------------------------------------------------
void Unit::setIgnoreInput(unsigned int index, bool ignore)
{
    bool ignored = getIgnoreInput(index);

    // if want to ignore and not ignored before, hence do it
    if (ignore && !ignored)
    {
        mIgnoreList.push_back(index);
        dirty();
    }

    // if ignored and want to not ignore anymore
    else if (!ignore && ignored)
    {
        // remove the fix mark
        for (IgnoreInputList::iterator it = mIgnoreList.begin(); it != mIgnoreList.end();)
            if (*it == index) it = mIgnoreList.erase(it);
            else it++;

        dirty();
    }
}

//--------------------------------------------------------------------------
bool Unit::getIgnoreInput(unsigned int index) const
{
    for (IgnoreInputList::const_iterator it = mIgnoreList.begin(); it != mIgnoreList.end(); it++)
        if (*it == index) return true;
    return false;
}

//--------------------------------------------------------------------------
void Unit::updateUniforms()
{
    // we do get the stateset of the geode, so that we do not
    // get problems with the shader specified on the unit' stateset
    osg::StateSet* ss = mGeode->getOrCreateStateSet();

    // viewport specific uniforms
    if (mViewport.valid())
    {
        osg::Uniform* w = ss->getOrCreateUniform(OSGPPU_VIEWPORT_WIDTH_UNIFORM, osg::Uniform::FLOAT);
        osg::Uniform* h = ss->getOrCreateUniform(OSGPPU_VIEWPORT_HEIGHT_UNIFORM, osg::Uniform::FLOAT);
        w->set((float)mViewport->width());
        h->set((float)mViewport->height());
    }

    // setup input texture uniforms
    InputToUniformMap::iterator it = mInputToUniformMap.begin();
    for (; it != mInputToUniformMap.end(); it++)
    {
        // only valid inputs
        if (it->first.valid())
        {
            // setup uniform
            osg::Uniform* tex = ss->getOrCreateUniform(it->second.first,
                convertTextureToUniformType(it->first->getOutputTexture(0)));
            tex->set((int)it->second.second);
        }
    }

}

//--------------------------------------------------------------------------
void Unit::update()
{
    if (mbDirty)
    {
        init();
        printDebugInfo(NULL);
        updateUniforms();
        mbDirty = false;
    }

    // if shader is attached, then update it
    if (mShader.valid())
        mShader->update();
}

//------------------------------------------------------------------------------
void Unit::traverse(osg::NodeVisitor& nv)
{
    // check if we have to update it
    if (nv.getVisitorType() == osg::NodeVisitor::UPDATE_VISITOR)
    {
        //printf("UNIT %s UPDATE traverse\n", getName().c_str());
        if (mbUpdateTraversed == false)
        {
            mbUpdateTraversed = true;

            update();
            getStateSet()->runUpdateCallbacks(&nv);

            osg::Group::traverse(nv);
        }

    }else if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
    {
        //printf("UNIT %s CULL traverse\n", getName().c_str());
        if (mbCullTraversed == false)
        {
            mbCullTraversed = true;
            osg::Group::traverse(nv);
        }

    }else
        osg::Group::traverse(nv);
}

//------------------------------------------------------------------------------
void Unit::init()
{
    // collect all inputs from the units above
    setupInputsFromParents();

    // check if we have input reference size
    if (getInputTextureIndexForViewportReference() >= 0 && getInputTexture(getInputTextureIndexForViewportReference()))
    {
        // if no viewport, so create it
        if (!mViewport.valid())
            mViewport = new osg::Viewport(0,0,0,0);

        // change viewport sizes
        mViewport->width() = getInputTexture(getInputTextureIndexForViewportReference())->getTextureWidth();
        mViewport->height() = getInputTexture(getInputTextureIndexForViewportReference())->getTextureHeight();

        // just notice that the viewport size is changed
        noticeChangeViewport();
    }

    // reassign input and shaders
    assignInputTexture();
    assignShader();
    assignViewport();
}

//--------------------------------------------------------------------------
// Helper class for collecting inputs from unit parents
//--------------------------------------------------------------------------
class CollectInputParents : public osg::NodeVisitor
{
public:
    CollectInputParents(Unit* caller) :
         osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_PARENTS),
        _caller(caller)
    {
        _inputUnitsFound = false;
    }

    void apply(osg::Group& node)
    {
        Unit* unit = dynamic_cast<osgPPU::Unit*>(&node);
        Processor* proc = dynamic_cast<osgPPU::Processor*>(&node);

        // check if the traversed node is an unit
        if (unit != NULL && unit != _caller)
        {
            // first force the unit to recompile its outputs
            // the update method should do this if it wasn't done before
            unit->update();

            // get the output texture 0 as input
            _input.push_back(unit->getOrCreateOutputTexture(0));

            _inputUnitsFound = true;

        // if it is a processor, then get the camera attachments as inputs
        }else if (proc != NULL && proc->getCamera())
        {
            // get first color attachment from the camera
            osg::Camera::BufferAttachmentMap& map = proc->getCamera()->getBufferAttachmentMap();
            _input.push_back(map[osg::Camera::COLOR_BUFFER]._texture.get());

        // nothing else, then just traverse
        }else
            traverse(node);
    }

    Unit* _caller;
    std::vector<osg::Texture*> _input;
    bool _inputUnitsFound;
};


//--------------------------------------------------------------------------
void Unit::setupInputsFromParents()
{
    // use a visitor to collect all inputs from parents
    CollectInputParents cp(this);
    this->accept(cp);

    // add each found texture as input to the unit
    bool changedInput = false;
    for (unsigned int i=0, k=0; k < cp._input.size(); k++)
    {
        // add as input texture
        if (!getIgnoreInput(k))
        {
            mInputTex[i++] = cp._input[k];
            changedInput = true;
        }
    }
    if (changedInput) noticeChangeInput();

    // if viewport is not defined and we need viewport from processor, then
    if (getViewport() == NULL &&
        (getInputTextureIndexForViewportReference() < 0
        || getInputTextureIndexForViewportReference() >=0 && !cp._inputUnitsFound))
    {
        // find the processor
        FindProcessorVisitor fp;
        this->accept(fp);

        // check if processor found
        if (fp._processor == NULL)
        {
            osg::notify(osg::FATAL) << "osgPPU::Unit::setupInputsFromParents() - " << getName() <<" - is not able to find the unit processor!" << std::endl;
            return;
        }

        // check if no camera is specified, then we have an error
        if (fp._processor != NULL && !fp._processor->getCamera())
        {
            osg::notify(osg::WARN) << "osgPPU::Unit::setupInputsFromParents() - " << getName() <<" - no camera attached and viewport is not specified!" << std::endl;
            return;
        }

        // get viewport from processor
        osg::ref_ptr<osg::Viewport> vp = new osg::Viewport(*(fp._processor->getCamera()->getViewport()));
        setViewport(vp.get());
    }

    setupBlockedChildren();
}

//--------------------------------------------------------------------------
void Unit::setupBlockedChildren()
{
    // check whenever this unit do contain barrier nodes as childs
    for (unsigned int i=0; i < getNumChildren(); i++)
    {
        // if the child is a barrier node, then connect inputs and outputs properly
        BarrierNode* br = dynamic_cast<BarrierNode*>(getChild(i));
        if (br)
        {
            // if the child is not a unit, then there is a bug
            Unit* child = dynamic_cast<Unit*>(br->getBlockedChild());
            if (!child)
            {
                osg::notify(osg::FATAL) << "osgPPU::Unit::setupInputsFromParents() - " << getName() <<" - non valid barrier child!" << std::endl;
                return;
            }

            // add the texture of the blocked parent to the blocked child
            child->mInputTex[child->getNumParents()] = getOrCreateOutputTexture(0);
            child->dirty();
        }
    }
}

//--------------------------------------------------------------------------
void Unit::dirty()
{
    mbDirty = true;

    // mark each child unit as dirty too
    for (unsigned int i=0; i < getNumChildren(); i++)
    {
        Unit* unit = dynamic_cast<Unit*>(getChild(i));
        if (unit && unit->isDirty() == false) unit->dirty();
    }
}

//--------------------------------------------------------------------------
void Unit::DrawCallback::drawImplementation (osg::RenderInfo& ri, const osg::Drawable* dr) const
{
    // only if parent is valid
    if (_parent->getActive())
    {   //_parent->printDebugInfo(dr);
        // unit should know that we are about to render it
        _parent->noticeBeginRendering(ri, dr);

        // set matricies used for the unit
        ri.getState()->applyProjectionMatrix(_parent->sProjectionMatrix.get());
        ri.getState()->applyModelViewMatrix(_parent->sModelviewMatrix.get());

        // now render the drawable geometry
        dr->drawImplementation(ri);

        // ok rendering is done, unit can do other stuff
        _parent->noticeFinishRendering(ri, dr);
    }
}

//--------------------------------------------------------------------------
void Unit::printDebugInfo(const osg::Drawable* dr)
{
    osg::NotifySeverity level = osg::DEBUG_INFO;
    if (level > osg::getNotifyLevel()) return;

    // debug information
    osg::notify(level) << getName() << " (" << getOrCreateStateSet()->getBinNumber() << ") run in thread " << OpenThreads::Thread::CurrentThread() << std::endl;

    if (getStateSet()->getAttribute(osg::StateAttribute::VIEWPORT) || (dr && dr->getStateSet()->getAttribute(osg::StateAttribute::VIEWPORT)))
    {
        const osg::Viewport* viewport = dynamic_cast<const osg::Viewport*>(getStateSet()->getAttribute(osg::StateAttribute::VIEWPORT));
        if (viewport == NULL) viewport = dynamic_cast<const osg::Viewport*>(dr->getStateSet()->getAttribute(osg::StateAttribute::VIEWPORT));
        osg::notify(level) << std::dec << "\t vp (ref " << (int)getInputTextureIndexForViewportReference() << "): " << (int)(viewport->x()) << " " << (int)(viewport->y()) << " " << (int)(viewport->width()) << " " << (int)(viewport->height()) << std::endl;
    }

    if (getShader() != NULL)
    {
        osg::notify(level) << "\t shader DEPRECATED: " << std::hex << getShader() << std::dec << std::endl;
        osg::StateSet::UniformList::const_iterator jt = getShader()->getUniformList().begin();
        for (; jt != getShader()->getUniformList().end(); jt++)
        {
            float fval = -1.0;
            int ival = -1;
            if (jt->second.first->getType() == osg::Uniform::INT || jt->second.first->getType() == osg::Uniform::SAMPLER_2D)
            {
                jt->second.first->get(ival);
                osg::notify(level) << "\t\t" << jt->first << " : " << ival << std::endl;//, (jt->second.second & osg::StateAttribute::ON) != 0);
            }else if (jt->second.first->getType() == osg::Uniform::FLOAT){
                jt->second.first->get(fval);
                osg::notify(level) << "\t\t" << jt->first << " : " << fval << std::endl;//, (jt->second.second & osg::StateAttribute::ON) != 0);
            }
        }
    }else if (getStateSet()->getAttribute(osg::StateAttribute::PROGRAM) || (dr && dr->getStateSet()->getAttribute(osg::StateAttribute::PROGRAM)))
    {
        const osg::Program* prog = dynamic_cast<const osg::Program*>(getStateSet()->getAttribute(osg::StateAttribute::PROGRAM));
        if (prog == NULL) prog = dynamic_cast<const osg::Program*>(dr->getStateSet()->getAttribute(osg::StateAttribute::PROGRAM));
        osg::notify(level) << "\t program: " << std::hex << prog << std::dec << std::endl;

        // print uniforms of the unit 
        osg::StateSet::UniformList::const_iterator jt = getStateSet()->getUniformList().begin();
        for (; jt != getStateSet()->getUniformList().end(); jt++)
        {
            float fval = -1.0;
            int ival = -1;
            if (jt->second.first->getType() == osg::Uniform::INT || jt->second.first->getType() == osg::Uniform::SAMPLER_2D)
            {
                jt->second.first->get(ival);
                osg::notify(level) << "\t\t" << jt->first << " : " << ival << std::endl;//, (jt->second.second & osg::StateAttribute::ON) != 0);
            }else if (jt->second.first->getType() == osg::Uniform::FLOAT){
                jt->second.first->get(fval);
                osg::notify(level) << "\t\t" << jt->first << " : " << fval << std::endl;//, (jt->second.second & osg::StateAttribute::ON) != 0);
            }
        }

        // print uniforms of the drawable
        if (dr)
        {
            osg::StateSet::UniformList::const_iterator jt = dr->getStateSet()->getUniformList().begin();
            for (; jt != dr->getStateSet()->getUniformList().end(); jt++)
            {
                float fval = -1.0;
                int ival = -1;
                if (jt->second.first->getType() == osg::Uniform::INT || jt->second.first->getType() == osg::Uniform::SAMPLER_2D)
                {
                    jt->second.first->get(ival);
                    osg::notify(level) << "\t\t" << jt->first << " : " << ival << std::endl;//, (jt->second.second & osg::StateAttribute::ON) != 0);
                }else if (jt->second.first->getType() == osg::Uniform::FLOAT){
                    jt->second.first->get(fval);
                    osg::notify(level) << "\t\t" << jt->first << " : " << fval << std::endl;//, (jt->second.second & osg::StateAttribute::ON) != 0);
                }
            }
        }
    }


    osg::notify(level) << "\t input: ";
    for (unsigned int i=0; i < getInputTextureMap().size(); i++)
    {
        osg::Texture* tex = getInputTexture(i);
        osg::notify(level) << " " << i << ":" << std::hex << tex << std::dec;
        if (tex)
        {
            if (getStateSet()->getTextureAttribute(i, osg::StateAttribute::TEXTURE))
                osg::notify(level) << "-attr";
            osg::notify(level) << " (" << tex->getTextureWidth() << "x" << tex->getTextureHeight() << ")";
        }
    }

    UnitInOut* inout = dynamic_cast<UnitInOut*>(this);
    if (inout)
    {
        osg::notify(level) << std::endl << "\t output: ";
        for (unsigned int i=0; i < inout->getOutputTextureMap().size(); i++)
        {
            osg::Texture* tex = inout->getOutputTexture(i);
            osg::notify(level) << " " << std::hex << tex << std::dec << " ";
            if (tex)
            {
                osg::notify(level) << "(" << tex->getTextureWidth() << "x" << tex->getTextureHeight() << " ";
                if (inout->getOutputZSliceMap().size() > 1)
                {
                    UnitInOut::OutputSliceMap::const_iterator it = inout->getOutputZSliceMap().begin();
                    osg::notify(level) << "{";
                    for (; it != inout->getOutputZSliceMap().end(); it++)
                        osg::notify(level) << it->first << "->" << it->second << ",";
                    osg::notify(level) << "}";
                }
                osg::notify(level) << ")";
            }
        }

        const osg::FrameBufferObject* fbo = dynamic_cast<const osg::FrameBufferObject*>(getStateSet()->getAttribute((osg::StateAttribute::Type) 0x101010));
        if (fbo == NULL && dr != NULL) fbo = dynamic_cast<const osg::FrameBufferObject*>(dr->getStateSet()->getAttribute((osg::StateAttribute::Type) 0x101010));
        osg::notify(level) << std::endl << "\t fbo: " << std::hex << fbo << std::dec << std::endl;        
    }

    osg::notify(level) << std::endl;
}

}; // end namespace

