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
#ifndef _IO_UNIT_H_
#define _IO_UNIT_H_

#include <osgPPU/Processor.h>
#include <osgPPU/Unit.h>
#include <osgPPU/UnitInOut.h>
#include <osgPPU/UnitOut.h>
#include <osgPPU/UnitOutCapture.h>
#include <osgPPU/UnitInResampleOut.h>
#include <osgPPU/UnitText.h>
#include <osgPPU/UnitBypass.h>

#include <osg/Notify>

#include "Base.h"

//--------------------------------------------------------------------------
bool readUnitInOut(osg::Object& obj, osgDB::Input& fr)
{
    // convert given object to unit 
    osgPPU::UnitInOut& unit = static_cast<osgPPU::UnitInOut&>(obj);

    bool itAdvanced = false;

    int isMipmappedIO = 0;
    if (fr.readSequence("isMipmappedInOut", isMipmappedIO))
    { 
        unit.setMipmappedInOut(isMipmappedIO);
        itAdvanced = true;
    }

    // read shader input 
    if (fr.matchSequence("MipmapShader {"))
    {
        int entry = fr[0].getNoNestedBrackets();

        fr += 2;

        // create shader
        osgPPU::Shader* shader = new osgPPU::Shader();

        // read data associated with the shader
        while (!fr.eof() && fr[0].getNoNestedBrackets()>entry)
        {
            bool local_itrAdvanced = false;

            // read uniform
            if (fr.matchSequence("RefUniformPair {"))
            {
                fr += 2;

                // read uniform
                osg::Uniform* uniform = fr.readUniform();
    
                // read state attribute
                std::string stateAttribute;
                osg::StateAttribute::GLModeValue mode = osg::StateAttribute::ON;
                if (fr.readSequence("StateAttribute", stateAttribute))
                {
                    StateSet_matchModeStr(stateAttribute.c_str(),mode);
                }
                
                ++fr;
                local_itrAdvanced = true;

                // create refuniformpair
                osg::StateSet::RefUniformPair pair (uniform, mode);

                // add uniform to the shader 
                shader->add(pair);
            }

            // read program
            osg::Program* program = dynamic_cast<osg::Program*>(fr.readObjectOfType(osgDB::type_wrapper<osg::Program>()));
            if (program)
            {
                shader->setProgram(program);
                local_itrAdvanced = true;
            }


            if (!local_itrAdvanced) ++fr;
        }
        
        // set new shader to the unit 
        unit.setGenerateMipmapsShader(shader);

        // skip trailing '}'
        ++fr;
        
        itAdvanced = true;
    }


    int useMipmapShader = 0;
    if (fr.readSequence("useMipmapShader", useMipmapShader))
    { 
        unit.setUseGenerateMipmapsShader(useMipmapShader);
        itAdvanced = true;
    }

    int useMipmaps = 0;
    if (fr.readSequence("useMipmaps", useMipmaps))
    { 
        unit.setUseMipmaps(useMipmaps);
        itAdvanced = true;
    }

    int mrtCount = -1;
    if (fr.readSequence("mrtCount", mrtCount))
    { 
        unit.setMRTNumber(mrtCount);
        itAdvanced = true;
    }

    return itAdvanced;
}

//--------------------------------------------------------------------------
bool readUnitInResampleOut(osg::Object& obj, osgDB::Input& fr)
{
    // convert given object to unit 
    osgPPU::UnitInResampleOut& unit = static_cast<osgPPU::UnitInResampleOut&>(obj);

    bool itAdvanced = false;

    float factorX = 0;
    if (fr.readSequence("factorX", factorX))
    { 
        unit.setFactorX(factorX);
        itAdvanced = true;
    }

    float factorY = 0;
    if (fr.readSequence("factorY", factorY))
    { 
        unit.setFactorY(factorY);
        itAdvanced = true;
    }

    return itAdvanced;
}

