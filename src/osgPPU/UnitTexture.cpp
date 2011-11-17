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

#include <osgPPU/UnitTexture.h>
#include <osg/Geode>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture2DArray>
#include <osg/Texture3D>
#include <osg/TextureCubeMap>
#include <osg/TextureRectangle>

namespace osgPPU
{
    //------------------------------------------------------------------------------
    UnitTexture::UnitTexture() : UnitBypass()
    {
    }

    //------------------------------------------------------------------------------
    UnitTexture::UnitTexture(const UnitTexture& u, const osg::CopyOp& copyop) : 
        UnitBypass(u, copyop),
        _externTexture(u._externTexture)
    {
    }
    
    //------------------------------------------------------------------------------
    UnitTexture::UnitTexture(osg::Texture* tex)
    {
        setTexture(tex);
    }
    
    //------------------------------------------------------------------------------
    UnitTexture::~UnitTexture()
    {

    }

    //------------------------------------------------------------------------------
    void UnitTexture::setupInputsFromParents()
    {
        mInputTex[0] = _externTexture;
        mOutputTex = mInputTex;
    }


    //------------------------------------------------------------------------------
    void UnitTexture::setTexture(osg::Texture* tex)
    {
        if (tex == getTexture() || tex == NULL) return;

        _externTexture = tex;
        dirty();

        // force texture size, helps if texture was not loaded before
        if (!_externTexture->areAllTextureObjectsLoaded() && _externTexture->getImage(0))
        {
            osg::Texture1D* tex1D = dynamic_cast<osg::Texture1D*>(_externTexture.get());
            osg::Texture2D* tex2D = dynamic_cast<osg::Texture2D*>(_externTexture.get());
            osg::Texture2DArray* tex2DArray = dynamic_cast<osg::Texture2DArray*>(_externTexture.get());
            osg::Texture3D* tex3D = dynamic_cast<osg::Texture3D*>(_externTexture.get());
            osg::TextureCubeMap* texCube = dynamic_cast<osg::TextureCubeMap*>(_externTexture.get());
            osg::TextureRectangle* texRectangle = dynamic_cast<osg::TextureRectangle*>(_externTexture.get());

            if (tex1D)
            {
                tex1D->setTextureWidth(_externTexture->getImage(0)->s());
            }else if (tex2D)
            {
                tex2D->setTextureSize(_externTexture->getImage(0)->s(), _externTexture->getImage(0)->t());
            }else if (tex2DArray)
            {
                tex2DArray->setTextureSize(_externTexture->getImage(0)->s(), _externTexture->getImage(0)->t(), _externTexture->getImage(0)->r());
            }else if (tex3D)
            {
                tex3D->setTextureSize(_externTexture->getImage(0)->s(), _externTexture->getImage(0)->t(), _externTexture->getImage(0)->r());
            }else if (texCube)
            {
                texCube->setTextureSize(_externTexture->getImage(0)->s(), _externTexture->getImage(0)->t());
            }else if (texRectangle)
            {
                texRectangle->setTextureSize(_externTexture->getImage(0)->s(), _externTexture->getImage(0)->t());
            }else
            {
                osg::notify(osg::FATAL) << "osgPPU::UnitTexture::setTexture() - " << getName() << " non-supported texture type specified!" << std::endl;
            }
        }

        // if no viewport, so create it
        if (mViewport == NULL)
            mViewport = new osg::Viewport(0,0,0,0);
        
        // change viewport sizes
        mViewport->width() = (osg::Viewport::value_type)_externTexture->getTextureWidth();
        mViewport->height() = (osg::Viewport::value_type)_externTexture->getTextureHeight();
        
        // just notice that the viewport size has changed
        noticeChangeViewport(mViewport);
    }


}; // end namespace
