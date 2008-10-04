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

#include <osgPPU/UnitText.h>
#include <osg/Geode>

namespace osgPPU
{
    //------------------------------------------------------------------------------
    UnitText::UnitText(const UnitText& unit, const osg::CopyOp& copyop) :
        UnitInOut(unit, copyop),
        mText(unit.mText),
        mSize(unit.mSize)
    {
    }

    //------------------------------------------------------------------------------
    UnitText::UnitText() : osgPPU::UnitInOut()
    {
        mSize = 26.0;

        // initialize text
        mText = new osgText::Text();
        mText->setFont();
        mText->setColor(osg::Vec4(1,1,1,1));
        mText->setAxisAlignment(osgText::Text::SCREEN);

        // setup some defaults parameters
        mText->setLayout(osgText::Text::LEFT_TO_RIGHT);
        mText->setCharacterSizeMode(osgText::Text::SCREEN_COORDS);

        // setup stateset so that the text is rendered in unit's renderbin
        mText->getOrCreateStateSet()->setRenderBinToInherit();
    }

    //------------------------------------------------------------------------------
    UnitText::~UnitText()
    {

    }

    //------------------------------------------------------------------------------
    void UnitText::setText(osgText::Text* text)
    {
        mText = text;
        dirty();
    }

    //------------------------------------------------------------------------------
    void UnitText::init()
    {
        // init inout ppu
        UnitInOut::init();

        // add text as drawable
        mGeode->removeDrawables(0, mGeode->getNumDrawables());
        mGeode->addDrawable(mText.get());

        // setup new draw callback for the text
        mText->setDrawCallback(new Unit::DrawCallback(this));

        // we take the width 640 as reference width for the size of characters
        mText->setCharacterSize(mSize * (float(getViewport()->width()) / 640.0), 1.0);

        // text is the same as bypass, hence do bypass here
        setOutputTextureMap(getInputTextureMap());

        // assign fbo
        assignFBO();

    }

}; // end namespace

