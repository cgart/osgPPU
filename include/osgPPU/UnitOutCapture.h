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

#ifndef _C_UNIT_CAPTURE__H_
#define _C_UNIT_CAPTURE__H_


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgPPU/Export.h>
#include <osgPPU/UnitOut.h>

namespace osgPPU
{
    //! Capture the content of the input texture to a file
    /**
    * Screen capturing ppu. The input texture is captured into a file.
    * This ppu allows to render out in higher resolution than your
    * monitor supports. This can be only achieved if your rendering
    * is going completely through ppu pipeline, so renderer in offscreen mode.
    **/
    class OSGPPU_EXPORT UnitOutCapture : public UnitOut {
        public:
            META_Node(osgPPU,UnitOutCapture);
        
            //! Create default ppfx 
            UnitOutCapture();
            UnitOutCapture(const UnitOutCapture&, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
            
            //! Release it and used memory
            virtual ~UnitOutCapture();
            
            //! Set path were to store the screenshots
            void setPath(const std::string& path) { mPath = path; }
    
            //! set extension   
            void setFileExtension(const std::string& ext) { mExtension = ext; }
    
            //! get currently used path 
            inline const std::string& getPath() const { return mPath; }
    
            //! get currently used extension 
            inline const std::string& getFileExtension() const { return mExtension; }
    
        protected:
            
            //! Here we are capturing the input to file
            void noticeFinishRendering(osg::RenderInfo &renderInfo, const osg::Drawable*);
        
            //! path were to store the files
            std::string mPath;
    
            //! Current number of the capture file 
            std::map<int,int> mCaptureNumber;
    
            //! file extensions
            std::string mExtension;
    };

};

#endif
