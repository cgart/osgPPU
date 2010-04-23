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
#include <osg/OperationThread>

namespace osgPPU
{
	//-------------------------------------------------------------------------
	// Helper operation to release modules when graphics thread is released
	//-------------------------------------------------------------------------
	struct ModuleReleaseOperation : public osg::Operation
	{
		UnitInOutModule* _parent;

		ModuleReleaseOperation(UnitInOutModule* parent) : osg::Operation("__UnitInOutModule_ReleaseOperation", true), _parent(parent) {}

		void release() { _parent->removeModule(); }
		void operator() (osg::Object *) {}
	};

	//-------------------------------------------------------------------------
	// Helper class to release module 
	//-------------------------------------------------------------------------
	struct DummyDrawable : public osg::Drawable
	{
		UnitInOutModule* _parent;

		META_Object(osgPPU, DummyDrawable);

		DummyDrawable(){};
		DummyDrawable(const DummyDrawable& drawable, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY){}
		DummyDrawable(UnitInOutModule* parent) : osg::Drawable(), _parent(parent){setUseDisplayList(false);}

		void drawImplementation (osg::RenderInfo& renderInfo) const{}
		void releaseGLObjects (osg::State* state)const
		{
			if (state) state->getGraphicsContext()->makeCurrent();
			_parent->removeModule();
		}
	};

    //-------------------------------------------------------------------------
    UnitInOutModule::UnitInOutModule() : UnitInOut(),
        _moduleDirty(false),
        _module(NULL)
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
    }

    //-------------------------------------------------------------------------
    bool UnitInOutModule::loadModule(const std::string& moduleFile)
    {
        _moduleLib = osgDB::DynamicLibrary::loadLibrary(moduleFile);

        if (!_moduleLib.valid())
        {
            //osg::notify(osg::FATAL) << "osgPPU::UnitInOutModule - cannot load module from " << moduleFile << std::endl;
            return false;
        }

        // try to find the main process
        OSGPPU_MODULE_ENTRY entry = (OSGPPU_MODULE_ENTRY)_moduleLib->getProcAddress(OSGPPU_MODULE_ENTRY_STR);
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
	void UnitInOutModule::init()
	{
		UnitInOut::init();

		// add a dummy drawable to be able to release the module in the graphic context
		mGeode->addDrawable(new DummyDrawable(this));
	}

    //-------------------------------------------------------------------------
    void UnitInOutModule::setModule(Module* module)
    {
        if (module == NULL || module == _module) return;

        _module = module;
        _moduleDirty = true;
    }

    //-------------------------------------------------------------------------
    void UnitInOutModule::removeModule()
    {
		if (_moduleLib)
		{
			OSGPPU_MODULE_RELEASE exitf = (OSGPPU_MODULE_RELEASE)_moduleLib->getProcAddress(OSGPPU_MODULE_RELEASE_STR);
			if (exitf) exitf();
		}

		_moduleLib = NULL;
        _module = NULL;
    }

    //-------------------------------------------------------------------------
    bool UnitInOutModule::noticeBeginRendering(osg::RenderInfo& ri, const osg::Drawable*)
    {
        if (!_module || !_moduleLib) return false;
        if (_moduleDirty)
		{
			// init module first
			if (_module->init() == false)
			{
				removeModule();
				return false;
			}

			// create an operation which will release the module on release of the graphics thread
			ModuleReleaseOperation* op = new ModuleReleaseOperation(this);
			if (ri.getState()->getGraphicsContext()->getGraphicsThread())
				ri.getState()->getGraphicsContext()->getGraphicsThread()->add(op);

			_moduleDirty = false;
		}

        return _module->beginAndProcess();
    }

    //-------------------------------------------------------------------------
    void UnitInOutModule::noticeFinishRendering(osg::RenderInfo& ri, const osg::Drawable*)
    {
        if (!_module) return;
        _module->beginAndProcess();
        _module->end();
    }

 
}

