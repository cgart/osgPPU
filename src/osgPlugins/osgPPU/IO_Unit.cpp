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
#include <osgPPU/Processor.h>
#include <osgPPU/Unit.h>
#include <osgPPU/UnitInOut.h>
#include <osgPPU/UnitInMipmapOut.h>
#include <osgPPU/UnitMipmapInMipmapOut.h>
#include <osgPPU/UnitOut.h>
#include <osgPPU/UnitOutCapture.h>
#include <osgPPU/UnitInResampleOut.h>
#include <osgPPU/UnitText.h>
#include <osgPPU/UnitBypass.h>
#include <osgPPU/UnitDepthbufferBypass.h>
#include <osgPPU/UnitTexture.h>
#include <osgPPU/BarrierNode.h>
#include <osgPPU/ColorAttribute.h>

#include <osg/Notify>
#include <osg/io_utils>

#include "Base.h"

//--------------------------------------------------------------------------
bool readColorAttribute(osg::Object& obj, osgDB::Input& fr)
{
    // convert given object to unit 
    osgPPU::ColorAttribute& ca = static_cast<osgPPU::ColorAttribute&>(obj);

    bool itAdvanced = false;

    float startTime  = 0;
    if (fr.readSequence("startTime", startTime))
    { 
        ca.setStartTime(startTime);
        itAdvanced = true;
    }

    float endTime  = 0;
    if (fr.readSequence("endTime", endTime))
    { 
        ca.setEndTime(endTime);
        itAdvanced = true;
    }

    osg::Vec4 startColor;
    if (fr.readSequence("startColor", startColor))
    { 
        ca.setStartColor(startColor);
        itAdvanced = true;
    }

    osg::Vec4 endColor;
    if (fr.readSequence("endColor", endColor))
    { 
        ca.setEndColor(endColor);
        itAdvanced = true;
    }

    return itAdvanced;
}

//--------------------------------------------------------------------------
bool readUnitTexture(osg::Object& obj, osgDB::Input& fr)
{
    // convert given object to unit 
    osgPPU::UnitTexture& unit = static_cast<osgPPU::UnitTexture&>(obj);

    bool itAdvanced = false;

    osg::Texture* texture = static_cast<osg::Texture*>(fr.readObjectOfType(osgDB::type_wrapper<osg::Texture>()));
    if (texture)
    {
        unit.setTexture(texture);
        itAdvanced = true;
    }

    return itAdvanced;
}

//--------------------------------------------------------------------------
bool readUnitInOut(osg::Object& obj, osgDB::Input& fr)
{
    // convert given object to unit 
    osgPPU::UnitInOut& unit = static_cast<osgPPU::UnitInOut&>(obj);

    bool itAdvanced = false;

    int index = 0;
    if (fr.readSequence("inputBypass", index))
    { 
        unit.setInputBypass(index);
        itAdvanced = true;
    }

    return itAdvanced;
}

