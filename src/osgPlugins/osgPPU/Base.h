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
#ifndef _BASE_H_
#define _BASE_H_

#include <osg/StateAttribute>
#include <osg/Texture>

#include <osgDB/Registry>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>

#include <osgPPU/Unit.h>


//! Read options to store connection between unit and its input
class ListReadOptions : public osgDB::ReaderWriter::Options
{
public:
    typedef std::map<osgPPU::Unit*, std::list<std::string> > List;
    typedef std::map<osgPPU::Unit*, std::map<std::string,std::string> > UniformInputMap;

    void setList(const List& l) { mList = l;}
    List& getList() { return mList; }

    void setUniformInputMap(const UniformInputMap& l) { mUniformInputMap = l;}
    UniformInputMap& getUniformInputMap() { return mUniformInputMap; }

    ListReadOptions() : osgDB::ReaderWriter::Options()
    {}

    ~ListReadOptions()
    {}

private:

    List mList;
    UniformInputMap mUniformInputMap;
};


extern bool StateSet_matchModeStr(const char* str,osg::StateAttribute::GLModeValue& mode);
extern const char* StateSet_getModeStr(osg::StateAttribute::GLModeValue value);
extern bool Texture_matchInternalFormatStr(const char* str,int& value);
extern const char* Texture_getInternalFormatStr(int value);


#endif

