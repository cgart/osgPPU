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

#ifndef _C_OSGPPU_SHADER_H_
#define _C_OSGPPU_SHADER_H_

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

//! Helper class to work with ppus and their shaders
/**
 * The shader class does encapsulate osg::Program and osg::Uniforms in one class.
 * The using of shader programs is therefor simplified. Shaders can be bound 
 * to any osg::StateSet.
 * NOTE: The shader class is deprecated. It will be replaced by a little bit different
 * interface in the next release. Hence try not to be very dependent on this implementation.
 * Shaders can be easyly bounded to Units by using Unit's StateSets. 
 *
 **/
class OSGPPU_EXPORT Shader : public osg::Object
{
    public:
        META_Object(osgPPU, Shader)

        //! Initialize new shader class
        Shader();

        /**
        * Copy constructor to create a shader object from the other one.
        * NOTE: copyop is currently ignored. The uniforms will be copied completely,
        * hence after the copy you will get two uniforms with the same value.
        **/
        Shader(const Shader&, const osg::CopyOp& copyop=osg::CopyOp::DEEP_COPY_ALL);
        
        //! Release used memory and close all used shaders
        virtual ~Shader();
            
        /**
         * Add new shader program. 
         * @param sh Shader object holding the shader program
         **/
        void addShader(osg::Shader* sh);

        /**
         * Return corresponding osg::Program used in this class.
         **/
        osg::Program* getProgram() const { return mProgram.get(); }

        /**
        * Set currently used program. This method does copy the program 
        * instead of using the given pointer directly.
        **/
        void setProgram(osg::Program* program);

        /**
        * Add new uniform. The uniform can also represent an array.
        * @param name Name of the uniform 
        * @param type Type of the uniform 
        * @param elementCount Number of elements if you add an array, otherwise 1
        **/
        void add(const std::string& name, osg::Uniform::Type type, unsigned int elementCount = 1);

        /**
        * Add new uniform. The uniform value will be copied.
        **/
        void add(osg::Uniform* uniform);
        void add(osg::StateSet::RefUniformPair uniform);

        /**
        * Delete uniform. Uniforms which are deleted are removed from the statesets.
        * @param name Name of the uniform 
        **/
        void del(const std::string& name);
        
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
        * @return false if some of the parameters is wrong 
        **/
        bool bind(unsigned int index, const std::string& name, osg::Texture* tex, int unit = -1);
        bool bind(const std::string& name, osg::Texture* tex, int unit = -1) {return bind(0, name, tex, unit);}

        /**
        * Bind a vertex attribute to a uniform. You have to take care to deliver correct
        * attribute data for the specified attribute.
        * @param name Name of the uniform variable
        * @param index Attribute index which to bound 
        **/                
        bool bindAttribute(const std::string& name, unsigned int index);

        /**
        * For shader model 4.0 hardwares you can specify the frag data to bound 
        **/
        bool bindFragData(const std::string& name, unsigned int index);
        
        /**
        * Check if certain uniform was added previously to the shader.
        * @param name Name of the uniform 
        **/
        bool isUniformExists(const std::string& name);
        
        /**
         * Enable the shader in the specified state set.
         * NOTE: This will also bind texture to specified or
         *  automagicaly chosen texture units.
         * @param ss StateSet where to apply the shader 
         * @param updateBindings should be the bindings automatically 
         *  being updated if they changes while the shader is enabled
         **/
        void enable(osg::StateSet* ss, bool updateBindings = true);

        /**
        * Method which can be used by derived classes to update their data.
        **/
        virtual void update() {};

        /**
         * Disable the shader in the given state set.
         * NOTE: Texture binding will not be removed, so
         * you have to call StateSet::removeTextureAttribute()
         * to unbind textures from the texture units.
         **/
        void disable(osg::StateSet* ss);
                                
        /**
        * Get uniform by a its name. If uniform was previously added or created this method will return it.
        * @param name Name of the uniform
        **/
        osg::Uniform* get(const std::string& name);
                
        /**
        * Get correpsonding uniform list.
        **/
        const osg::StateSet::UniformList& getUniformList() const { return mUniforms; }

        /**
        * Set uniform list. The unfiroms setted before will be removed.
        * You have to call enable() method afterwards to assign the new uniform
        * list to the stateset.
        **/
        void setUniformList(const osg::StateSet::UniformList& list);

        /**
        * Set the number of maximal supported texture units. Per default this number is set to 8.
        **/
        void setMaximalSupportedTextureUnits(int i);

        //! Store texture to units mapping
        struct TexUnit{
            osg::ref_ptr<osg::Texture> t;
            int unit;
            unsigned int element;
            std::string name;
        };

        /**
        * Reset texture bindings. Call this if you have rebound textures and want to force
        * to recreate texture boundings within the shader. Normally this would be called
        * automatically by the update function if you have previously called any bind function
        **/
        void resetBindings(osg::StateSet* ss);

    protected:

        typedef std::map<std::string, std::map<int,TexUnit> > TexUnitDb;
        TexUnitDb mTexUnits;
        
        //! Here we store the osg programm holding all shader objects
        osg::ref_ptr<osg::Program>  mProgram;
        
        //! List of all added parameters
        osg::StateSet::UniformList mUniforms;

        /**
         * Set parameters as uniform values.
         **/
        void addParameter(const std::string& name, osg::Uniform* param);

        virtual void onAddUniform(osg::Uniform* ) {};
        virtual void onDelUniform(osg::Uniform* ) {};
        virtual void onSetUniform(osg::Uniform* ) {};
        virtual void onEnable(osg::StateSet* ) {};
        virtual void onDisable(osg::StateSet* ) {};
        virtual void onAddSource(osg::Shader* ) {};
        virtual void onBindTexture(TexUnit& ) {};
        virtual void onBindAttribute(const std::string& , int ) {};
        virtual void onBindFragData(const std::string& , int ) {};
                
        //! Convert string type name into type
        osg::Uniform::Type convertToUniformType(const std::string& name);

        //! Update callback for the programm update
        class UpdateCallback : public virtual osg::StateAttribute::Callback
        {
            public:
                UpdateCallback(Shader* sh, osg::StateSet* ss);
                virtual void operator () (osg::StateAttribute*, osg::NodeVisitor*);
            private:
                Shader* mShader;
                osg::ref_ptr<osg::StateSet> mStateSet;
        };
        
        //! mark if boundings are dirty
        bool mDirtyBoundings;

        //! maximal possible number of supported texture units
        int mMaxTextureUnits;

    //private:

        //void copyToShader(Shader* sh);
        //void copyFromShader(Shader* sh);
    
};      
        
}; // end namespace

#endif
