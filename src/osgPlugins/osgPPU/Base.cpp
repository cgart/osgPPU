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
#include "Base.h"

//--------------------------------------------------------------------------
bool StateSet_matchModeStr(const char* str,osg::StateAttribute::GLModeValue& mode)
{
    if (strcmp(str,"INHERIT")==0) mode = osg::StateAttribute::INHERIT;
    else if (strcmp(str,"ON")==0) mode = osg::StateAttribute::ON;
    else if (strcmp(str,"OFF")==0) mode = osg::StateAttribute::OFF;
    else if (strcmp(str,"OVERRIDE_ON")==0) mode = osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON;
    else if (strcmp(str,"OVERRIDE_OFF")==0) mode = osg::StateAttribute::OVERRIDE|osg::StateAttribute::OFF;
    else if (strcmp(str,"OVERRIDE|ON")==0) mode = osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON;
    else if (strcmp(str,"OVERRIDE|OFF")==0) mode = osg::StateAttribute::OVERRIDE|osg::StateAttribute::OFF;
    else if (strcmp(str,"PROTECTED|ON")==0) mode = osg::StateAttribute::PROTECTED|osg::StateAttribute::ON;
    else if (strcmp(str,"PROTECTED|OFF")==0) mode = osg::StateAttribute::PROTECTED|osg::StateAttribute::OFF;
    else if (strcmp(str,"PROTECTED|OVERRIDE|ON")==0) mode = osg::StateAttribute::PROTECTED|osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON;
    else if (strcmp(str,"PROTECTED|OVERRIDE|OFF")==0) mode = osg::StateAttribute::PROTECTED|osg::StateAttribute::OVERRIDE|osg::StateAttribute::OFF;
    else return false;
    return true;
}


//--------------------------------------------------------------------------
const char* StateSet_getModeStr(osg::StateAttribute::GLModeValue value)
{
    switch(value)
    {
        case(osg::StateAttribute::INHERIT): return "INHERIT";
        case(osg::StateAttribute::ON): return "ON";
        case(osg::StateAttribute::OFF): return "OFF";
        case(osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON): return "OVERRIDE|ON";
        case(osg::StateAttribute::OVERRIDE|osg::StateAttribute::OFF): return "OVERRIDE|OFF";
        case(osg::StateAttribute::PROTECTED|osg::StateAttribute::ON): return "PROTECTED|ON";
        case(osg::StateAttribute::PROTECTED|osg::StateAttribute::OFF): return "PROTECTED|OFF";
        case(osg::StateAttribute::PROTECTED|osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON): return "PROTECTED|OVERRIDE|ON";
        case(osg::StateAttribute::PROTECTED|osg::StateAttribute::OVERRIDE|osg::StateAttribute::OFF): return "PROTECTED|OVERRIDE|OFF";
    }
    return "";
}

