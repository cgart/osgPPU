/***************************************************************************
 *   Copyright (c) 2009   Art Tevs                                         *
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

#ifndef _C_UNIT_BYPASS_REPEAT_H_
#define _C_UNIT_BYPASS_REPEAT_H_


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgPPU/Export.h>
#include <osgPPU/UnitBypass.h>

namespace osgPPU
{
    //! Implementation of repeatable UnitInOut (rendering several iterations)
    /**
    * A repeatable unit is an unit, which runs for several iterations. The unit subgraph
    * which parent this unit is, is rendered several times. So on the first call
    * parent of a repeatable unit is used as input to the subgraph. After full computation of
    * the subgraph the output of the last unit is set as input to the repeatable unit.
    * Then the subgraph is iterated until numIterations-1 is reached. 
    * After all iterations the continue child is executed with the output of the last unit in 
    * the repeatable subgraph.
    *
    * This unit is a bypass unit, which do not have any special rendering routines.
    * So the execution time of this Unit is negligable. The iterative execution is achieved by
    * using special CullVisitor, so using other kind of CullVisitors will break up here.
    *
    * NOTE: Currently only 1 input to the repeat unit is supported!
    *       Please check that last node (which output is reused) has also only 1 output specified!
    **/
    class OSGPPU_EXPORT UnitBypassRepeat : public UnitBypass {
        public:
            META_Node(osgPPU,UnitBypassRepeat);
        
            UnitBypassRepeat();
            UnitBypassRepeat(const UnitBypassRepeat&, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
            
            virtual ~UnitBypassRepeat();

            /**
            * Set child unit, which is used as last/output unit in the iterative unit subgraph.
            * After the first iteration the output of this unit is used as input to the iterative subgraph.
            * After last iteration the execution is passed to the children of this unit, so the unit
            * execution continues on the child of a last unit of iterative subgraph.
            * @param node Node in the scenegraph which is a last unit in the iterative subgraph
            **/
            void setLastNode(Unit* node);
            inline Unit* getLastNode() { return _lastNode.get(); }
            inline const Unit* getLastNode() const { return _lastNode.get(); }

            /**
            * Set the amount of iterations (default 1).
            **/
            inline void setNumIterations(int num) { _numIterations = num; dirty(); }
            inline int getNumIterations() { return _numIterations; }
            inline const unsigned getNumIterations() const { return _numIterations; }

            /**
            * If last node in the iterative subgraph has more than one output (MRT), then
            * specify here the mrt index of the output you would like to use while iterating.
            * (default is 0)
            **/
            inline void setLastNodeOutputIndex(unsigned index) { _lastNodeOutputIndex = index; dirty(); }
            inline unsigned getLastNodeOutputIndex() { return _lastNodeOutputIndex; }
            inline const unsigned getLastNodeOutputIndex() const { return _lastNodeOutputIndex; }

        protected:
            //! Overriden method from the base class
            virtual void init();

            //! Special treatment for traversing repeatable subgraph
            virtual void traverse(osg::NodeVisitor& nv);

            int _numIterations;
            unsigned _lastNodeOutputIndex;
            osg::ref_ptr<Unit>    _lastNode;
    };

};

#endif
