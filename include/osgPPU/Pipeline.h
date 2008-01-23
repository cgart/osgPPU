/***************************************************************************
 *   Copyright (c) 2008   Art Tevs                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 ***************************************************************************/

#ifndef _C_PIPELINE__H_
#define _C_PIPELINE__H_


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgPPU/Export.h>
#include <osgPPU/Unit.h>

namespace osgPPU
{

//! Class derived from std::list which do represent a pipeline of units
/**
* We need this class definition to allow reading adn writing of the pipeline
* into files.
**/
class OSGPPU_EXPORT Pipeline : public std::list<osg::ref_ptr<Unit> >, public osg::Object
{
    public:
        META_Object(osgPPU, Pipeline);

        /**
        * Simple constructor to create empty pipeline.
        **/
        Pipeline() : std::list<osg::ref_ptr<Unit> >(), osg::Object()
        {
        }

        /**
        * Copy constructor to copy the ppus from the given pipeline.
        * @param pipeline Pipeline to copy from
        * @param copyop Specify how to copy the ppus (currentl only copying of pointers is supported)
        **/
        Pipeline(const Pipeline& pipeline, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY) :
            std::list<osg::ref_ptr<Unit> >(pipeline), osg::Object(pipeline, copyop)
        {

        }

};

}; // end namespace

#endif
