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

#include <osgPPU/Visitor.h>
#include <osgPPU/UnitBypass.h>
#include <osgPPU/BarrierNode.h>
#include <osgUtil/CullVisitor>

#include <OpenThreads/ScopedLock>
#include <OpenThreads/Mutex>

// Mutex used to let threads only change data values of Units in serialized manner
static OpenThreads::Mutex    s_mutex_changeUnitSubgraph;

namespace osgPPU
{

//------------------------------------------------------------------------------
void CleanUpdateTraversedVisitor::run (osg::Group* root)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
    root->traverse(*this);
}

//------------------------------------------------------------------------------
void CleanCullTraversedVisitor::run (osg::Group* root)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
    root->traverse(*this);
}

//------------------------------------------------------------------------------
void RemoveUnitVisitor::run (osg::Group* root)
{
    // remove operation must be atomic
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(s_mutex_changeUnitSubgraph);

    // the specified root should be the unit which we would like to remove
    Unit* unit = dynamic_cast<Unit*>(root);
    if (unit == NULL)
    {
        osg::notify(osg::FATAL) << "osgPPU::RemoveUnitVisitor::run() - not a valid unit was specified" << std::endl;
        return;
    }

    // set all parent units as parents for the own children
    for (unsigned int i=0; i < unit->getNumParents(); i++)
        for (unsigned int j=0; j < unit->getNumChildren(); j++)
        {
            // copy the childonly if it is another unit
            // TODO: some better removing strategies are required
            if (dynamic_cast<Unit*>(unit->getChild(j)) != NULL)
            {
                if (!unit->getParent(i)->containsNode(unit->getChild(j)))
                    unit->getParent(i)->addChild(unit->getChild(j));
            }
        }

    // mark the unit as dirty, which will force to mark every child unit also as dirty
    unit->dirty();
    
    // remove all children
    unit->removeChildren(0, unit->getNumChildren());

    // remove unit from its parents
    osg::Node::ParentList parents = unit->getParents();
    for (unsigned int i=0; i < parents.size(); i++)
    {
        parents[i]->removeChild(unit);
    }
}

//------------------------------------------------------------------------------
void SetMaximumInputsVisitor::apply (osg::Group &node)
{
    Unit* unit = dynamic_cast<Unit*>(&node);
    if (unit)
    {
        osg::StateSet* ss = unit->getOrCreateStateSet();

        // remove all unneccessary textures
        for (unsigned int i=mMaxUnitInputIndex+1; i < ss->getTextureAttributeList().size(); i++)
        {
            ss->removeTextureAttribute(i, osg::StateAttribute::TEXTURE);
        }

    }
    node.traverse(*this);
}

//------------------------------------------------------------------------------
void SetMaximumInputsVisitor::run (osg::Group* root)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(s_mutex_changeUnitSubgraph);

    root->traverse(*this);
}

//------------------------------------------------------------------------------
void OptimizeUnitsVisitor::apply (osg::Group &node)
{
    Unit* unit = dynamic_cast<Unit*>(&node);

    // if it was a unit, set the maximal input count
    if (unit)
    {
        const Unit::TextureMap& tmap = unit->getInputTextureMap();

        for (Unit::TextureMap::const_iterator it = tmap.begin(); it != tmap.end(); it++)
        {
            if (it->first > (int)_maxUnitInputIndex)
                _maxUnitInputIndex = it->first;
        }
    }

    // traverse the node
    node.traverse(*this);
}

//------------------------------------------------------------------------------
void OptimizeUnitsVisitor::run (osg::Group* root)
{
    // first detect maximal number of input textures
    _maxUnitInputIndex = 0;
    root->traverse(*this);

    // set the unit's input texture attributes accordingly
    {
        // the opration change some unit values, hence make it atomic
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(s_mutex_changeUnitSubgraph);

        osg::notify(osg::INFO) << "osgPPU::OptimizeUnitsVisitor::run() - maximum possible texture index is " << _maxUnitInputIndex << "." << std::endl;
        SetMaximumInputsVisitor sm(_maxUnitInputIndex);
        root->traverse(sm);
    }
}

