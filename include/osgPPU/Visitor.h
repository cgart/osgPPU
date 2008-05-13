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
#ifndef _C_VISITOR__H_
#define _C_VISITOR__H_

//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgPPU/Export.h>
#include <osgPPU/Processor.h>
#include <osgPPU/Unit.h>
#include <osg/NodeVisitor>
#include <osg/Group>

namespace osgPPU
{

//------------------------------------------------------------------------------
// Helper visitor to perform traverse mask swapping
//------------------------------------------------------------------------------
class CleanTraverseMaskVisitor : public osg::NodeVisitor
{
public:

    CleanTraverseMaskVisitor() : osg::NodeVisitor()
    {
    }

    void apply (osg::Group &node)
    {
        Unit* unit = dynamic_cast<Unit*>(&node);
        if (unit)
        {
            unit->mbTraversed = unit->mbTraversedMask;
        }
        node.traverse(*this);
    }
};


//------------------------------------------------------------------------------
// Helper visitor to setup maximum number of input attachments
//------------------------------------------------------------------------------
class SetMaximumInputsVisitor : public osg::NodeVisitor
{
public:

    SetMaximumInputsVisitor(unsigned int max) : osg::NodeVisitor()
    {
        mMaxUnitInputIndex = max;
    }

    void apply (osg::Group &node)
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

    unsigned int mMaxUnitInputIndex;
};

//! Node visitor used to setup the units correctly
/**
* This visitor do work on the unit graph. It checks if a unit
* was added to the graph and set its inputs and outputs correctly.
* The processor does usually apply this visitor to its subgraph.
*
* Another feature of that visitor is to resolve cycles in the unit subgraph.
* This is done in the optimization mode, where the visitor do inserts
* a node into the cycle which blocks the cyclic traversion. Hence the optimization
* step has always be done before traversing the unit graph in a usual way.
**/
class OSGPPU_EXPORT Visitor : public osg::NodeVisitor
{
    public:
        typedef enum 
        {
            NONE = 0,
            INIT_UNIT_GRAPH = 1,
            DIRTY_ALL_UNITS = 2,
            FIND_UNIT = 3,
            UPDATE = 4,
            CULL = 5,
            RESOLVE_CYCLES = 6,
            OPTIMIZE = 7,
            REMOVE_UNIT = 8
        } Mode;


        /**
        * Setup the processor which will be used during the traversal of the graph.
        * The processor is required to get the attached camera and other settings
        * specified for the unit subgraph.
        **/
        Visitor(Processor* proc);
        ~Visitor();
        
        /**
        * Check if the traversed subgraph is valid.
        * An unvalid subgraph exists when it is cyclic.
        * The rendering shouldn't be performed unless the graph became valid.
        * Use RESOLVE_CYCLES running mode to resolve all cycles first.
        **/
        inline bool isTraversedGraphValid() const { return mbValidSubgraph; }

        /**
        * Perform traversion of this visitor in the unit's subgraph.
        * Specify the mode which is used in the traversion.
        **/
        void perform(Mode, osg::Group*);

        /**
        * Set culling visitor, which is used for culling the subgraph.
        **/
        inline void setCullVisitor(osg::NodeVisitor* nv) { mCullVisitor = nv; }

        /**
        * Find a unit in the subgraph based on its name.
        **/
        inline Unit* findUnit(const std::string& name, osg::Group* n)
        {
            mUnitToFindName = name;
            perform(FIND_UNIT, n);
            return mUnitToFind;
        }

        /**
        * Remove unit from the graph
        **/
        inline bool removeUnit(Unit* unit, osg::Group* root)
        {
            mUnitToRemove = unit;
            perform(REMOVE_UNIT, root);
            return mUnitToRemoveResult;
        }

        /**
        * Get current working mode of the visitor.
        * You need this to detect if a node which is currently traversed is traversed
        * by this visitor or not.
        **/
        Mode getMode() const { return mMode; }

    private:

        /**
        * Apply the visitor to a unit. The apply method is called for
        * the group, since Unit is derived from group.
        **/
        void apply (osg::Group &node);
        
        typedef std::list<Unit*> UnitSet;

        osg::ref_ptr<CleanTraverseMaskVisitor> mCleanTraversedMaskVisitor;
        Processor* mProcessor;
        Unit* mUnitToFind;
        Unit* mUnitToRemove;

        Mode mMode;
        std::string mUnitToFindName;
        UnitSet mUnitSet;     
        osg::NodeVisitor* mCullVisitor;

        bool mbValidSubgraph;
        bool mUnitToRemoveResult;
        unsigned int mMaxUnitInputIndex;
};

}; // end namespace

#endif
