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
#ifndef _C_PIPELINE__H_
#define _C_PIPELINE__H_

//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgPPU/Export.h>
#include <osgPPU/Unit.h>
#include <osgUtil/RenderBin>
#include <osgUtil/RenderLeaf>

namespace osgPPU
{

//! Class derived osgUtil::RenderBin which do represent a pipeline of units
/**
*  The pipeline is derived from the
* osgUtil::RenderBin class so that all attached units are sorted into this bin.
* There is no public interface for the pipeline, since it is not required
* to work with it outside of the processor.
**/
class OSGPPU_EXPORT Pipeline : public osgUtil::RenderBin
{
    private:
        //! Processor is allowed to create/destroy pipelines
        friend class Processor;

        /**
        * Copy constructor to copy the ppus from the given pipeline.
        * @param pipeline Pipeline to copy from
        * @param copyop Specify how to copy the ppus (currentl only copying of pointers is supported)
        **/
        Pipeline(const Pipeline& pipeline, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

        /**
        * Sort the pipeline. This will sort all attached units based on their index
        * and dependency graph.
        **/
        void sortImplementation();
        
        /**
        * Get pipeline name. The name can only be specified during hte creation process
        * through the osgPPU::Processor. The name of the pipeline should be unique.
        **/
        inline const std::string& getName() const { return mName; }

        /**
        * Simple constructor to create empty pipeline.
        **/
        Pipeline();

        /**
        * Only processor is allowed to destroy this object.
        **/
        virtual ~Pipeline();

        /**
        * Only processor can setup the name of this pipeline.
        **/
        inline void setName(const std::string& name) { mName = name; }


        std::string mName;
        struct SortByIndex;

        
};

}; // end namespace

#endif
