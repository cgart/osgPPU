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

#include <osgPPU/Utility.h>

#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/TextureCubeMap>
#include <osg/TextureRectangle>
#include <osg/Texture2DArray>

namespace osgPPU
{
//--------------------------------------------------------------------------
GLenum createSourceTextureFormat(GLenum internalFormat)
{
    switch (internalFormat)
    {
        case GL_LUMINANCE32F_ARB:
        case GL_LUMINANCE16F_ARB: return GL_LUMINANCE;

        case GL_LUMINANCE_ALPHA32F_ARB:
        case GL_LUMINANCE_ALPHA16F_ARB: return GL_LUMINANCE_ALPHA;

        case GL_RGB32F_ARB:
        case GL_RGB16F_ARB: return GL_RGB;

        case GL_RGBA32F_ARB:
        case GL_RGBA16F_ARB: return GL_RGBA;

        case GL_LUMINANCE32UI_EXT:
        case GL_LUMINANCE32I_EXT:
        case GL_LUMINANCE16UI_EXT:
        case GL_LUMINANCE16I_EXT:
        case GL_LUMINANCE8UI_EXT:
        case GL_LUMINANCE8I_EXT: return GL_LUMINANCE_INTEGER_EXT;

        case GL_LUMINANCE_ALPHA32UI_EXT:
        case GL_LUMINANCE_ALPHA32I_EXT:
        case GL_LUMINANCE_ALPHA16UI_EXT:
        case GL_LUMINANCE_ALPHA16I_EXT:
        case GL_LUMINANCE_ALPHA8UI_EXT:
        case GL_LUMINANCE_ALPHA8I_EXT: return GL_LUMINANCE_ALPHA_INTEGER_EXT;

        case GL_RGB32UI_EXT:
        case GL_RGB32I_EXT:
        case GL_RGB16UI_EXT:
        case GL_RGB16I_EXT:
        case GL_RGB8UI_EXT:
        case GL_RGB8I_EXT: return GL_RGB_INTEGER_EXT;

        case GL_RGBA32UI_EXT:
        case GL_RGBA32I_EXT:
        case GL_RGBA16UI_EXT:
        case GL_RGBA16I_EXT:
        case GL_RGBA8UI_EXT:
        case GL_RGBA8I_EXT: return GL_RGBA_INTEGER_EXT;

        default: return internalFormat;
    }
}

//--------------------------------------------------------------------------
osg::Uniform::Type convertTextureToUniformType(osg::Texture* tex)
{
    
    if (dynamic_cast<osg::Texture1D*>(tex)) return osg::Uniform::SAMPLER_1D;
    if (dynamic_cast<osg::Texture2D*>(tex)) return osg::Uniform::SAMPLER_2D;
    if (dynamic_cast<osg::Texture3D*>(tex)) return osg::Uniform::SAMPLER_3D;
    if (dynamic_cast<osg::TextureCubeMap*>(tex)) return osg::Uniform::SAMPLER_CUBE;
    if (dynamic_cast<osg::TextureRectangle*>(tex)) return osg::Uniform::SAMPLER_2D;
    if (dynamic_cast<osg::Texture2DArray*>(tex)) return osg::Uniform::SAMPLER_2D_ARRAY;
    
    return osg::Uniform::UNDEFINED;
    
}


}; //end namespace




