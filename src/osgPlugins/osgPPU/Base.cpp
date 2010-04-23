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
#include <cstring>

//--------------------------------------------------------------------------
bool ppu_StateSet_matchModeStr(const char* str,osg::StateAttribute::GLModeValue& mode)
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
const char* ppu_StateSet_getModeStr(osg::StateAttribute::GLModeValue value)
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
bool ppu_Texture_matchInternalFormatStr(const char* _str,int& value)
{
    std::string str (_str);
    std::transform( str.begin(), str.end(), str.begin(), ::toupper );
   
    if      (str == std::string("GL_INTENSITY")) value = GL_INTENSITY;
    else if (str == std::string("GL_LUMINANCE")) value = GL_LUMINANCE;
    else if (str == std::string("GL_ALPHA")) value = GL_ALPHA;
    else if (str == std::string("GL_LUMINANCE_ALPHA")) value = GL_LUMINANCE_ALPHA;
    else if (str == std::string("GL_RGB")) value = GL_RGB;
    else if (str == std::string("GL_RGBA")) value = GL_RGBA;
    else if (str == std::string("GL_COMPRESSED_ALPHA_ARB")) value = GL_COMPRESSED_ALPHA_ARB;
    else if (str == std::string("GL_COMPRESSED_LUMINANCE_ARB")) value = GL_COMPRESSED_LUMINANCE_ARB;
    else if (str == std::string("GL_COMPRESSED_INTENSITY_ARB")) value = GL_COMPRESSED_INTENSITY_ARB;
    else if (str == std::string("GL_COMPRESSED_LUMINANCE_ALPHA_ARB")) value = GL_COMPRESSED_LUMINANCE_ALPHA_ARB;
    else if (str == std::string("GL_COMPRESSED_RGB_ARB")) value = GL_COMPRESSED_RGB_ARB;
    else if (str == std::string("GL_COMPRESSED_RGBA_ARB")) value = GL_COMPRESSED_RGBA_ARB;
    else if (str == std::string("GL_COMPRESSED_RGB_S3TC_DXT1_EXT")) value = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    else if (str == std::string("GL_COMPRESSED_RGBA_S3TC_DXT1_EXT")) value = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    else if (str == std::string("GL_COMPRESSED_RGBA_S3TC_DXT3_EXT")) value = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    else if (str == std::string("GL_COMPRESSED_RGBA_S3TC_DXT5_EXT")) value = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    else if (str == std::string("GL_ALPHA16F_ARB")) value = GL_ALPHA16F_ARB;
    else if (str == std::string("GL_ALPHA32F_ARB")) value = GL_ALPHA32F_ARB;
    else if (str == std::string("GL_INTENSITY16F_ARB")) value = GL_INTENSITY16F_ARB;
    else if (str == std::string("GL_INTENSITY32F_ARB")) value = GL_INTENSITY32F_ARB;
    else if (str == std::string("GL_LUMINANCE16F_ARB")) value = GL_LUMINANCE16F_ARB;
    else if (str == std::string("GL_LUMINANCE32F_ARB")) value = GL_LUMINANCE32F_ARB;
    else if (str == std::string("GL_LUMINANCE_ALPHA16F_ARB")) value = GL_LUMINANCE_ALPHA16F_ARB;
    else if (str == std::string("GL_LUMINANCE_ALPHA32F_ARB")) value = GL_LUMINANCE_ALPHA32F_ARB;
    else if (str == std::string("GL_RGB16F_ARB")) value = GL_RGB16F_ARB;
    else if (str == std::string("GL_RGB32F_ARB")) value = GL_RGB32F_ARB;
    else if (str == std::string("GL_RGBA16F_ARB")) value = GL_RGBA16F_ARB;
    else if (str == std::string("GL_RGBA32F_ARB")) value = GL_RGBA32F_ARB;
    else
    {
        osgDB::Field::FieldType type = osgDB::Field::calculateFieldType(str.c_str());
        if (type==osgDB::Field::INTEGER)
        {
            value = atoi(str.c_str());
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
const char* ppu_Texture_getInternalFormatStr(int value)
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


//--------------------------------------------------------------------------
bool ppu_Texture_matchOutputTypeStr(const char* str, osgPPU::UnitInOut::TextureType& value)
{
    if (     strcmp(str,"TEXTURE_2D")==0) value = osgPPU::UnitInOut::TEXTURE_2D;
    else if (strcmp(str,"TEXTURE_CUBEMAP")==0) value = osgPPU::UnitInOut::TEXTURE_CUBEMAP;
    else if (strcmp(str,"TEXTURE_3D")==0) value = osgPPU::UnitInOut::TEXTURE_3D;
    else
    {
        osgDB::Field::FieldType type = osgDB::Field::calculateFieldType(str);
        if (type==osgDB::Field::INTEGER)
        {
            value = (osgPPU::UnitInOut::TextureType)atoi(str);
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
const char* ppu_Texture_getOutputTextureTypeStr(osgPPU::UnitInOut::TextureType value)
{
    switch(value)
    {
        case(osgPPU::UnitInOut::TEXTURE_2D): return "TEXTURE_2D";
        case(osgPPU::UnitInOut::TEXTURE_CUBEMAP): return "TEXTURE_CUBEMAP";
        case(osgPPU::UnitInOut::TEXTURE_3D): return "TEXTURE_3D";
    }
    return NULL;
}
