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


#include <sstream>

#include <osgPPU/Processor.h>
#include <osgPPU/Unit.h>

#include <osgDB/Registry>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>


class ReaderWriterPPU : public osgDB::ReaderWriter
{
private:
    // ----------------------------------------------------------------------------------------------------
    void writeUnit(const osgPPU::Unit* unit, osgDB::Output& fout, const osgDB::ReaderWriter::Options* options) const
    {
        fout << std::endl;
        fout.writeBeginObject(std::string("osgPPU::") + std::string(unit->className()));
        fout.moveIn();
                fout.indent() << "Name " <<  fout.wrapString(unit->getName()) << std::endl;

        fout.moveOut();
        fout.writeEndObject();
    }

public:
    // ----------------------------------------------------------------------------------------------------
    virtual const char* className() const { return "osgPPU pipeline loader"; }
    

    // ----------------------------------------------------------------------------------------------------
    virtual bool acceptsExtension(const std::string& extension) const
    {
        return osgDB::equalCaseInsensitive(extension, "ppu");// || osgDB::equalCaseInsensitive(extension, "osgppu");      
    }


    // ----------------------------------------------------------------------------------------------------
    virtual ReadResult readObject(const std::string& file, const osgDB::ReaderWriter::Options* options) const
    {
        std::string ext = osgDB::getLowerCaseFileExtension(file);
        if( !acceptsExtension(ext) )
            return ReadResult::FILE_NOT_HANDLED;

        std::string fileName = osgDB::findDataFile( file, options );
        if (fileName.empty()) return ReadResult::FILE_NOT_FOUND;

        // code for setting up the database path so that internally referenced file are searched for on relative paths. 
        std::ifstream fin(fileName.c_str());
        if (fin)
        {
            return readObject(fin, options);
        }
        return 0L;
    }

    // ----------------------------------------------------------------------------------------------------
    virtual ReadResult readObject(std::istream& fin, const osgDB::ReaderWriter::Options* options) const
    {
        /*osgDB::Input fr;
        fr.attach(&fin);
        fr.setOptions(options);

        // load all nodes in file, placing them in a group.
        while(!fr.eof())
        {

        }*/

        return ReadResult::FILE_NOT_HANDLED;
    }

    // ----------------------------------------------------------------------------------------------------
    virtual WriteResult writeObject(const osg::Object& obj,const std::string& fileName, const osgDB::ReaderWriter::Options* options) const
    {
        std::string ext = osgDB::getLowerCaseFileExtension(fileName);
        if (!acceptsExtension(ext)) return WriteResult::FILE_NOT_HANDLED;

        // open file for writing
        osgDB::Output fout(fileName.c_str());
        fout.setOptions(options);

        // write to file 
        if (fout)
        {
            fout << "IN DEVELOPMENT - please ignore this file !!!" << std::endl<< std::endl;

            // convert object to processor pipeline 
            const osgPPU::Processor* ppu = dynamic_cast<const osgPPU::Processor*>(&obj);
            if (ppu == NULL)
                return WriteResult("osgPPU::writeObject - You have to provide osgPPU::Processor to write to file");
            
            // write out information about the pipeline
            fout.writeBeginObject("osgPPU::Processor");
            fout.moveIn();

                fout.indent() << "Name " <<  fout.wrapString(ppu->getName()) << std::endl;

                // for each unit in the pipeline write it to file 
                osgPPU::Processor::Pipeline::const_iterator it = ppu->getPipeline().begin();
                for (; it != ppu->getPipeline().end(); it++)
                {
                    writeUnit(it->get(), fout, options);
                }


            fout.moveOut();
            fout.writeEndObject();

            fout << std::endl << "IN DEVELOPMENT - please ignore this file !!!" << std::endl;

            return WriteResult::FILE_SAVED;
        }

        return WriteResult("osgPPU::writeObject - Unable to open file for output");
    }


};


// now register with Registry to instantiate the above
REGISTER_OSGPLUGIN(ppu, ReaderWriterPPU)

