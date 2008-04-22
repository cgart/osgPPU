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

#include <sstream>

#include <osgPPU/Processor.h>
#include <osgPPU/Unit.h>

#include <osgDB/Registry>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>

#include <osg/NodeVisitor>

#include "Base.h"

// ----------------------------------------------------------------------------------------------------
// Helper class to visit each unit once and to write it to an output
// ----------------------------------------------------------------------------------------------------
class OutputVisitor : public osg::NodeVisitor
{
    public:
        OutputVisitor(osgDB::Output* out) : osg::NodeVisitor(), _output(out)
        {
            setNodeMaskOverride(0xffffffff);
        }
        ~OutputVisitor() {}

        void apply (osg::Group &node)
        {
            osgPPU::Unit* unit = dynamic_cast<osgPPU::Unit*>(&node);

            // not an unit, then just traverse it
            if (unit == NULL) traverse(node);

            // check if it wasn't visited before
            bool visited = false;
            for (unsigned int i=0; i < _visited.size(); i++)
                if (_visited[i] == unit) {visited = true; break;}

            // it is a unit and not visited before
            if (unit != NULL && !visited)
            {
                // because a unit can be referenced more than one, this would
                // produce wrong output to the .ppu file (see osgDB::Registry::writeObject),
                // therefor we artificially change the reference counter to 1
                int counter = unit->referenceCount();
                for (int i=0; i<counter; i++) unit->unref_nodelete();

                // write the object
                _output->writeObject(*unit);

                // increment the reference counter back
                for (int i=0; i<counter; i++) unit->ref();

                // place it
                _visited.push_back(unit);
                node.traverse(*this);
            }
        }
    
        
    private:
        osgDB::Output* _output;
        std::vector<osgPPU::Unit*> _visited;
};



class ReaderWriterPPU : public osgDB::ReaderWriter
{
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
        
        // during the reading we require to read some data from the osg plugin, hence preload this library 
        std::string pluginLibraryName = osgDB::Registry::instance()->createLibraryNameForExtension("osg");
        osgDB::Registry::instance()->loadLibrary(pluginLibraryName);

        // read input 
        osgDB::Input fr;
        fr.attach(&fin);

        // here we store the readed result
        osgPPU::Processor* pp = new osgPPU::Processor();

        // first read the pipeline
        if (fr.matchSequence((std::string(pp->libraryName()) + std::string("::") + std::string(pp->className())).c_str()))
        {
            // move in
            fr += 2;

            // setup the read options list
            ListReadOptions* list = new ListReadOptions();
            fr.setOptions(list);

            // this list will contain all readed units
            std::list<osg::ref_ptr<osgPPU::Unit> > units;

            // read all units in the file
            osg::Object* object = NULL;
            while((object=fr.readObject())!=NULL)
            {
                osgPPU::Unit* unit = dynamic_cast<osgPPU::Unit*>(object);
                if (unit)
                {
                    units.push_back(unit);
                }
            }
            
            // the option should contain now a list of output ppus
            for (ListReadOptions::List::const_iterator it = list->getList().begin(); it!= list->getList().end(); it++)
            {
                // for each output ppu do get the corresponding object
                for (std::list<std::string>::const_iterator kt=it->second.begin(); kt!=it->second.end(); kt++)
                {
                    osg::ref_ptr<osgPPU::Unit> unit = dynamic_cast<osgPPU::Unit*>(fr.getObjectForUniqueID(*kt));
                    if (!unit.valid())
                        osg::notify(osg::WARN)<<"Unit " << it->first->getName() << " cannot find input ppu " << *kt << std::endl;    
                    else
                        it->first->addChild(unit.get());
                }
            }

            // the option should contain now a map of uniform to inputs
            for (ListReadOptions::UniformInputMap::const_iterator it = list->getUniformInputMap().begin(); it!= list->getUniformInputMap().end(); it++)
            {
                // for each output ppu do get the corresponding object
                for (std::map<std::string, std::string>::const_iterator kt=it->second.begin(); kt!=it->second.end(); kt++)
                {
                    osg::ref_ptr<osgPPU::Unit> unit = dynamic_cast<osgPPU::Unit*>(fr.getObjectForUniqueID(kt->first));
                    if (!unit.valid())
                        osg::notify(osg::WARN)<<"Unit " << it->first->getName() << " cannot find correct input uniform mapping " << kt->first << " to " << kt->second << std::endl;
                    else
                    {
                        it->first->setInputToUniform(unit.get(), kt->second);
                    }
                }
            }

            // read processor's name    
            std::string name;
            if (fr.readSequence("name", name))
            { 
                pp->setName(name);
            }

            // read processor's direct ancestors
            if (fr.matchSequence("PPUOutput {"))
            {
                int entry = fr[0].getNoNestedBrackets();
        
                fr += 2;
        
                while (!fr.eof() && fr[0].getNoNestedBrackets()>entry)
                {
                    // input is a ppu
                    if (fr[0].matchWord("PPU"))
                    {
                        osg::ref_ptr<osgPPU::Unit> unit = dynamic_cast<osgPPU::Unit*>(fr.getObjectForUniqueID(fr[1].getStr()));
                        if (unit.valid())
                            pp->addChild(unit.get());
                        else
                            osg::notify(osg::FATAL)<<"osgPPU::readObject - Something bad happens, cannot parse processor!" << std::endl;    
                    }
                    ++fr;
                }
                
                // skip trailing '}'
                ++fr;
            }
            
            // skip trailing '}'
            ++fr;

        }

