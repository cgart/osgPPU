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

#ifndef _C_SHADER_ATTRIBUTE_H_
#define _C_SHADER_ATTRIBUTE_H_

//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osg/Shader>
#include <osg/Program>
#include <osg/Uniform>
#include <osg/Texture>
#include <osg/StateAttribute>

#include <osgPPU/Export.h>

namespace osgPPU
{

//! Wrapper class to simplify work with shader and uniforms
/**
 * ShaderAttribute class is a simple wrapper for the osg::Shader, osg::Program
 * and osg::Uniform classes to simplify the work with shaders. It can be used
 * to define shader programs for the osgPPU::Unit or in external applications
 * as well.
 *
 **/
class OSGPPU_EXPORT ShaderAttribute : public osg::Program
{
    public:
        META_Object(osgPPU, ShaderAttribute)

        /** Initialize shader to the fixed function pipeline per default **/
        ShaderAttribute();

        /**
        * Copy constructor to create a shader object from the other one.
        * NOTE: copyop is currently ignored. The uniforms will be copied completely,
        * hence after the copy you will get two uniforms with the same value.
        **/
        ShaderAttribute(const ShaderAttribute&, const osg::CopyOp& copyop=osg::CopyOp::DEEP_COPY_ALL);

        /** Release used memory and close all used shader programs **/
        virtual ~ShaderAttribute();

