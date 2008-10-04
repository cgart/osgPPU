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

#include <osgPPU/UnitCameraAttachmentBypass.h>
#include <osgPPU/Processor.h>
#include <osgPPU/BarrierNode.h>

namespace osgPPU
{

    //------------------------------------------------------------------------------
    UnitCameraAttachmentBypass::UnitCameraAttachmentBypass() : UnitBypass()
    {
        _bufferComponent = osg::Camera::COLOR_BUFFER;
    }

    //------------------------------------------------------------------------------
    UnitCameraAttachmentBypass::UnitCameraAttachmentBypass(const UnitCameraAttachmentBypass& u, const osg::CopyOp& copyop) :
        UnitBypass(u, copyop),
        _bufferComponent(u._bufferComponent)
    {

    }

    //------------------------------------------------------------------------------
    UnitCameraAttachmentBypass::~UnitCameraAttachmentBypass()
    {

    }

    //------------------------------------------------------------------------------
    void UnitCameraAttachmentBypass::setBufferComponent(osg::Camera::BufferComponent c)
    {
        _bufferComponent = c;
        dirty();
    }

    //------------------------------------------------------------------------------
    void UnitCameraAttachmentBypass::init()
    {
        UnitBypass::init();
    }

    //------------------------------------------------------------------------------
    void UnitCameraAttachmentBypass::setupInputsFromParents()
    {
        // scan all parents and look for the processor
        Processor* proc = NULL;
        for (unsigned int i=0; i < getNumParents(); i++)
        {
            proc = dynamic_cast<Processor*>(getParent(i));
            if (proc) break;
        }
        if (proc)
        {
            // get depthbuffer attachment
            osg::Camera::BufferAttachmentMap& map = proc->getCamera()->getBufferAttachmentMap();
            osg::Texture* input = map[_bufferComponent]._texture.get();

            // if no such attachment then warning
            if (!input)
            {
                osg::notify(osg::WARN) << "osgPPU::UnitCameraAttachmentBypass::setupInputsFromParents() - processor's camera has no specified buffer attachment" << std::endl;
            }else
            {
                // set the input texture
                mInputTex.clear();
                mInputTex[0] = input;
                noticeChangeInput();

                // the viewport should be retrieved from the input texture
                setInputTextureIndexForViewportReference(0);
                mOutputTex = mInputTex;
            }
        }else
        {
            osg::notify(osg::WARN) << "osgPPU::UnitCameraAttachmentBypass::setupInputsFromParents() - unit is not a direct child of processor" << std::endl;
        }

        // setup eventually blocked children
        setupBlockedChildren();
    }

}; // end namespace