        return pp;
    }


    // ----------------------------------------------------------------------------------------------------
    virtual WriteResult writeObject(const osg::Object& obj,const std::string& fileName, const osgDB::ReaderWriter::Options* options) const
    {
        std::string ext = osgDB::getLowerCaseFileExtension(fileName);
        if (!acceptsExtension(ext)) return WriteResult::FILE_NOT_HANDLED;

        // check if correct object was given
        if (!dynamic_cast<const osgPPU::Processor*>(&obj))
        {
            return WriteResult("osgPPU::writeObject - Wrong object to write was given. Do only support osgPPU::Processor");
        }
        // during the writing we require to read some data from the osg plugin, hence preload this library 
        std::string pluginLibraryName = osgDB::Registry::instance()->createLibraryNameForExtension("osg");
        osgDB::Registry::instance()->loadLibrary(pluginLibraryName);

        // open file for writing
        osgDB::Output fout(fileName.c_str());
        fout.setOptions(options);

        // write to file 
        if (fout)
        {
            // convert object to processor pipeline 
            osgPPU::Processor& ppu = const_cast<osgPPU::Processor&>(static_cast<const osgPPU::Processor&>(obj));

            // we use a special traverser, hence mark the graph as non dirty for a while
            bool dirty = ppu.isDirtyUnitSubgraph();
            ppu.markUnitSubgraphNonDirty();

            // we can only write the unit graph if it is not dirty
            //if (ppu.isDirtyUnitSubgraph())
            //{
            //    return WriteResult("osgPPU::writeObject - Unit's subgraph is still dirty, run processor first to resolve all issues");            
            //}

            // write out information about the pipeline
            fout.writeBeginObject(std::string(ppu.libraryName()) + std::string("::") + std::string(ppu.className()));
            fout.moveIn();

                OutputVisitor ov(&fout);
                ppu.traverse(ov);

                // write processor's name                
                fout.indent() << "name " << ppu.getName() << std::endl;

                // write all outputs of the processor
                fout.writeBeginObject("PPUOutput");
                fout.moveIn();
                
                for (unsigned int i=0; i < ppu.getNumChildren(); i++)
                {
                    if (dynamic_cast<const osgPPU::Unit*>(ppu.getChild(i)))
                    {
                        std::string uid;
                        if (!fout.getUniqueIDForObject(ppu.getChild(i), uid))
                        {
                            fout.createUniqueIDForObject(ppu.getChild(i), uid);
                            fout.registerUniqueIDForObject(ppu.getChild(i), uid);
                        }
        
                        fout.indent() << "PPU " << uid << std::endl;
                    }            
                }


                fout.moveOut();
                fout.writeEndObject();

            fout.moveOut();
            fout.writeEndObject();

            if (dirty) ppu.dirtyUnitSubgraph();

            return WriteResult::FILE_SAVED;
        }

        return WriteResult("osgPPU::writeObject - Unable to open file for output");
    }


};


// now register with Registry to instantiate the above
REGISTER_OSGPLUGIN(ppu, ReaderWriterPPU)