//--------------------------------------------------------------------------
bool readUnitText(osg::Object& obj, osgDB::Input& fr)
{
    // convert given object to unit 
    osgPPU::UnitText& unit = static_cast<osgPPU::UnitText&>(static_cast<osgPPU::UnitInOut&>(obj));

    bool itAdvanced = false;

    int size = 0;
    if (fr.readSequence("size", size))
    { 
        unit.setSize(size);
        itAdvanced = true;
    }

    return itAdvanced;
}

//--------------------------------------------------------------------------
bool readUnitOutCapture(osg::Object& obj, osgDB::Input& fr)
{
    // convert given object to unit 
    osgPPU::UnitOutCapture& unit = static_cast<osgPPU::UnitOutCapture&>(obj);

    bool itAdvanced = false;

    std::string path = 0;
    if (fr.readSequence("Path", path))
    { 
        unit.setPath(path);
        itAdvanced = true;
    }

    std::string ext = 0;
    if (fr.readSequence("Extension", ext))
    { 
        unit.setFileExtension(ext);
        itAdvanced = true;
    }

    return itAdvanced;
}


//--------------------------------------------------------------------------
bool readUnit(osg::Object& obj, osgDB::Input& fr)
{
    // convert given object to unit 
    osgPPU::Unit& unit = static_cast<osgPPU::Unit&>(obj);
    bool itAdvanced = false;

    std::string name;
    if (fr.readSequence("name", name))
    { 
        unit.setName(name);
        itAdvanced = true;
    }

    int index = 0;
    if (fr.readSequence("index", index))
    { 
        unit.setIndex(index);
        itAdvanced = true;
    }

    float blendEndTime = 0.0;
    if (fr.readSequence("blendFinalTime", blendEndTime))
    { 
        unit.setBlendFinalTime(blendEndTime);
        itAdvanced = true;
    }

    float blendStartTime = 0.0;
    if (fr.readSequence("blendStartTime", blendStartTime))
    { 
        unit.setBlendStartTime(blendStartTime);
        itAdvanced = true;
    }

    float blendStartValue = 0.0;
    if (fr.readSequence("blendStartValue", blendStartValue))
    { 
        unit.setBlendStartValue(blendStartValue);
        itAdvanced = true;
    }

    float blendEndValue = 0.0;
    if (fr.readSequence("blendFinalValue", blendEndValue))
    { 
        unit.setBlendFinalValue(blendEndValue);
        itAdvanced = true;
    }

    int blendEnabled = 0;
    if (fr.readSequence("blendEnabled", blendEnabled))
    { 
        unit.setUseBlendMode(blendEnabled);
        itAdvanced = true;
    }

    int isActive = 0;
    if (fr.readSequence("isActive", isActive))
    { 
        unit.setActive(isActive);
        itAdvanced = true;
    }

    int isOffline = 0;
    if (fr.readSequence("isOffline", isOffline))
    { 
        unit.setOfflineMode(isOffline);
        itAdvanced = true;
    }

    int inputTextureIndexForViewportReference = 0;
    if (fr.readSequence("inputTextureIndexForViewportReference", inputTextureIndexForViewportReference))
    { 
        unit.setInputTextureIndexForViewportReference(inputTextureIndexForViewportReference);
        itAdvanced = true;
    }


    std::string internalFormatStr;
    if (fr.readSequence("outputInternalFormat", internalFormatStr))
    { 
        int fmt;
        if (Texture_matchInternalFormatStr(internalFormatStr.c_str(), fmt))
        {
            unit.setOutputInternalFormat(fmt);
        }else{
            osg::notify(osg::WARN)<<"Unit " << unit.getName() << " cannot read output internal format." << std::endl;    
        }
        itAdvanced = true;
    }


    // read viewport 
    osg::Viewport *vp = static_cast<osg::Viewport*>(fr.readObjectOfType(osgDB::type_wrapper<osg::Viewport>()));
    if (vp)
    {
        unit.setViewport(vp);
        itAdvanced = true;
    }

    // read all inputs
    if (fr.matchSequence("Input {"))
    {
        int entry = fr[0].getNoNestedBrackets();

        fr += 2;

        while (!fr.eof() && fr[0].getNoNestedBrackets()>entry)
        {
            bool local_itrAdvanced = false;
            std::string ppuName;

            // input is a ppu
            if (fr[0].matchWord("PPU"))
            {
                // get ppu name
                ppuName = fr[1].getStr();

                // reader should contain the list options
                ListReadOptions* opt = dynamic_cast<ListReadOptions*>(const_cast<osgDB::ReaderWriter::Options*>(fr.getOptions()));
                if (opt)
                {
                    ListReadOptions::List& list = opt->getList();
                    list[&unit].push_back(ppuName);
                }else{
                    osg::notify(osg::WARN)<<"osgPPU::readObject - Something bad happens!" << std::endl;    
                }
            // if not ppu then texture
            }else
            {
                osg::Texture* tex = dynamic_cast<osg::Texture*>(fr.readObjectOfType(osgDB::type_wrapper<osg::Texture>()));
                if (tex)
                {
                    unit.setInputTexture(tex, unit.getInputTextureMap().size());
                    local_itrAdvanced = true;
                }
            }

            if (!local_itrAdvanced) ++fr;
        }
        
        // skip trailing '}'
        ++fr;
        
        itAdvanced = true;
    }
    
    // read shader input 
    if (fr.matchSequence("Shader {"))
    {
        int entry = fr[0].getNoNestedBrackets();

        fr += 2;

        // create shader
        osgPPU::Shader* shader = new osgPPU::Shader();

        // read data associated with the shader
        while (!fr.eof() && fr[0].getNoNestedBrackets()>entry)
        {
            bool local_itrAdvanced = false;

            // read uniform
            if (fr.matchSequence("RefUniformPair {"))
            {
                fr += 2;

                // read uniform
                osg::Uniform* uniform = fr.readUniform();
    
                // read state attribute
                std::string stateAttribute;
                osg::StateAttribute::GLModeValue mode = osg::StateAttribute::ON;
                if (fr.readSequence("StateAttribute", stateAttribute))
                {
                    StateSet_matchModeStr(stateAttribute.c_str(),mode);
                }
                
                ++fr;
                local_itrAdvanced = true;

                // create refuniformpair
                osg::StateSet::RefUniformPair pair (uniform, mode);

                // add uniform to the shader 
                shader->add(pair);
            }

            // read program
            osg::Program* program = dynamic_cast<osg::Program*>(fr.readObjectOfType(osgDB::type_wrapper<osg::Program>()));
            if (program)
            {
                shader->setProgram(program);
                local_itrAdvanced = true;
            }


            if (!local_itrAdvanced) ++fr;
        }
        
        // set new shader to the unit 
        unit.setShader(shader);

        // skip trailing '}'
        ++fr;
        
        itAdvanced = true;
    }



    return itAdvanced;
}