//------------------------------------------------------------------------------
void ResolveUnitsCyclesVisitor::apply (osg::Group &node)
{
    bool foundCycle = false;

    // traverse the node path and check, whenever this node was already added
    osg::NodePath::reverse_iterator it = getNodePath().rbegin(); it++;
    for (; it!=getNodePath().rend(); it++)
    {
        // if we are able to find this node again, so there is a cycle
        if (*it == dynamic_cast<osg::Node*>(&node))
        {
            // create new node, which will be used to block the traversion
            BarrierNode* br = new BarrierNode();
            foundCycle = true;

            // the previous node in the list, shouldn't contain this node as a child anymore
            it --;
            (dynamic_cast<osg::Group*>(*it))->replaceChild(dynamic_cast<osg::Node*>(&node), br);

            // debug info
            osg::notify(osg::INFO) << "osgPPU::ResolveUnitsCyclesVisitor::apply():" << std::endl << "\t";
            for (osg::NodePath::iterator kt = getNodePath().begin(); kt!=getNodePath().end(); kt++) osg::notify(osg::INFO) << (*kt)->getName() << " -> ";
            osg::notify(osg::INFO) << std::endl << "\tReplace child " << node.getName() << " of node " << (*it)->getName() << " with a barrier node to resolve cycles!" <<std::endl;

            // now the child of the barrier node would be the current node
            br->setBlockedChild(&node);
            br->setBlockedParent(dynamic_cast<osg::Group*>(*it));
            br->setName((*it)->getName() + std::string("-") + node.getName());

            it ++;
        }
    }

    // traverse the unit as if it where a group node
    if (!foundCycle)
        node.traverse(*this);

}

//------------------------------------------------------------------------------
void ResolveUnitsCyclesVisitor::run (osg::Group* root)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(s_mutex_changeUnitSubgraph);

    root->traverse(*this);
}


//------------------------------------------------------------------------------
void SetupUnitRenderingVisitor::run (osg::Group* root)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(s_mutex_changeUnitSubgraph);

    // setup unit list
    mUnitSet.clear();
    root->traverse(*this);

    // setup the indices of the units, so that they got sorted correctly in the pipeline
    unsigned int index = _proc->getOrCreateStateSet()->getBinNumber();
    const std::string& binName = _proc->getOrCreateStateSet()->getBinName();

    // the unit set do contain units in the traversed order
    for (UnitSet::iterator it = mUnitSet.begin(); it != mUnitSet.end(); it++)
    {
        // sort the unit into the according pipeline
        //(*it)->getOrCreateStateSet()->setRenderBinToInherit();
        (*it)->getOrCreateStateSet()->setRenderBinDetails(index, binName);
        //printf("INIT %s %d\n", (*it)->getName().c_str(), index);

        // initialize units by updating them (the initialization process is called automagically
        _proc->onUnitInit(*it);
        (*it)->update();
    }

}


//------------------------------------------------------------------------------
void SetupUnitRenderingVisitor::apply (osg::Group &node)
{
    Unit* unit = dynamic_cast<Unit*>(&node);

    // we do here a manuall children accept method calling.
    // this is required to call children in reverse order
    // TODO: This behaviour don't call Unit::traverse() method
    //  in future releases it could be a problem, hence looking
    //  for better solutions
    for (int i= (int)node.getNumChildren()-1; i>=0; i--)
    {
        node.getChild(i)->accept(*this);
    }

    // traverse the unit as if it where a group node
    //node.traverse(*this);

    // if the given node is a Unit
    if (unit != NULL)
    {
        // add the new unit only if it wasn't added before
        bool found = false;
        for (UnitSet::iterator it=mUnitSet.begin(); it!=mUnitSet.end(); it++)
            if (*it == unit) found = true;

        // add the unit to the unit set
        if (found == false)
            mUnitSet.push_front(unit);
    }
}


}; //end namespace




