/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osgPPU/Text.h>
#include <osg/MatrixTransform>

namespace osgPPU
{

    //-------------------------------------------------------------------------
    Text::Text()
    {
        // create text with parameters
        mText = new osgText::Text;
        mText->setLayout(osgText::Text::LEFT_TO_RIGHT);
        mText->setCharacterSizeMode(osgText::Text::SCREEN_COORDS);
        
        // create geode to hold the text    
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(mText.get());
        osg::StateSet* stateSet = geode->getOrCreateStateSet();
        stateSet->setMode(GL_LIGHTING,osg::StateAttribute::OFF);
        stateSet->setMode(GL_DEPTH_TEST,osg::StateAttribute::OFF);
        //stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        stateSet->setMode(GL_BLEND,osg::StateAttribute::ON);
        //stateSet->setRenderBinDetails(11, "RenderBin");
    
        // create projection matrix
        /*osg::ref_ptr<osg::Projection> projNode = new osg::Projection;
        projNode->setMatrix(osg::Matrix::ortho2D(0,1, 0,1));
    
        // create modelview node
        osg::ref_ptr<osg::MatrixTransform> modelView = new osg::MatrixTransform;
        modelView->setMatrix(osg::Matrix::identity());
        modelView->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    
        // now combine the things to a group node in the right manner
        mGroup= new osg::Group;
        mGroup->addChild(projNode.get());
        projNode->addChild(modelView.get());
        modelView->addChild(geode.get());*/
    
    }
    
    //-------------------------------------------------------------------------
    Text::Text(const Text& tx, const osg::CopyOp& copyop)
    {
        mText = new osgText::Text(*tx.mText, copyop);
        mGroup = new osg::Group(*tx.mGroup, copyop);
        mString = tx.mString;
        mColor = tx.mColor;
    }

    //-------------------------------------------------------------------------
    Text::~Text()
    {
        mText = NULL;
        mGroup = NULL;
    }
    
    //-------------------------------------------------------------------------
    bool Text::setFont(const std::string& fontFile)
    {
        osg::ref_ptr<osgText::Font> mFont = osgText::readFontFile(fontFile);
        if (mFont.valid()) mText->setFont(mFont.get());
        return mFont.valid();
    }
    
    //-------------------------------------------------------------------------
    void Text::setPosition(float x, float y)
    {
        mText->setPosition(osg::Vec3(x,y,0));
    }
    
    //-------------------------------------------------------------------------
    void Text::setSize(float s, float aspectRatio)
    {
        mText->setCharacterSize(s, aspectRatio);
    }
    
    //-------------------------------------------------------------------------
    void Text::setColor(const osg::Vec4& color)
    {
        mText->setColor(color);
        mColor = color;
    }
    
    //-------------------------------------------------------------------------
    void Text::setText(const std::string& str)
    {
        mString = str;
        mText->setText(str);
    }

}; //end namespace