        /**
        * Add new uniform. The uniform can also represent an array.
        * The uniform will also be added to all the parent StateSet's. If
        * a parent stateset is attached later, then the uniform will be added
        * in the apply method.
        * @param name Name of the uniform
        * @param type Type of the uniform
        * @param elementCount Number of elements if you add an array, otherwise 1
        **/
        void add(const std::string& name, osg::Uniform::Type type, unsigned int elementCount = 1, osg::StateAttribute::OverrideValue mode = osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

        /**
        * Add new uniform. The uniform value will be copied.
        **/
        void add(osg::Uniform* uniform, osg::StateAttribute::OverrideValue mode = osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        void add(osg::StateSet::RefUniformPair uniform);

        /**
        * Delete uniform. Uniforms which are deleted are removed from the parental StateSets too.
        * @param name Name of the uniform
        **/
        void del(const std::string& name);

        /**
        * Set uniform value.
        **/
        bool set(const std::string& name, bool b0){return set(0,name,b0);}
        bool set(const std::string& name, bool b0, bool b1){return set(0,name,b0,b1);}
        bool set(const std::string& name, bool b0, bool b1, bool b2){return set(0,name,b0,b1,b2);}
        bool set(const std::string& name, bool b0, bool b1, bool b2, bool b3){return set(0,name,b0,b1,b2,b3);}

        bool set(const std::string& name, int i0){return set(0,name,i0);}
        bool set(const std::string& name, int i0, int i1){return set(0,name,i0,i1);}
        bool set(const std::string& name, int i0, int i1, int i2){return set(0,name,i0,i1,i2);}
        bool set(const std::string& name, int i0, int i1, int i2, int i3){return set(0,name,i0,i1,i2,i3);}

        bool set(const std::string& name, float b0){return set(0,name,b0);}
        bool set(const std::string& name, float b0, float b1){return set(0,name,b0,b1);}
        bool set(const std::string& name, float b0, float b1, float b2){return set(0,name,b0,b1,b2);}
        bool set(const std::string& name, float b0, float b1, float b2, float b3){return set(0,name,b0,b1,b2,b3);}

        bool set(const std::string& name, const osg::Vec2& v){return set(0,name,v);}
        bool set(const std::string& name, const osg::Vec3& v){return set(0,name,v);}
        bool set(const std::string& name, const osg::Vec4& v){return set(0,name,v);}

        bool set(const std::string& name, const osg::Matrix2& m){return set(0,name,m);}
        bool set(const std::string& name, const osg::Matrix3& m){return set(0,name,m);}
        bool set(const std::string& name, const osg::Matrixf& m){return set(0,name,m);}

        bool set(unsigned int index, const std::string& name, bool b0);
        bool set(unsigned int index, const std::string& name, bool b0, bool b1);
        bool set(unsigned int index, const std::string& name, bool b0, bool b1, bool b2);
        bool set(unsigned int index, const std::string& name, bool b0, bool b1, bool b2, bool b3);

        bool set(unsigned int index, const std::string& name, int i0);
        bool set(unsigned int index, const std::string& name, int i0, int i1);
        bool set(unsigned int index, const std::string& name, int i0, int i1, int i2);
        bool set(unsigned int index, const std::string& name, int i0, int i1, int i2, int i3);

        bool set(unsigned int index, const std::string& name, float b0);
        bool set(unsigned int index, const std::string& name, float b0, float b1);
        bool set(unsigned int index, const std::string& name, float b0, float b1, float b2);
        bool set(unsigned int index, const std::string& name, float b0, float b1, float b2, float b3);

        bool set(unsigned int index, const std::string& name, const osg::Vec2& v);
        bool set(unsigned int index, const std::string& name, const osg::Vec3& v);
        bool set(unsigned int index, const std::string& name, const osg::Vec4& v);

        bool set(unsigned int index, const std::string& name, const osg::Matrix2& m);
        bool set(unsigned int index, const std::string& name, const osg::Matrix3& m);
        bool set(unsigned int index, const std::string& name, const osg::Matrixf& m);


        /**
        * Bind a texture to the specified uniform. This method do siplify your life ;-)
        * @param index If uniform is an array element, then specify the index here
        * @param name Name of the uniform to which to bound the texture
        * @param tex  Texture to bound
        * @param unit Texture unit to which to bound the texture. Specify -1 to automagically choose free texture unit
        **/
        bool bindTexture(unsigned int index, const std::string& name, osg::Texture* tex, int unit = -1);

        //! @copydoc bindTexture()
        bool bindTexture(const std::string& name, osg::Texture* tex, int unit = -1) {return bind(0, name, tex, unit);}

        /**
        * Bind a vertex attribute to a uniform. You have to take care to deliver correct
        * attribute data for the specified attribute.
        * @see osg::Programm::addBindAttribLocation()
        * @param name Name of the uniform variable
        * @param index Attribute index which to bound
        **/
        bool bindAttribute(const std::string& name, unsigned int index);

        /**
        * For shader model 4.0 hardwares you can specify the frag data to bound
        **/
        bool bindFragData(const std::string& name, unsigned int index);

        /**
         * Apply the shader attribute to the given state. This will bind the shader program
         * and set the uniforms. NOTE: The uniforms would be bound in this method,
         * hence the appropriate uniform value might be propagated to the shader in the next frame.
         **/
        virtual void apply (osg::State &state) const;

        /** @copydoc osg::StateAttribute::compare() **/
        virtual int compare (const osg::StateAttribute &sa) const;

        /**
        * Get uniform by a its name. If uniform was previously added or created this method will return it.
        * @param name Name of the uniform
        **/
        osg::Uniform* get(const std::string& name);

        /**
        * Set uniform list.
        **/
        void setUniformList(const osg::StateSet::UniformList& list);

        /**
        * Get correpsonding uniform list.
        **/
        const osg::StateSet::UniformList& getUniformList() const { return mUniforms; }

        /**
        * Set the number of maximal supported texture units. Per default this number is set to 8.
        * This number is needed to setup automagic texture to uniform binding.
        **/
        void setMaximalSupportedTextureUnits(int i);

        /**
         * Dirty the shader. This will force to rebind and reset all the uniforms to the appropriate
         * parental StateAttributes.
        **/
        void dirty() { mDirty = true; }

    protected:

        typedef std::map<std::string, std::map<int,TexUnit> > TexUnitDb;
        typedef std::map<osg::StateSet*, bool> ParentDirtyMap;

        //! Database to hold the texture to uniform bindings
        TexUnitDb mTexUnits;

        //! List of all added parameters
        osg::StateSet::UniformList mUniforms;

        //! mark if boundings are dirty
        bool mDirtyTextureBindings;

        //! maximal possible number of supported texture units
        int mMaxTextureUnits;

        //! Marks if shader is dirty
        bool mDirty;

        //! List of all currently known parents and according dirty state
        ParentDirtyMap mParents;

        /** Texture tu unit mapping structure **/
        struct TexUnit{
            osg::ref_ptr<osg::Texture> t;
            int unit;
            unsigned int element;
            std::string name;
        };

        /**
         * Set parameters as uniform values.
         **/
        void addParameter(const std::string& name, osg::Uniform* param);

        /**
        * Reset texture bindings. Call this if you have rebound textures and want to force
        * to recreate texture boundings within the shader. Normally this would be called
        * automatically by the update function if you have previously called any bind function
        **/
        void resetBindings(osg::StateSet* ss);

        //! Convert string type name into type
        osg::Uniform::Type convertToUniformType(const std::string& name);

};

}; // end namespace

#endif
