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

#ifndef _C_UNIT_INRESAMPLEOUT_H_
#define _C_UNIT_INRESAMPLEOUT_H_


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgPPU/Export.h>
#include <osgPPU/UnitInOut.h>

namespace osgPPU
{
    //! Same as UnitInOut but do resampling inbetween
    /**
    * Resample the input. This PPU will 
    * render the input data resampled to the output. Next PPU will work 
    * on the resampled one. NOTE: You loose information in your data after 
    * appling this PPU.
    **/
    class OSGPPU_EXPORT UnitInResampleOut : public UnitInOut {
        public:
            META_Object(osgPPU,UnitInResampleOut);
        
            //! Create default ppfx 
            UnitInResampleOut(osg::State* state);
            UnitInResampleOut();
            UnitInResampleOut(const UnitInResampleOut&, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
            
            //! Release it and used memory
            virtual ~UnitInResampleOut();
            
            //! Set resampling factor
            void setFactorX(float x);
    
            //! Set resampling factor
            void setFactorY(float Y);
    
            //! Get resampling factor
            float getFactorX() const { return mWidthFactor; }
    
            //! Get resampling factor
            float getFactorY() const { return mHeightFactor; }
    
        protected:
            float mWidthFactor, mHeightFactor;
            bool mDirtyFactor;
    
            //! Overwritten render method
            void render(int mipmapLevel = 0);
    
    };

};

#endif
