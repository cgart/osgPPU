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
    
            /**
            * Set if the output should be generated only once.
            * The unit will be activated only for one frame. 
            * You will need to reactivate the unit, if you would like to have continous frame capturing
            * after one frame shot.
            **/
            inline void setShotOnce(bool b) { mShotOnce = b; if (b) setActive(true); }

            //! Check if the unit will shot once on the next traversion
            inline bool getShotOnce() { return mShotOnce; }

            //! Direct function, which can be used to take a screenshot.
            virtual void captureInput(osg::State* state);

            //! Initialze the default Processoring unit
            virtual void init();

			/**
			* Specify whenever the pixel format of the image to store should be the same as the internal format
			* of input texture. This might be helpful if you want to store directly as it is, without converting.
			* However if you want for example store a float texture to JPG files, you might need to disable
			* this feature allowing the plugin to handle the texture data.
			* 
			* Per default this feature is disabled.
			**/
			inline void setUseInputTextureInternalFormat(bool use) { mUseInputTextureInternalFormat = use; }

			//! Check whenever internal format of input texture is used as pixel format for storing
			inline bool getUseInputTextureInternalFormat() const { return mUseInputTextureInternalFormat; }

        protected:
        
            //! path were to store the files
            std::string mPath;
    
            //! Current number of the capture file 
            std::map<int,int> mCaptureNumber;
    
            //! file extensions
            std::string mExtension;

			//! Do only one screenshot
            bool mShotOnce;

			//! Store with the same pixel format as used by the input textures
			bool mUseInputTextureInternalFormat;
    };

};

#endif