//--------------------------------------------------------------------------
bool readUnitMipmapInMipmapOut(osg::Object& obj, osgDB::Input& fr)
{
    // there are no special variables to read
    return true;
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
bool readUnitInMipmapOut(osg::Object& obj, osgDB::Input& fr)
{
    // convert given object to unit 
    osgPPU::UnitInMipmapOut& unit = static_cast<osgPPU::UnitInMipmapOut&>(obj);

    bool itAdvanced = false;

    int inputIndex = 0;
    if (fr.readSequence("inputIndex", inputIndex))
    { 
        unit.generateMipmapForInputTexture(inputIndex);
        itAdvanced = true;
    }

    int useShader = 0;
    if (fr.readSequence("useShader", useShader))
    { 
        unit.setUseShader(useShader?true:false);
        itAdvanced = true;
    }

    return itAdvanced;
}

//--------------------------------------------------------------------------
bool readUnitText(osg::Object& obj, osgDB::Input& fr)
{
    // convert given object to unit 
    osgPPU::UnitText& unit = static_cast<osgPPU::UnitText&>(obj);

    bool itAdvanced = false;

    int size = 0;
    if (fr.readSequence("size", size))
    { 
        unit.setSize(size);
        itAdvanced = true;
    }

    osgText::Text* text = static_cast<osgText::Text*>(fr.readObjectOfType(osgDB::type_wrapper<osgText::Text>()));
    if (text)
    {
        unit.setText(text);
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

    int isActive = 0;
    if (fr.readSequence("isActive", isActive))
    { 
        unit.setActive(isActive?true:false);
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
        if (Texture_matchInternalFormatStr(internalFormatStr.c_str(), fmt) != 0)
        {
            unit.setOutputInternalFormat(fmt);
        }else{
            osg::notify(osg::WARN)<<"Unit " << unit.getName() << " cannot read output internal format \""<< internalFormatStr << "\"." << std::endl;
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

    // read color attribute
    osgPPU::ColorAttribute* ca = static_cast<osgPPU::ColorAttribute*>(fr.readObjectOfType(osgDB::type_wrapper<osgPPU::ColorAttribute>()));
    if (ca)
    {
        unit.setColorAttribute(ca);
        itAdvanced = true;
    }

    // read all inputs
    if (fr.matchSequence("PPUOutput {"))
    {
        int entry = fr[0].getNoNestedBrackets();

        fr += 2;

        while (!fr.eof() && fr[0].getNoNestedBrackets()>entry)
        {
            std::string ppuid;

            // input is a ppu
            if (fr[0].matchWord("PPU"))
            {
                // get ppu name
                ppuid = fr[1].getStr();

                // reader should contain the list options
                ListReadOptions* opt = dynamic_cast<ListReadOptions*>(const_cast<osgDB::ReaderWriter::Options*>(fr.getOptions()));
                if (opt)
                {
                    ListReadOptions::List& list = opt->getList();
                    list[&unit].push_back(ppuid);
                }else{
                    osg::notify(osg::WARN)<<"osgPPU::readObject - Something bad happens!" << std::endl;    
                }
            }
            ++fr;
        }
        
        // skip trailing '}'
        ++fr;
        
        itAdvanced = true;
    }

    // read all inputs
    if (fr.matchSequence("IgnoreInput {"))
    {
        int entry = fr[0].getNoNestedBrackets();

        fr += 2;

        while (!fr.eof() && fr[0].getNoNestedBrackets()>entry)
        {
            unsigned int index = -1;
            if (fr[0].getUInt(index))
            {
                ++fr;
                unit.setIgnoreInput(index);
            }else{
                osg::notify(osg::FATAL) << "osgPPU::ReaderWriter::readUnit() - syntax error in IgnoreInput field" << std::endl;
                break;
            }
        }
        
        // skip trailing '}'
        ++fr;
        
        itAdvanced = true;
    }
    
    // input to uniform map
    if (fr.matchSequence("InputToUniformMap {"))
    {
        int entry = fr[0].getNoNestedBrackets();

        fr += 2;

        std::map<std::string, std::string> inputMap;
        
        while (!fr.eof() && fr[0].getNoNestedBrackets()>entry)
        {
            // get names
            inputMap[fr[0].getStr()] = fr[1].getStr();
            fr += 2;         
        }
        
        // add element
        ListReadOptions* opt = dynamic_cast<ListReadOptions*>(const_cast<osgDB::ReaderWriter::Options*>(fr.getOptions()));
        if (opt)
        {
            ListReadOptions::UniformInputMap& map = opt->getUniformInputMap();
            map[&unit] = inputMap;
        }else{
            osg::notify(osg::WARN)<<"osgPPU::readObject - Something bad happens!" << std::endl;
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
bool writeColorAttribute(const osg::Object& obj, osgDB::Output& fout)
{
    // convert given object to unit 
    const osgPPU::ColorAttribute& ca = static_cast<const osgPPU::ColorAttribute&>(obj);

    fout.indent() << "startTime " << ca.getStartTime() << std::endl;
    fout.indent() << "endTime " << ca.getEndTime() << std::endl;
    fout.indent() << "startColor " << ca.getStartColor() << std::endl;
    fout.indent() << "endColor " << ca.getEndColor() << std::endl;

    return true;
}

//--------------------------------------------------------------------------
bool writeUnit(const osg::Object& obj, osgDB::Output& fout)
{
    // convert given object to unit 
    const osgPPU::Unit& unit = static_cast<const osgPPU::Unit&>(obj);

    // since we do not directlz reference to other ppu's but to its textures
    // it can happen that the unique id will not be written, hence force it here
    std::string uid;
    if (!fout.getUniqueIDForObject(&obj, uid))
    {
        fout.createUniqueIDForObject(&obj, uid);
        fout.registerUniqueIDForObject(&obj, uid);
    }
    fout.writeUniqueID(uid);

    // retrieve default parameters and sotre them
    fout.indent() << "name " <<  fout.wrapString(unit.getName()) << std::endl;
    fout.indent() << "isActive " <<  unit.getActive() << std::endl;
    fout.indent() << "inputTextureIndexForViewportReference " <<  unit.getInputTextureIndexForViewportReference() << std::endl;
    
    // write internal format
    {
        const char* str = Texture_getInternalFormatStr(unit.getOutputInternalFormat());
        
        if (str) fout.indent() << "outputInternalFormat " << str << std::endl;
        else fout.indent() << "outputInternalFormat " << unit.getOutputInternalFormat() << std::endl;
    }

    // write ignore input indices
    {
        const std::vector<unsigned int>& index = unit.getIgnoreInputList();
        if (index.size())
        {
            fout << std::endl;
            fout.writeBeginObject("IgnoreInput");
            fout.moveIn();

            for (unsigned int i=0; i < index.size(); i++)
            {
                fout.indent() << index[i] << std::endl;    
            }

            fout.moveOut();
            fout.writeEndObject();
        }
    }

    // write linkage between input and uniform names
    {
        const osgPPU::Unit::InputToUniformMap& map = unit.getInputToUniformMap();
        if (map.size())
        {
            fout << std::endl;
            fout.writeBeginObject("InputToUniformMap");
            fout.moveIn();

            for (osgPPU::Unit::InputToUniformMap::const_iterator it = map.begin(); it != map.end(); it++)
            {
                // get uid for the input
                std::string uid;
                if (!fout.getUniqueIDForObject(it->first.get(), uid))
                {
                    fout.createUniqueIDForObject(it->first.get(), uid);
                    fout.registerUniqueIDForObject(it->first.get(), uid);
                }
                fout.indent() << uid << " " << it->second.first << std::endl;
            }
            
            fout.moveOut();
            fout.writeEndObject();
        }
    }
    
    // for non bypass ppus do specify inputs
    fout << std::endl;
    fout.writeBeginObject("PPUOutput");
    fout.moveIn();

        // for each parent node do
        for (unsigned int i=0; i < unit.getNumChildren(); i++)
        {
            const osgPPU::BarrierNode* barrier = dynamic_cast<const osgPPU::BarrierNode*>(unit.getChild(i));

            // if it is a unit
            if (dynamic_cast<const osgPPU::Unit*>(unit.getChild(i)))
            {

                std::string uid;
                if (!fout.getUniqueIDForObject(unit.getChild(i), uid))
                {
                    fout.createUniqueIDForObject(unit.getChild(i), uid);
                    fout.registerUniqueIDForObject(unit.getChild(i), uid);
                }

                fout.indent() << "PPU " << uid << std::endl;

            // if it is a barrier node
            }else if (barrier)
            {
                std::string uid;
                if (!fout.getUniqueIDForObject(barrier->getBlockedChild(), uid))
                {
                    fout.createUniqueIDForObject(barrier->getBlockedChild(), uid);
                    fout.registerUniqueIDForObject(barrier->getBlockedChild(), uid);
                }

                fout.indent() << "PPU " << uid << std::endl;
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

    // write the color attribute
    if (unit.getColorAttribute())
    {
        fout << std::endl;
        osgDB::Registry::instance()->writeObject(static_cast<const osgPPU::ColorAttribute&>(*(unit.getColorAttribute())), fout);
    }
    
    return true;
}

//--------------------------------------------------------------------------
bool writeUnitTexture(const osg::Object& obj, osgDB::Output& fout)
{
    // convert given object to unit 
    const osgPPU::UnitTexture& unit = static_cast<const osgPPU::UnitTexture&>(obj);

    osgDB::Registry::instance()->writeObject(
        (static_cast<const osg::Texture&>(*(const_cast<osgPPU::UnitTexture&>(unit).getTexture()))), fout);

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

    fout.indent() << "inputBypass " <<  unit.getInputBypass() << std::endl;

    return true;
}

//--------------------------------------------------------------------------
bool writeUnitMipmapInMipmapOut(const osg::Object& obj, osgDB::Output& fout)
{
    return true;
}

//--------------------------------------------------------------------------
bool writeUnitInMipmapOut(const osg::Object& obj, osgDB::Output& fout)
{
    // convert given object to unit 
    const osgPPU::UnitInMipmapOut& unit = static_cast<const osgPPU::UnitInMipmapOut&>(obj);

    fout.indent() << "inputIndex " <<  unit.getGenerateMipmapForInputTextureIndex() << std::endl;
    fout.indent() << "useShader " << unit.getUseShader() << std::endl;

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

    // write out shader program
    osgDB::Registry::instance()->writeObject(unit.getText(), fout);

    return true;
}


// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_ColorAttributeProxy
(
    new osgPPU::ColorAttribute,
    "ColorAttribute",
    "Object StateAttribute ColorAttribute",
    &readColorAttribute,
    &writeColorAttribute
);

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
osgDB::RegisterDotOsgWrapperProxy g_UnitBypassProxy
(
    new osgPPU::UnitBypass,
    "UnitBypass",
    "UnitBypass",
    &readUnit,
    &writeUnit
);

// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_UnitDepthbufferBypassProxy
(
    new osgPPU::UnitDepthbufferBypass,
    "UnitDepthbufferBypass",
    "UnitDepthbufferBypass",
    &readUnit,
    &writeUnit
);

// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_UnitTextureProxy
(
    new osgPPU::UnitTexture,
    "UnitTexture",
    "Unit UnitTexture",
    &readUnitTexture,
    &writeUnitTexture
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
osgDB::RegisterDotOsgWrapperProxy g_UnitInMipmapOutProxy
(
    new osgPPU::UnitInMipmapOut,
    "UnitInMipmapOut",
    "Unit UnitInOut UnitInMipmapOut",
    &readUnitInMipmapOut,
    &writeUnitInMipmapOut
);

// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_UnitMipmapInMipmapOutProxy
(
    new osgPPU::UnitMipmapInMipmapOut,
    "UnitMipmapInMipmapOut",
    "Unit UnitInOut UnitMipmapInMipmapOut",
    &readUnitMipmapInMipmapOut,
    &writeUnitMipmapInMipmapOut
);

// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_UnitTextProxy
(
    new osgPPU::UnitText,
    "UnitText",
    "Unit UnitInOut UnitText",
    &readUnitText,
    &writeUnitText
);

