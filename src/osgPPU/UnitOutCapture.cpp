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
#include <osgPPU/UnitOutCapture.h>

#include <osg/Texture2D>
#include <osgDB/WriteFile>
#include <osgDB/Registry>

#include <iostream>

namespace osgPPU
{
    //------------------------------------------------------------------------------
    UnitOutCapture::UnitOutCapture(const UnitOutCapture& unit, const osg::CopyOp& copyop) :
        UnitOut(unit, copyop)
    {
    
    }
    //------------------------------------------------------------------------------
    UnitOutCapture::UnitOutCapture() : UnitOut()
    {
        mPath = ".";
        mCaptureNumber = 0;
        mExtension = "png";
    }
    
    //------------------------------------------------------------------------------
    UnitOutCapture::~UnitOutCapture()
    {
    }
    
    
    //------------------------------------------------------------------------------
    void UnitOutCapture::noticeFinishRendering(osg::RenderInfo &renderInfo, const osg::Drawable* drawable)
    {
        if (getActive() && renderInfo.getState())
        {
            // if we want to capture the framebuffer
            char filename[256];
            
            // for each input texture do
            for (unsigned int i=0; i < mInputTex.size(); i++)
            {
                // create file name
                sprintf( filename, "%s/%d_%04d.%s", mPath.c_str(), i, mCaptureNumber, mExtension.c_str());
                std::cout << "Capture " << mCaptureNumber << " frame to " << filename << " ...";
                std::cout.flush();
            
                mCaptureNumber++;
    
                // input texture 
                osg::Texture* input = getInputTexture(i);
    
                // bind input texture, so that we can get image from it
                if (input != NULL) input->apply(*renderInfo.getState());
                
                // retrieve texture content
                osg::ref_ptr<osg::Image> img = new osg::Image();
                img->readImageFromCurrentTexture(renderInfo.getContextID(), false);
                //img->readPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE);
                osgDB::ReaderWriter::WriteResult res = osgDB::Registry::instance()->writeImage(*img, filename, NULL);
                //write_png(filename, img->data(), w, h, 4, 8, PNG_COLOR_TYPE_RGBA, 1);
                if (res.success())
                    std::cout << " OK" << std::endl;
                else
                    std::cout << " failed! (" << res.message() << ")" << std::endl;
                            
                // unbind the texture back 
                if (input != NULL)
                    renderInfo.getState()->applyTextureMode(0, input->getTextureTarget(), false);
            }
        }
        UnitOut::noticeFinishRendering(renderInfo, drawable);
    }

}; // end namespace
