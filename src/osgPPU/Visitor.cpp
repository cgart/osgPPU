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

namespace osgPPU
{

//------------------------------------------------------------------------------
Visitor::Visitor(Processor* proc) : osg::NodeVisitor()
{
    setTraversalMode(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN);
    setVisitorType(osg::NodeVisitor::NODE_VISITOR);

    mbValidSubgraph = true;
    mProcessor = proc;
    mMode = NONE;
    mUnitToFind = NULL;
    mCullVisitor = NULL;
    mMaxUnitInputIndex = 0;
    mCleanTraversedMaskVisitor = new CleanTraverseMaskVisitor();
}

//------------------------------------------------------------------------------
Visitor::~Visitor()
{
}


//------------------------------------------------------------------------------
void Visitor::apply (osg::Group &node)
{
    if (mMode == NONE) return;

    Unit* unit = dynamic_cast<Unit*>(&node);

    // if we want to optimize the graph
    if (mMode == OPTIMIZE)
    {
        // if it was a unit, set the maximal input count
        if (unit)
        {
            const Unit::TextureMap& tmap = unit->getInputTextureMap();

            for (Unit::TextureMap::const_iterator it = tmap.begin(); it != tmap.end(); it++)
            {
                if (it->first > (int)mMaxUnitInputIndex)
                    mMaxUnitInputIndex = it->first;
            }
        }

        // traverse the node 
        node.traverse(*this);

    // if we have to resolve the cycles
    } else if (mMode == RESOLVE_CYCLES)
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
                osg::notify(osg::INFO) << "osgPPU::Visitor::apply(RESOLVE_CYCLES):" << std::endl << "\t";
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

    // we have to initialize the unit graph
    }else if (mMode == INIT_UNIT_GRAPH)
    {
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

    // dirty the complete graph
    }else if (mMode == DIRTY_ALL_UNITS)
    {        
        // convert to unit
        if (unit != NULL) unit->dirty();

        // traverse the unit as if it where a group node
        node.traverse(*this);

    // if we are looking for a unit
    }else if (mMode == FIND_UNIT)
    {
        if (unit != NULL && unit->getName() == mUnitToFindName)
        {
            mUnitToFind = unit;
            return;
        }

        // traverse the unit as if it where a group node
        node.traverse(*this);

    // if we are looking for a unit to remove it
    }else if (mMode == REMOVE_UNIT)
    {
        if (unit != NULL && unit == mUnitToRemove)
        {
            // set all parent units as parents for the own children
            for (unsigned int i=0; i < unit->getNumParents(); i++)
                for (unsigned int j=0; j < unit->getNumChildren(); j++)
                {
                    if (!unit->getParent(i)->containsNode(unit->getChild(j)))
                        unit->getParent(i)->addChild(unit->getChild(j));
                }

            // mark each child as dirty
            for (unsigned int j=0; j < unit->getNumChildren(); j++)
            {
                if (dynamic_cast<Unit*>(unit->getChild(j)))
                    dynamic_cast<Unit*>(unit->getChild(j))->dirty();
            }

            // remove all children
            unit->removeChildren(0, unit->getNumChildren());

            // remove unit from its parents
            osg::Node::ParentList parents = unit->getParents();
            for (unsigned int i=0; i < parents.size(); i++)
            {
                parents[i]->removeChild(unit);
            }
            return;
        }

        // traverse the unit as if it where a group node
        node.traverse(*this);

    // if we want to update the unit graph
    }else if (mMode == UPDATE)
    {   
        // depth-first-traverse
        node.traverse(*this);

        // update if it was a unit
        if (unit != NULL)
        {
            // iterate through parent list and mark the children as dirty, if parent is dirty
            bool dirty = false;
            for (osg::NodePath::iterator it = getNodePath().begin(); it != getNodePath().end(); it++)
            {
                Unit* u = dynamic_cast<Unit*>(*it);
                if (u && dirty) u->dirty();
                if (u && u->isDirty()) dirty = true;
            }

            // update the unit
            mProcessor->onUnitUpdate(unit);
            unit->update();
        }
    }
}

//------------------------------------------------------------------------------
void Visitor::perform(Mode mode, osg::Group* n)
{
    mMode = mode;

    switch(mMode)
    {
        // if we want to init the unit's subgraph
        case INIT_UNIT_GRAPH:
            {
                // clear the subgraph units
                mUnitSet.clear();
                mbValidSubgraph = true;

                // perform traversion in this mode
                n->traverse(*this);
        
                // setup the indices of the units, so that they got sorted correctly in the pipeline
                unsigned int index = mProcessor->getOrCreateStateSet()->getBinNumber();
                const std::string& binName = mProcessor->getOrCreateStateSet()->getBinName();
            
                // the unit set do contain units in the traversed order
                for (UnitSet::iterator it = mUnitSet.begin(); it != mUnitSet.end(); it++)
                {
                    // sort the unit into the according pipeline
                    //(*it)->getOrCreateStateSet()->setRenderBinToInherit();
                    (*it)->getOrCreateStateSet()->setRenderBinDetails(++index, binName);
                    //printf("INIT %s %d\n", (*it)->getName().c_str(), index);

                    // initialize units by updating them (the initialization process is called automagically
                    mProcessor->onUnitInit(*it);
                    (*it)->update();
                }
    
            }
            break;

        // optimize the graph
        case OPTIMIZE:
            if (mbValidSubgraph)
            {
                mMaxUnitInputIndex = 0;
                n->traverse(*this);

                // set the unit's input texture attributes accordingly
                osg::notify(osg::INFO) << "osgPPU::Visitor::perform(OPTIMIZE) - maximum possible texture index is " << mMaxUnitInputIndex << "." << std::endl;
                SetMaximumInputsVisitor sm(mMaxUnitInputIndex);
                n->traverse(sm);
            }
            break;

        // if we want to find a unit, then
        case FIND_UNIT:
            if (mbValidSubgraph)
            {
                mUnitToFind = NULL;
                n->traverse(*this);
            }    
            break;

        // if we want to remove a unit, then
        case REMOVE_UNIT:
            if (!mbValidSubgraph)
            {
                mUnitToRemoveResult = false;
                break;
            }else
            {
                n->traverse(*this);
            }
            break;

        // if we want to update the units
        case UPDATE:
            if (mbValidSubgraph)
            {
                // perform cleanup on node colors
                n->traverse(*mCleanTraversedMaskVisitor);
    
                // setup the traversal and start traversing
                setVisitorType(osg::NodeVisitor::UPDATE_VISITOR);
                n->traverse(*this);
                setVisitorType(osg::NodeVisitor::NODE_VISITOR);
            }
            break;

        // if we want to cull units (note: valid cull visitor must be provided)
        case CULL:
            if (mbValidSubgraph)
            {
                if (mCullVisitor == NULL) break;
    
                // perform cleanup on node colors
                n->traverse(*mCleanTraversedMaskVisitor);
    
                // setup the traversal and start traversing
                n->traverse(*mCullVisitor);    
            }   
            break;

        // in all other cases, just traverse
        default:
            if (mbValidSubgraph)
                n->traverse(*this);
    }

    mMode = NONE;
}

}; //end namespace