//--------------------------------------------------------------------------
void writeShader(const osgPPU::Shader* sh, osgDB::Output& fout)
{
    // write uniform list
    osg::StateSet::UniformList::const_iterator jt = sh->getUniformList().begin();
    for (; jt != sh->getUniformList().end(); jt++)
    {
        fout.writeBeginObject("RefUniformPair");
        fout.moveIn();
            
            osgDB::Registry::instance()->writeObject(static_cast<const osg::Uniform&>(*(jt->second.first)), fout);
            fout.indent() << "StateAttribute " << StateSet_getModeStr(jt->second.second) << std::endl;

        fout.moveOut();
        fout.writeEndObject();
    }

    // force write shaders to files
    //fout.setOutputShaderFiles(true);

    // write out shader program
    osgDB::Registry::instance()->writeObject(static_cast<const osg::Program&>(*(sh->getProgram())), fout);
}


//--------------------------------------------------------------------------
bool writeUnit(const osg::Object& obj, osgDB::Output& fout)
{
    // convert given object to unit 
    const osgPPU::Unit& unit = static_cast<const osgPPU::Unit&>(obj);

    // the list of input ppus must be passed as option 
    const ListWriteOptions::List& ilist = dynamic_cast<const ListWriteOptions*>(fout.getOptions())->getList();

    // since we do not directlz reference to other ppu's but to its textures
    // it can happen that the unique id will not be written, hence force it here
    /*std::string uid;
    if (obj.referenceCount() <= 1)// !fout.getUniqueIDForObject(&obj, uid))
    {
        fout.createUniqueIDForObject(&obj, uid);
        fout.registerUniqueIDForObject(&obj, uid);
        fout.writeUniqueID(uid);
    }*/

    // retrieve default parameters and sotre them
    fout.indent() << "name " <<  fout.wrapString(unit.getName()) << std::endl;
    fout.indent() << "index " <<  unit.getIndex() << std::endl;
    fout.indent() << "blendFinalTime " <<  unit.getBlendFinalTime() << std::endl;
    fout.indent() << "blendStartTime " <<  unit.getBlendStartTime() << std::endl;
    fout.indent() << "blendStartValue " <<  unit.getBlendStartValue() << std::endl;
    fout.indent() << "blendFinalValue " <<  unit.getBlendFinalValue() << std::endl;
    fout.indent() << "blendEnabled " <<  unit.getUseBlendMode() << std::endl;
    fout.indent() << "isActive " <<  unit.getActive() << std::endl;
    fout.indent() << "isOffline " <<  unit.getOfflineMode() << std::endl;
    fout.indent() << "inputTextureIndexForViewportReference " <<  unit.getInputTextureIndexForViewportReference() << std::endl;
    
    // write internal format
    {
        const char* str = Texture_getInternalFormatStr(unit.getOutputInternalFormat());
        
        if (str) fout.indent() << "outputInternalFormat " << str << std::endl;
        else fout.indent() << "outputInternalFormat " << unit.getOutputInternalFormat() << std::endl;
    }

    // for non bypass ppus do specify inputs
    fout << std::endl;
    fout.writeBeginObject("Input");
    fout.moveIn();

        ListWriteOptions::List::const_iterator it = ilist.begin();
        for (int index = 0; it != ilist.end(); it++, index++)
        {
            // input texture is there but no according ppu, hence external input
            if (it->first == NULL && it->second == NULL)
            {
                osg::notify(osg::WARN)<<"Unit " << unit.getName() << " has incorrect inputs." << std::endl;    
                //fout.indent() << "PPU " << fout.wrapString(it->first->getName()) << std::endl;
            }else if (it->first == NULL && it->second != NULL)
            {
                //osg::notify(osg::WARN)<<"Unit " << unit.getName() << " has as input an external texture." << std::endl;
                osgDB::Registry::instance()->writeObject(static_cast<const osg::Texture&>(*(it->second)), fout);
            }else
            {
                // create unique id name for the ppu 
                /*std::string uid;
                if (!fout.getUniqueIDForObject(it->first, uid))
                {
                    //fout.createUniqueIDForObject(it->first, uid);
                    //fout.registerUniqueIDForObject(it->first, uid);
                    osg::notify(osg::WARN)<<"Unit " << unit.getName() << " cannot find input " << it->first->getName() << std::endl;    
                }else*/
                { 
                fout.indent() << "PPU " << fout.wrapString(it->first->getName()) << std::endl;
                //fout.indent() << "PPU " << uid << std::endl;
                }
            }
        }

    fout.moveOut();
    fout.writeEndObject();

    // write out viewport of the ppu
    if (unit.getViewport())
    {
        fout << std::endl;
        osgDB::Registry::instance()->writeObject(static_cast<const osg::Viewport&>(*(unit.getViewport())), fout);
    }

    // if the unit contains shader, then
    if (unit.getShader())
    {
        fout << std::endl;
        fout.writeBeginObject("Shader");
        fout.moveIn();
            writeShader(unit.getShader(), fout);
        fout.moveOut();
        fout.writeEndObject();
    }

    return true;
}



