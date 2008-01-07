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

#ifndef _C_OSGPPU_TEXT_H_
#define _C_OSGPPU_TEXT_H_

//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osg/Projection>

#include <osgText/Font>
#include <osgText/Text>
#include <osg/Geode>
#include <osg/Group>
#include <osg/CameraNode>


namespace osgPPU
{

    /**
    * Simple class drawing a text on the screen.
    **/
    class Text : public osg::Object
    {
        public:
   
            META_Object(osgPPU, Text)

            //! Simpel constructor
            Text();

            //! Copy constructor
            Text(const Text&, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);
    
            //! Destructor
            virtual ~Text();
    
            /**
            * Place the text on certain 2D-Coordinates on the screen
            * The coordinates arer between 0 and 1
            **/
            void setPosition(float x, float y);
    
            /**
            * Specify the text which sould be shown
            **/
            void setText(const std::string& text);
            const std::string& getText() const { return mString; }
    
            /**
            * Specify the file name containing the true type font to load
            * and use for this text
            **/
            bool setFont(const std::string& fontFile);
    
            /**
            * Set text color
            **/
            void setColor (float r, float g, float b, float a) {setColor(osg::Vec4(r,g,b,a));}
            void setColor (const osg::Vec4& color);
    
            const osg::Vec4& getColor() const { return mColor; }
    
            /**
            * Set the size of the text
            **/
            void setSize(float s, float aspectRatio = 1.0);
    
            /**
            * Get the node of this text. SO it can be added to the scene graph
            **/
            osg::Node* getNode() {return mGroup.get();}
    
            /**
            * Get text pointer
            **/
            osgText::Text* getTextPtr () { return mText.get(); }
            
        protected:
            osg::ref_ptr<osgText::Text> mText;
            osg::ref_ptr<osg::Group> mGroup;
    
            std::string mString;
            osg::Vec4 mColor;
    };

}; // end namespace

#endif
