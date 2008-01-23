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
#ifndef _BASE_H_
#define _BASE_H_

#include <osg/StateAttribute>
#include <osg/Texture>

#include <osgDB/Registry>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>

#include <osgPPU/Unit.h>

//! Just options which we pass from the readerwriter to the object writer functions
class ListOptions : public osgDB::ReaderWriter::Options
{
public:
    typedef std::list<std::pair<osgPPU::Unit*, osg::Texture*> > List;

    void setList(const List& l) { mList = l;}
    const List& getList() const { return mList; }

    ListOptions() : osgDB::ReaderWriter::Options()
    {}

    ~ListOptions()
    {}


private:

    List mList;
};



extern bool StateSet_matchModeStr(const char* str,osg::StateAttribute::GLModeValue& mode);
extern const char* StateSet_getModeStr(osg::StateAttribute::GLModeValue value);
extern bool Texture_matchInternalFormatStr(const char* str,int& value);
extern const char* Texture_getInternalFormatStr(int value);


#endif

