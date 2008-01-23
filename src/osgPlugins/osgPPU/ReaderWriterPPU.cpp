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

#include "Base.h"

class ReaderWriterPPU : public osgDB::ReaderWriter
{
private:
    // ----------------------------------------------------------------------------------------------------
    std::list<std::pair<osgPPU::Unit*, osg::Texture*> > findInputs(const osgPPU::Processor& ppu, const osgPPU::Unit* unit) const
    {
        std::list<std::pair<osgPPU::Unit*, osg::Texture*> > list;
        int inputCount = 0;

        // get list of all inputs for this unit
        const osgPPU::Unit::TextureMap& map = unit->getInputTextureMap();

        // for each texture in the map search for a ppu, which do has this as output
        for (osgPPU::Unit::TextureMap::const_iterator it = map.begin(); it!=map.end(); it++)
        {
            if (it->second.valid())
            {
                int found = -1;

                // scan the pipeline and search for an unit which output is equal to this input 
                osgPPU::Pipeline::const_iterator jt = ppu.getPipeline().begin();
                for (; jt != ppu.getPipeline().end(); jt++)
                {
                    // since we support multiple outputs we have to check each output 
                    const osgPPU::Unit::TextureMap& outmap = (*jt)->getOutputTextureMap();
                    for (osgPPU::Unit::TextureMap::const_iterator ot = outmap.begin(); ot!=outmap.end(); ot++)
                    {
                        // if output texture is equal to the input, then add this ppu into the list
                        if (ot->second.get() == it->second.get())
                        {
                            // if this is a bypass ppu and the input is equal to the output, then 
                            // mark this appropriately
                            //if (!strcmp((*jt)->className(), "Unit") && unit == jt->get())
                            //{
                            //    list.push_back(std::pair<osgPPU::Unit*, osg::Texture*>(jt->get(), NULL));                                 
                            //}else
                                // if we have already found one before, then do replace that
                                // if the index of the one to check is greater
                                if ((*jt)->getIndex() >= found && found != -1)
                                {
                                    list.back() = std::pair<osgPPU::Unit*, osg::Texture*>(jt->get(), it->second.get()); 
                                }else{
                                    if (inputCount < map.size())
                                    {
                                        list.push_back(std::pair<osgPPU::Unit*, osg::Texture*>(jt->get(), it->second.get())); 
                                        inputCount ++;
                                    }
                                }
                            found = (*jt)->getIndex();
                            break;
                        }
                    }
                }
                // no ppu found, but input is specified, hence it is an external input, therefor set to NULL
                if (found == -1)
                {
                    list.push_back(std::pair<osgPPU::Unit*, osg::Texture*>(NULL, it->second.get())); 
                    inputCount ++;
                }

            }else
                list.push_back(std::pair<osgPPU::Unit*, osg::Texture*>(NULL, NULL)); 
        }
        return list;
    }


public:
    // ----------------------------------------------------------------------------------------------------
    virtual const char* className() const { return "osgPPU pipeline loader"; }
    

    // ----------------------------------------------------------------------------------------------------
    virtual bool acceptsExtension(const std::string& extension) const
    {
        return osgDB::equalCaseInsensitive(extension, "ppu") || osgDB::equalCaseInsensitive(extension, "osgppu");      
    }


    // ----------------------------------------------------------------------------------------------------
    virtual ReadResult readObject(const std::string& file, const osgDB::ReaderWriter::Options* options) const
    {
        std::string ext = osgDB::getLowerCaseFileExtension(file);
        if( !acceptsExtension(ext) )
            return ReadResult::FILE_NOT_HANDLED;

        std::string fileName = osgDB::findDataFile( file, options );
        if (fileName.empty()) return ReadResult::FILE_NOT_FOUND;

        std::ifstream fin(fileName.c_str());
        if (!fin) return ReadResult("osgPPU::readObject - Unable to open file for reading");
        
        // read input 
        osgDB::Input fr;
        fr.attach(&fin);

        // here we store the readed result
        osgPPU::Pipeline* pp = new osgPPU::Pipeline();
    
        // read all units in the file
        osg::Object* object = NULL;
        while((object=fr.readObject())!=NULL)
        {
            osgPPU::Unit* unit = dynamic_cast<osgPPU::Unit*>(object);
            if (unit) pp->push_back(unit);
        }
        
        return pp;
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
            // convert object to processor pipeline 
            const osgPPU::Processor& ppu = static_cast<const osgPPU::Processor&>(obj);
            
            // write out information about the pipeline
            fout.writeBeginObject(std::string(ppu.libraryName()) + std::string("::") + std::string(ppu.getPipeline().className()));
            fout.moveIn();
                
                // for each unit in the pipeline write it to file 
                osgPPU::Pipeline::const_iterator it = ppu.getPipeline().begin();
                for (; it != ppu.getPipeline().end(); it++)
                {
                    // setup options for this writer
                    ListOptions* op = new ListOptions();
                    fout.setOptions(op);

                    // create a list of all input data to the ppu
                    // this list will be used to map input to ppu and to store it correctly
                    // if an external input is used a null will be stored
                    op->setList(findInputs(ppu, it->get()));

                    // write object, this would cause the plugin to find correct wrapper and to write 
                    fout.writeObject(**it);
                }


            fout.moveOut();
            fout.writeEndObject();

            return WriteResult::FILE_SAVED;
        }

        return WriteResult("osgPPU::writeObject - Unable to open file for output");
    }


};


// now register with Registry to instantiate the above
REGISTER_OSGPLUGIN(ppu, ReaderWriterPPU)

