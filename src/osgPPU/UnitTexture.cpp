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

namespace osgPPU
{
    //------------------------------------------------------------------------------
    // Helper class to allow initialization of the input texture
    //------------------------------------------------------------------------------
    class UnitTexture::DrawCallback : public osg::Drawable::DrawCallback
    {   
        public:
            DrawCallback(UnitTexture* parent) : osg::Drawable::DrawCallback(), _parent(parent) {}
            ~DrawCallback() {}

            inline void drawImplementation (osg::RenderInfo& ri, const osg::Drawable* dr) const
            {
                // apply the texture once, so that it get loaded
                ri.getState()->applyTextureAttribute(0, _parent->getTexture());

                // remove this callback and the drawable out of the unit
                _parent->getGeode()->removeDrawable(const_cast<osg::Drawable*>(dr));
            }

            mutable UnitTexture* _parent;
    };

    //------------------------------------------------------------------------------
    UnitTexture::UnitTexture() : Unit()
    {
        //setOfflineMode(true);
    }

    //------------------------------------------------------------------------------
    UnitTexture::UnitTexture(const UnitTexture& u, const osg::CopyOp& copyop) : 
        Unit(u, copyop)
    {
    }
    
    //------------------------------------------------------------------------------
    UnitTexture::UnitTexture(osg::Texture* tex)
    {
        setTexture(tex);
        //setOfflineMode(true);
    }
    
    //------------------------------------------------------------------------------
    UnitTexture::~UnitTexture()
    {

    }


    //------------------------------------------------------------------------------
    void UnitTexture::init()
    {
        // add a drawable with our own callback
        mDrawable = createTexturedQuadDrawable();
        mDrawable->setDrawCallback(new UnitTexture::DrawCallback(this));
        mGeode->removeDrawables(0, mGeode->getNumDrawables());
        mGeode->addDrawable(mDrawable.get());
        
        // check if we have input reference size 
        if (getInputTextureIndexForViewportReference() >= 0 && getInputTexture(getInputTextureIndexForViewportReference()))
        {
            // if no viewport, so create it
            if (!mViewport.valid())
                mViewport = new osg::Viewport(0,0,0,0);
            
            // change viewport sizes
            mViewport->width() = getInputTexture(getInputTextureIndexForViewportReference())->getTextureWidth();
            mViewport->height() = getInputTexture(getInputTextureIndexForViewportReference())->getTextureHeight();
    
            // just notice that the viewport size is changed
            noticeChangeViewport();
        }
    
        // reassign input and shaders
        assignInputTexture();
        assignViewport();
    }

    //------------------------------------------------------------------------------
    void UnitTexture::setTexture(osg::Texture* tex)
    {
        if (tex == getTexture()) return;

        mInputTex.clear();
        mOutputTex.clear();

        mInputTex[0] = tex;
        mOutputTex[0] = tex;

        dirty();
    }


}; // end namespace