//--------------------------------------------------------------------------
bool writeUnitOutCapture(const osg::Object& obj, osgDB::Output& fout)
{
    // convert given object to unit 
    const osgPPU::UnitOutCapture& unit = static_cast<const osgPPU::UnitOutCapture&>(obj);
        
    fout.indent() << "Path " <<  fout.wrapString(unit.getPath()) << std::endl;
    fout.indent() << "Extension " <<  fout.wrapString(unit.getFileExtension()) << std::endl;

    return true;
}




//--------------------------------------------------------------------------
bool writeUnitInOut(const osg::Object& obj, osgDB::Output& fout)
{
    // convert given object to unit 
    const osgPPU::UnitInOut& unit = static_cast<const osgPPU::UnitInOut&>(obj);

    fout.indent() << "isMipmappedInOut " <<  unit.getMipmappedInOut() << std::endl;
    fout.indent() << "useMipmapShader " << unit.getUseGenerateMipmapsShader() << std::endl;
    fout.indent() << "useMipmaps " << unit.getUseMipmaps() << std::endl;
    fout.indent() << "mrtCount" << unit.getMRTNumber() << std::endl;

    if (unit.getGenerateMipmapsShader())
    {
        fout << std::endl;
        fout.writeBeginObject("MipmapShader");
        fout.moveIn();
            writeShader(unit.getGenerateMipmapsShader(), fout);
        fout.moveOut();
        fout.writeEndObject();
    }

    return true;
}


