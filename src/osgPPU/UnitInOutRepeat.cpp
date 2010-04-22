/***************************************************************************
 *   Copyright (c) 2010   Art Tevs                                         *
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

#include <osgPPU/UnitInOutRepeat.h>
#include <osgPPU/Processor.h>
#include <osgPPU/BarrierNode.h>
#include <osgPPU/Visitor.h>
#include <osgPPU/UnitInOut.h>

namespace osgPPU
{
    //------------------------------------------------------------------------------
    // Notify callback, which will enable specified texture on a certain iteration.
    //------------------------------------------------------------------------------
    struct ChangeInputsCallback : public Unit::NotifyCallback
    {
        ChangeInputsCallback(osg::Texture* texture, unsigned unit, unsigned changeOn, unsigned numIterations) : 
            _iteration(0),
            _unit(unit),
            _changeOnIteration(changeOn),
            _numIterations(numIterations),
            _texture(texture)
        {}

        void operator()(osg::RenderInfo& ri, const Unit* unit) const
        {
            if (_iteration >= _changeOnIteration)
            {
                //glAccum(GL_ADD, 1.0);
                ri.getState()->applyTextureAttribute(_unit, _texture);
            }
            _iteration = (_iteration + 1) % _numIterations;
        }

        mutable unsigned _iteration;
        unsigned _unit;
        unsigned _changeOnIteration;
        unsigned _numIterations;
        osg::ref_ptr<osg::Texture> _texture;
    };

    //------------------------------------------------------------------------------
    UnitInOutRepeat::UnitInOutRepeat() : UnitInOut()
    {
        _numIterations = 0;
        _lastNode = NULL;
        _lastNodeOutputIndex = 0;
    }

    //------------------------------------------------------------------------------
    UnitInOutRepeat::UnitInOutRepeat(const UnitInOutRepeat& u, const osg::CopyOp& copyop) : 
        UnitInOut(u, copyop),
        _numIterations(u._numIterations),
        _lastNodeOutputIndex(u._lastNodeOutputIndex),
        _lastNode(u._lastNode)
    {

    }
    
    //------------------------------------------------------------------------------
    UnitInOutRepeat::~UnitInOutRepeat()
    {

    }
    
    //------------------------------------------------------------------------------
    void UnitInOutRepeat::setLastNode(Unit* node)
    {
        _lastNode = node;
        dirty();        
    }

    //------------------------------------------------------------------------------
    void UnitInOutRepeat::traverse(osg::NodeVisitor& nv)
    {
        // the repeatable traversion is only interesting for cull visitors
        if (nv.getVisitorType() != osg::NodeVisitor::CULL_VISITOR || _lastNode == NULL || _numIterations <= 1)
        {
            UnitInOut::traverse(nv);
            return;
        }

        // disable access to all child units
		std::vector<unsigned> nodeMasks(_lastNode->getNumChildren(), 0);
        for (unsigned i=0; i < _lastNode->getNumChildren(); i++)
        {
            Unit* unit = dynamic_cast<Unit*>(_lastNode->getChild(i));
            if (unit)
			{
				nodeMasks[i] = unit->getNodeMask();
				unit->setNodeMask(0x0);
			}
        }

        // for every iteration we do
        for (int i=0; i < _numIterations; i++)
        {
            // mark every unit as not being culled before
            CleanCullTraversedVisitor::sVisitor->run(this);

            // run cull visitor which will stop after the last unit
            UnitInOut::traverse(nv);
        }

        // continue traversing after last unit (start traversion on every child unit)
        for (unsigned i=0; i < _lastNode->getNumChildren(); i++)
        {
            Unit* unit = dynamic_cast<Unit*>(_lastNode->getChild(i));
            if (unit)
            {
                unit->setNodeMask(nodeMasks[i]);
                unit->accept(nv);
            }
        }
    }

    //------------------------------------------------------------------------------
    void UnitInOutRepeat::init()
    {
        // currently only one input per repeat unit is supported, hence force it
        if (getNumParents() > 1)
        {
            osg::notify(osg::FATAL) << "osgPPU::UnitBypassRepeat - does support only 1 unit as parent (you specified " << getNumParents() << " ). Behaviour is undefined" << std::endl;
            return;
        }

        UnitInOut::init();

        if (_lastNode == NULL || _numIterations <= 1) return;

        // warning that we use only one output from the last unit
        if (_lastNode->getOutputTextureMap().size() > _lastNodeOutputIndex + 1)
        {
            osg::notify(osg::WARN) << "osgPPU::UnitBypassRepeat - last node has " << _lastNode->getOutputTextureMap().size() << " outputs, however you specified to use " << _lastNodeOutputIndex << " output. Output 0 will be forced!" << std::endl;
            _lastNodeOutputIndex = 0;
        }

		// setup a change in the input on my self, so get on the second iteration the output of the last unit
		setBeginDrawCallback(new ChangeInputsCallback(_lastNode->getOrCreateOutputTexture(_lastNodeOutputIndex), 0, 1, _numIterations));

        // input of each child should be changed on the second iteration
        /*for (unsigned i=0; i < getNumChildren(); i++)
        {
            Unit* unit = dynamic_cast<Unit*>(getChild(i));
            if (unit)
            {
                // determine which input from this unit goes to its children and add a callback there
                for (unsigned j=0, index=0; j < unit->getNumParents(); j++)
                {
                    if (unit->getParent(j) == this)
                        unit->setBeginDrawCallback(new ChangeInputsCallback(_lastNode->getOrCreateOutputTexture(_lastNodeOutputIndex), index, 1, _numIterations));

                    // if parent is a io unit, then skip also MRTs
                    UnitInOut* unitIO = dynamic_cast<UnitInOut*>(unit->getParent(j));
                    if (unitIO) index += unitIO->getOutputDepth();
                    else index ++;
                }
            }
        }*/
    }


}; // end namespace
