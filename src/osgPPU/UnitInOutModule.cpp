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

#include <osgPPU/UnitInOutModule.h>

#include <osg/Texture2D>
#include <osg/TextureCubeMap>

namespace osgPPU
{

    //-------------------------------------------------------------------------
    UnitInOutModule::UnitInOutModule() : UnitInOut(),
        _moduleDirty(false)
    {
    }

    //-------------------------------------------------------------------------
    UnitInOutModule::UnitInOutModule(const UnitInOutModule& unit, const osg::CopyOp& copyop) : 
        UnitInOut(unit, copyop),
        _moduleDirty(unit._moduleDirty),
        _module(unit._module),
        _moduleLib(unit._moduleLib)
    {

    }
    
    //-------------------------------------------------------------------------
    UnitInOutModule::~UnitInOutModule()
    {
        // first remove the module and then close the dynamic library
        removeModule();
        _moduleLib = NULL;
    }

    //-------------------------------------------------------------------------
    bool UnitInOutModule::loadModule(const std::string& moduleFile)
    {
        _moduleLib = osgDB::DynamicLibrary::loadLibrary(moduleFile);

        if (!_moduleLib.valid())
        {
            osg::notify(osg::FATAL) << "osgPPU::UnitInOutModule - cannot load module from " << moduleFile << std::endl;
            return false;
        }

        // try to find the main process
        osgppuInitModule entry = (osgppuInitModule)_moduleLib->getProcAddress(OSGPPU_MODULE_ENTRY_STR);
        if (entry == NULL)
        {
            osg::notify(osg::FATAL) << "osgPPU::UnitInOutModule - no entry point " << OSGPPU_MODULE_ENTRY_STR << " in the module " << moduleFile << " was found" << std::endl;
            return false;
        }
        _moduleFile = moduleFile;

        // call the entry point, so that the module register himself by this unit
        return entry(this);
    }

    //-------------------------------------------------------------------------
    void UnitInOutModule::setModule(Module* module)
    {
        if (module == NULL) return;
        if (module == _module.get()) return;

        _module = module;
        if (_module->init() == false)
            removeModule();
        _moduleDirty = false;
    }

    //-------------------------------------------------------------------------
    void UnitInOutModule::removeModule()
    {
        _module = NULL;
    }

    //-------------------------------------------------------------------------
    bool UnitInOutModule::noticeBeginRendering(osg::RenderInfo& ri, const osg::Drawable*)
    {
        if (!_module.get()) return false;
        if (_moduleDirty) setModule(_module.get());

        return _module->beginAndProcess();
    }

    //-------------------------------------------------------------------------
    void UnitInOutModule::noticeFinishRendering(osg::RenderInfo& ri, const osg::Drawable*)
    {
        if (!_module.get()) return;
        _module->end();
    }

 
}