//--------------------------------------------------------------------------
bool Texture_matchInternalFormatStr(const char* str,int& value)
{
    if (     strcmp(str,"GL_INTENSITY")==0) value = GL_INTENSITY;
    else if (strcmp(str,"GL_LUMINANCE")==0) value = GL_LUMINANCE;
    else if (strcmp(str,"GL_ALPHA")==0) value = GL_ALPHA;
    else if (strcmp(str,"GL_LUMINANCE_ALPHA")==0) value = GL_LUMINANCE_ALPHA;
    else if (strcmp(str,"GL_RGB")==0) value = GL_RGB;
    else if (strcmp(str,"GL_RGBA")==0) value = GL_RGBA;
    else if (strcmp(str,"GL_COMPRESSED_ALPHA_ARB")==0) value = GL_COMPRESSED_ALPHA_ARB;
    else if (strcmp(str,"GL_COMPRESSED_LUMINANCE_ARB")==0) value = GL_COMPRESSED_LUMINANCE_ARB;
    else if (strcmp(str,"GL_COMPRESSED_INTENSITY_ARB")==0) value = GL_COMPRESSED_INTENSITY_ARB;
    else if (strcmp(str,"GL_COMPRESSED_LUMINANCE_ALPHA_ARB")==0) value = GL_COMPRESSED_LUMINANCE_ALPHA_ARB;
    else if (strcmp(str,"GL_COMPRESSED_RGB_ARB")==0) value = GL_COMPRESSED_RGB_ARB;
    else if (strcmp(str,"GL_COMPRESSED_RGBA_ARB")==0) value = GL_COMPRESSED_RGBA_ARB;
    else if (strcmp(str,"GL_COMPRESSED_RGB_S3TC_DXT1_EXT")==0) value = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    else if (strcmp(str,"GL_COMPRESSED_RGBA_S3TC_DXT1_EXT")==0) value = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    else if (strcmp(str,"GL_COMPRESSED_RGBA_S3TC_DXT3_EXT")==0) value = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    else if (strcmp(str,"GL_COMPRESSED_RGBA_S3TC_DXT5_EXT")==0) value = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    else if (strcmp(str,"GL_ALPHA16F_ARB")==0) value = GL_ALPHA16F_ARB;
    else if (strcmp(str,"GL_ALPHA32F_ARB")==0) value = GL_ALPHA32F_ARB;
    else if (strcmp(str,"GL_INTENSITY16F_ARB")==0) value = GL_INTENSITY16F_ARB;
    else if (strcmp(str,"GL_INTENSITY32F_ARB")==0) value = GL_INTENSITY32F_ARB;
    else if (strcmp(str,"GL_RGBA16F_ARB")==0) value = GL_RGBA16F_ARB;
    else if (strcmp(str,"GL_RGBA32F_ARB")==0) value = GL_RGBA32F_ARB;
    else if (strcmp(str,"GL_LUMINANCE16F_ARB")==0) value = GL_LUMINANCE16F_ARB;
    else if (strcmp(str,"GL_LUMINANCE32F_ARB")==0) value = GL_LUMINANCE32F_ARB;
    else if (strcmp(str,"GL_LUMINANCE_ALPHA16F_ARB")==0) value = GL_LUMINANCE_ALPHA16F_ARB;
    else if (strcmp(str,"GL_LUMINANCE_ALPHA32F_ARB")==0) value = GL_LUMINANCE_ALPHA32F_ARB;
    else if (strcmp(str,"GL_RGB16F_ARB")==0) value = GL_RGB16F_ARB;
    else if (strcmp(str,"GL_RGB32F_ARB")==0) value = GL_RGB32F_ARB;
    else
    {
        osgDB::Field::FieldType type = osgDB::Field::calculateFieldType(str);
        if (type==osgDB::Field::INTEGER)
        {
            value = atoi(str);
            return true;
        }
        else
        {
            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------
const char* Texture_getInternalFormatStr(int value)
{
    switch(value)
    {
        case(GL_INTENSITY): return "GL_INTENSITY";
        case(GL_LUMINANCE): return "GL_LUMINANCE";
        case(GL_ALPHA): return "GL_ALPHA";
        case(GL_LUMINANCE_ALPHA): return "GL_LUMINANCE_ALPHA";
        case(GL_RGB): return "GL_RGB";
        case(GL_RGBA): return "GL_RGBA";
        case(GL_COMPRESSED_ALPHA_ARB): return "GL_COMPRESSED_ALPHA_ARB";
        case(GL_COMPRESSED_LUMINANCE_ARB): return "GL_COMPRESSED_LUMINANCE_ARB";
        case(GL_COMPRESSED_INTENSITY_ARB): return "GL_COMPRESSED_INTENSITY_ARB";
        case(GL_COMPRESSED_LUMINANCE_ALPHA_ARB): return "GL_COMPRESSED_LUMINANCE_ALPHA_ARB";
        case(GL_COMPRESSED_RGB_ARB): return "GL_COMPRESSED_RGB_ARB";
        case(GL_COMPRESSED_RGBA_ARB): return "GL_COMPRESSED_RGBA_ARB";
        case(GL_COMPRESSED_RGB_S3TC_DXT1_EXT): return "GL_COMPRESSED_RGB_S3TC_DXT1_EXT";
        case(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT): return "GL_COMPRESSED_RGBA_S3TC_DXT1_EXT";
        case(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT): return "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT";
        case(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT): return "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT";
        case(GL_INTENSITY32F_ARB): return "GL_INTENSITY32F_ARB";
        case(GL_LUMINANCE32F_ARB): return "GL_LUMINANCE32F_ARB";
        case(GL_ALPHA32F_ARB): return "GL_ALPHA32F_ARB";
        case(GL_LUMINANCE_ALPHA32F_ARB): return "GL_LUMINANCE_ALPHA32F_ARB";
        case(GL_RGB32F_ARB): return "GL_RGB32F_ARB";
        case(GL_RGBA32F_ARB): return "GL_RGBA32F_ARB";
        case(GL_INTENSITY16F_ARB): return "GL_INTENSITY16F_ARB";
        case(GL_LUMINANCE16F_ARB): return "GL_LUMINANCE16F_ARB";
        case(GL_ALPHA16F_ARB): return "GL_ALPHA16F_ARB";
        case(GL_LUMINANCE_ALPHA16F_ARB): return "GL_LUMINANCE_ALPHA16F_ARB";
        case(GL_RGB16F_ARB): return "GL_RGB16F_ARB";
        case(GL_RGBA16F_ARB): return "GL_RGBA16F_ARB";
    }
    return NULL;
}

