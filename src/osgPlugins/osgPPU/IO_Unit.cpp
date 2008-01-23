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
#ifndef _IO_UNIT_H_
#define _IO_UNIT_H_

#include <osgPPU/Processor.h>
#include <osgPPU/Unit.h>
#include <osgPPU/PPUInOut.h>
#include <osgPPU/PPUText.h>

#include <osg/Notify>

#include "Base.h"

// forward declare functions to use later.
bool Unit_readLocalData(osg::Object& obj, osgDB::Input& fr){return false;}
bool UnitOut_readLocalData(osg::Object& obj, osgDB::Input& fr){return false;}
bool UnitOutCapture_readLocalData(osg::Object& obj, osgDB::Input& fr){return false;}
bool UnitInOut_readLocalData(osg::Object& obj, osgDB::Input& fr){return false;}
bool UnitInResampleOut_readLocalData(osg::Object& obj, osgDB::Input& fr){return false;}
bool UnitText_readLocalData(osg::Object& obj, osgDB::Input& fr){return false;}


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

    // write out shader program
    osgDB::Registry::instance()->writeObject(static_cast<const osg::Program&>(*(sh->getProgram())), fout);
}


//--------------------------------------------------------------------------
bool writeUnit(const osg::Object& obj, osgDB::Output& fout)
{
    // convert given object to unit 
    const osgPPU::Unit& unit = static_cast<const osgPPU::Unit&>(obj);

    // the list of input ppus must be passed as option 
    const ListOptions::List& ilist = dynamic_cast<const ListOptions*>(fout.getOptions())->getList();

    // retrieve default parameters and sotre them
    fout.indent() << "Index " <<  unit.getIndex() << std::endl;
    fout.indent() << "BlendEndTime " <<  unit.getEndBlendTime() << std::endl;
    fout.indent() << "BlendStartTime " <<  unit.getStartBlendTime() << std::endl;
    fout.indent() << "BlendStartValue " <<  unit.getStartBlendValue() << std::endl;
    fout.indent() << "BlendEndValue " <<  unit.getEndBlendValue() << std::endl;
    fout.indent() << "BlendEnabled " <<  unit.useBlendMode() << std::endl;
    fout.indent() << "isActive " <<  unit.isActive() << std::endl;
    fout.indent() << "isOffline " <<  unit.getOfflineMode() << std::endl;
    fout.indent() << "inputTextureIndexForViewportReference " <<  unit.getInputTextureIndexForViewportReference() << std::endl;
    fout.indent() << "useMipmapShader " << unit.getUseMipmapShader() << std::endl;
    fout.indent() << "useMipmaps " << unit.getUseMipmaps() << std::endl;

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

        ListOptions::List::const_iterator it = ilist.begin();
        for (int index = 0; it != ilist.end(); it++, index++)
        {
            // input texture is there but no according ppu, hence external input
            if (it->first == NULL && it->second == NULL)
            {
                osg::notify(osg::WARN)<<"Unit " << unit.getName() << " has incorrect inputs." << std::endl;    
                fout.indent() << "PPU " << fout.wrapString(it->first->getName()) << std::endl;
            }else if (it->first == NULL && it->second != NULL)
            {
                //osg::notify(osg::WARN)<<"Unit " << unit.getName() << " has as input an external texture." << std::endl;
                osgDB::Registry::instance()->writeObject(static_cast<const osg::Texture&>(*(it->second)), fout);
            }else
            {
                // check if just to bypass or not 
                //if (&unit == it->first && !strcmp(unit.className(), "Unit"))
                //    fout.indent() << "Bypass" << std::endl;  
                //else
                fout.indent() << "PPU " << fout.wrapString(it->first->getName()) << std::endl;
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

    if (unit.getMipmapShader())
    {
        fout << std::endl;
        fout.writeBeginObject("MipmapShader");
        fout.moveIn();
            writeShader(unit.getMipmapShader(), fout);
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

    fout.indent() << "isMipmappedIO " <<  unit.getMipmappedIO() << std::endl;

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
    new osgPPU::Unit,
    "Unit",
    "Unit Object",
    &Unit_readLocalData,
    &writeUnit
);

// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_UnitOutProxy
(
    new osgPPU::UnitOut,
    "UnitOut",
    "UnitOut Object",
    &UnitOut_readLocalData,
    &writeUnit
);

// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_UnitOutCaptureProxy
(
    new osgPPU::UnitOutCapture,
    "UnitOutCapture",
    "Unit UnitOut UnitOutCapture oObject",
    &UnitOutCapture_readLocalData,
    &writeUnitOutCapture
);

// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_UnitInOutProxy
(
    new osgPPU::UnitInOut,
    "UnitInOut",
    "Unit UnitInOut Object",
    &UnitInOut_readLocalData,
    &writeUnitInOut
);

// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_UnitInResampleOutProxy
(
    new osgPPU::UnitInResampleOut,
    "UnitInResampleOut",
    "Unit UnitInOut UnitInResampleOut Object",
    &UnitInResampleOut_readLocalData,
    &writeUnitInResampleOut
);

// register the read and write functions with the osgDB::Registry.
osgDB::RegisterDotOsgWrapperProxy g_UnitTextProxy
(
    static_cast<osg::Object*>(static_cast<osgPPU::Unit*>(new osgPPU::UnitText)),
    "UnitText",
    "Unit UnitInOut UnitText Object",

    // HACK: must be the line below, but produce some segfaults in the osgdb_osgtext.so
    //"Unit UnitInOut UnitText osgText::Text Object",
    &UnitText_readLocalData,
    &writeUnitText
);


#endif