//--------------------------------------------------------------------------
bool writeUnitInResampleOut(const osg::Object& obj, osgDB::Output& fout)
{
    // convert given object to unit 
    const osgPPU::UnitInResampleOut& unit = static_cast<const osgPPU::UnitInResampleOut&>(obj);

    fout.indent() << "factorX " <<  unit.getFactorX() << std::endl;
    fout.indent() << "factorY " <<  unit.getFactorY() << std::endl;

    return true;
}


//--------------------------------------------------------------------------
bool writeUnitText(const osg::Object& obj, osgDB::Output& fout)
{
    // convert given object to unit 
    const osgPPU::UnitText& unit = static_cast<const osgPPU::UnitText&>(static_cast<const osgPPU::Unit&>(obj));

    fout.indent() << "size " <<  unit.getSize() << std::endl;

    return true;
}



// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_UnitProxy
(
    NULL,
    "Unit",
    "Unit",
    &readUnit,
    &writeUnit
);

// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_UnitOutProxy
(
    new osgPPU::UnitOut,
    "UnitOut",
    "UnitOut",
    &readUnit,
    &writeUnit
);

// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_UnitBypassProxy
(
    new osgPPU::UnitBypass,
    "UnitBypass",
    "UnitBypass",
    &readUnit,
    &writeUnit
);

// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_UnitOutCaptureProxy
(
    new osgPPU::UnitOutCapture,
    "UnitOutCapture",
    "Unit UnitOut UnitOutCapture",
    &readUnitOutCapture,
    &writeUnitOutCapture
);

// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_UnitInOutProxy
(
    new osgPPU::UnitInOut,
    "UnitInOut",
    "Unit UnitInOut",
    &readUnitInOut,
    &writeUnitInOut
);

// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_UnitInResampleOutProxy
(
    new osgPPU::UnitInResampleOut,
    "UnitInResampleOut",
    "Unit UnitInOut UnitInResampleOut",
    &readUnitInResampleOut,
    &writeUnitInResampleOut
);

// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_UnitTextProxy
(
    static_cast<osg::Object*>(static_cast<osgPPU::Unit*>(new osgPPU::UnitText)),
    "UnitText",
    "Unit UnitInOut UnitText",

    // HACK: must be the line below, but produce some segfaults in the osgdb_osgtext.so
    //"Unit UnitInOut UnitText osgText::Text Object",
    &readUnitText,
    &writeUnitText
);


#endif

