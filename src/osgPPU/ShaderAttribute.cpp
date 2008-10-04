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

#include <osgPPU/ShaderAttribute.h>
#include <osg/StateAttribute>
#include <assert.h>

#define DEBUG_SH 0

namespace osgPPU
{

//--------------------------------------------------------------------------
ShaderAttribute::ShaderAttribute()
{
    mDirtyTextureBindings = true;
    mMaxTextureUnits = 8;
}

//--------------------------------------------------------------------------
ShaderAttribute::ShaderAttribute(const ShaderAttribute& sh, const osg::CopyOp& copyop) :
    osg::Program(sh, copyop),
    mTexUnits(sh.mTexUnits),
    mDirtyTextureBindings(sh.mDirtyTextureBindings),
    mMaxTextureUnits(sh.mMaxTextureUnits)
{
    setName(sh.getName());

    // copy attributes
    osg::Program::AttribBindingList::const_iterator it = sh.getAttribBindingList().begin();
    for (; it != sh.getAttribBindingList().end(); it ++)
        addBindAttribLocation(it->first, it->second);

    // copy uniforms
    mUniforms.clear();
    osg::StateSet::UniformList::const_iterator jt = sh.mUniforms.begin();
    for (; jt!= sh.mUniforms.end(); jt++)
    {
        osg::ref_ptr<osg::Uniform> u = new osg::Uniform(
                        jt->second.first->getType(),
                        jt->second.first->getName(),
                        jt->second.first->getNumElements());
        u->copyData(*jt->second.first);
        mUniforms[jt->first] = osg::StateSet::RefUniformPair(u, jt->second.second);
    }
}


//--------------------------------------------------------------------------
ShaderAttribute::~ShaderAttribute()
{

}

//--------------------------------------------------------------------------
void ShaderAttribute::setMaximalSupportedTextureUnits(int i)
{
    mMaxTextureUnits = i;
}

//--------------------------------------------------------------------------
osg::Uniform::Type ShaderAttribute::convertToUniformType(const std::string& name)
{
#define value(str, type) if (name == std::string(str)) return type;

    value("float", osg::Uniform::FLOAT)
    value("vec2", osg::Uniform::FLOAT_VEC2)
    value("vec3", osg::Uniform::FLOAT_VEC3)
    value("vec4", osg::Uniform::FLOAT_VEC4)

    value("bool", osg::Uniform::BOOL)
    value("bvec2", osg::Uniform::BOOL_VEC2)
    value("bvec3", osg::Uniform::BOOL_VEC3)
    value("bvec4", osg::Uniform::BOOL_VEC4)

    value("int", osg::Uniform::INT)
    value("ivec2", osg::Uniform::INT_VEC2)
    value("ivec3", osg::Uniform::INT_VEC3)
    value("ivec4", osg::Uniform::INT_VEC4)

    value("mat2", osg::Uniform::FLOAT_MAT2)
    value("mat3", osg::Uniform::FLOAT_MAT3)
    value("mat4", osg::Uniform::FLOAT_MAT4)

    value("sampler1D", osg::Uniform::SAMPLER_1D)
    value("sampler2D", osg::Uniform::SAMPLER_2D)
    value("sampler3D", osg::Uniform::SAMPLER_3D)
    value("samplerCube", osg::Uniform::SAMPLER_CUBE)
    value("sampler1DShadow", osg::Uniform::SAMPLER_1D_SHADOW)
    value("sampler2DShadow", osg::Uniform::SAMPLER_2D_SHADOW)

    value("sampler1DArray", osg::Uniform::SAMPLER_1D_ARRAY)
    value("sampler2DArray", osg::Uniform::SAMPLER_2D_ARRAY)
    value("sampler1DArrayShadow", osg::Uniform::SAMPLER_1D_ARRAY_SHADOW)
    value("sampler2DArrayShadow", osg::Uniform::SAMPLER_2D_ARRAY_SHADOW)

#undef value
    return osg::Uniform::FLOAT;
}


//--------------------------------------------------------------------------
void ShaderAttribute::addParameter(const std::string& name, osg::Uniform* param, osg::StateAttribute::OverrideValue mode)
{
    if (param)
        mUniforms[name] = osg::StateSet::RefUniformPair(param, mode);
}

//--------------------------------------------------------------------------
void ShaderAttribute::add(const std::string& name, osg::Uniform::Type type, unsigned int elementCount, osg::StateAttribute::OverrideValue mode)
{
    addParameter(name, new osg::Uniform(type, name, elementCount), mode);
}

//--------------------------------------------------------------------------
void ShaderAttribute::add(osg::Uniform* uniform, osg::StateAttribute::OverrideValue mode)
{
    if (!uniform) return;

    // create a copy of the uniform
    osg::Uniform* u = new osg::Uniform(
                    uniform->getType(),
                    uniform->getName(),
                    uniform->getNumElements());
    u->copyData(*uniform);

    addParameter(u->getName(), u, mode);
}


//--------------------------------------------------------------------------
void ShaderAttribute::add(osg::StateSet::RefUniformPair uniform)
{
    if (!uniform.first) return;

    // create a copy of the uniform
    osg::Uniform* u = new osg::Uniform(
                    uniform.first->getType(),
                    uniform.first->getName(),
                    uniform.first->getNumElements());
    u->copyData(*(uniform.first));

    addParameter(u->getName(), u, uniform.second);
}

//--------------------------------------------------------------------------
void ShaderAttribute::del(const std::string& name)
{
    // remove uniform from our database
    osg::StateSet::UniformList::iterator it = mUniforms.find(name);
    if (it != mUniforms.end())
        mUniforms.erase(it);
}

//--------------------------------------------------------------------------
osg::Uniform* ShaderAttribute::get(const std::string& name)
{
    osg::StateSet::UniformList::const_iterator it = mUniforms.begin();
    for (; it != mUniforms.end(); it++)
    {
        if (it->first == name) return it->second.first.get();
    }
    return NULL;
}

//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, bool b0){
    osg::Uniform* u = get(name);
    if (!u) return false;
    u->setElement(index, b0);
    return true;
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, bool b0, bool b1){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, b0, b1);
    return true;
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, bool b0, bool b1, bool b2){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, b0,b1,b2);
    return true;
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, bool b0, bool b1, bool b2, bool b3){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, b0,b1,b2,b3);
    return true;
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, int i0){
    osg::Uniform* u = get(name);
    if (!u) return false;
    u->setElement(index, i0);
    return true;
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, int i0, int i1){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, i0,i1);
    return true;
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, int i0, int i1, int i2){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, i0,i1,i2);
    return true;
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, int i0, int i1, int i2, int i3){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, i0,i1,i2,i3);
    return true;
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, float f0){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, f0);
    return true;
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, float f0, float f1){
    return set(index, name, osg::Vec2(f0,f1));
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, float f0, float f1, float f2){
    return set(index, name, osg::Vec3(f0,f1,f2));
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, float f0, float f1, float f2, float f3){
    return set(index, name, osg::Vec4(f0,f1,f2,f3));
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, const osg::Vec2& v){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, v);
    return true;
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, const osg::Vec3& v){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, v);
    return true;
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, const osg::Vec4& v){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, v);
    return true;
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, const osg::Matrix2& m){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, m);
    return true;
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, const osg::Matrix3& m){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, m);
    return true;
}
//--------------------------------------------------------------------------
bool ShaderAttribute::set(unsigned int index, const std::string& name, const osg::Matrixf& m){
    osg::Uniform* u = get(name);
    if (!u) return false;
    u->setElement(index, m);
    return true;
}

//--------------------------------------------------------------------------
bool ShaderAttribute::bindTexture(unsigned int index, const std::string& name, osg::Texture* t, int unit)
{
    // check whenever such an uniform already exists, if not so add it
    if (!get(name)) add(name, osg::Uniform::INT, 1);

    // check if index is valid
    if (index > get(name)->getNumElements() )
    {
        printf("You want to bind a texture to %s[%d], but there is only %d elements registered\n", name.c_str(), index, get(name)->getNumElements());
        return false;
    }

    // check if we have already such one setted to an element
    TexUnitDb::iterator it = mTexUnits.find(name);

    // there exists already such binding
    if (it != mTexUnits.end())
    {
        // check if there is any bounding for the given index
        std::map<int, TexUnit>::iterator jt = it->second.find(index);

        // ok there is such one, so proceed only if we want to change something
        if (jt != it->second.end())
        {
            if (t == jt->second.t.get())
            {
                return true;
            }
        }
    }

    // create new database content
    TexUnit tu;
    tu.t = t;
    tu.element = index;
    tu.unit = unit;
    tu.name = name;

    // add new texunit into database
    (mTexUnits[name])[index] = tu;

    // texture boundings are dirty now
    mDirtyTextureBindings = true;

    // just set value if predefined
    if (unit >= 0)
    {
        set(index, name, (int)unit);
    }

    return true;
}

//--------------------------------------------------------------------------
bool ShaderAttribute::bindAttribute(const std::string& name, unsigned int index)
{
    addBindAttribLocation(name, index);
    return true;
}

//--------------------------------------------------------------------------
bool ShaderAttribute::bindFragData(const std::string& name, unsigned int index)
{
    addBindFragDataLocation(name, index);
    return true;
}

//--------------------------------------------------------------------------
void ShaderAttribute::setUniformList(const osg::StateSet::UniformList& list)
{
    mUniforms = list;
}

//--------------------------------------------------------------------------
void ShaderAttribute::apply(osg::State& state) const
{
    // first apply the program as it is
    osg::Program::apply(state);

    // this have to be our object
    const Program::PerContextProgram* lastAppliedProgram = state.getLastAppliedProgramObject();
    if (lastAppliedProgram == NULL) return;
    //assert(lastAppliedProgram == getPCP(state.getContextID()));

    // now apply all uniforms which are in the database
    for (osg::StateSet::UniformList::const_iterator it = mUniforms.begin(); it != mUniforms.end(); it++)
    {
        if ((it->second.second & osg::StateAttribute::ON) == osg::StateAttribute::ON)
            lastAppliedProgram->apply(*(it->second.first));
    }

    // if texture boundings are dirty, then reset them
    if (mDirtyTextureBindings)
        const_cast<ShaderAttribute*>(this)->resetTextureUniforms();
}

//--------------------------------------------------------------------------
void ShaderAttribute::resetTextureUniforms()
{
    // enable texture units and bind them
    TexUnitDb::iterator it = mTexUnits.begin();
    std::vector<TexUnit*> units(mMaxTextureUnits);
    for (int i=0; i < mMaxTextureUnits; i++) units.push_back(NULL);

    // first assign specified units
    for (; it != mTexUnits.end(); it++)
    {
        // now go through all elements
        std::map<int, TexUnit>::iterator jt = it->second.begin();
        for (; jt != it->second.end(); jt++)
        {
            // check if unit was specified
            if (jt->second.unit >= 0 && jt->second.unit < mMaxTextureUnits)
            {
                // check if unit is free
                if (units[jt->second.unit] == NULL)
                    units[jt->second.unit] = &jt->second;
                else {
                    printf("Shader::enable() - You have specified a unit number %d to %s in %s which is already given!\n", jt->second.unit, it->first.c_str(), getName().c_str());
                }
            }
        }
    }

    // now for the rest assign free units
    it = mTexUnits.begin();
    for (; it != mTexUnits.end(); it++)
    {
        // now go through all elements
        std::map<int, TexUnit>::iterator jt = it->second.begin();
        for (; jt != it->second.end(); jt++)
        {
            // check if unit was specified
            if (jt->second.unit < 0 || jt->second.unit >=mMaxTextureUnits)
            {
                // iterate through unit list and search for first free place
                for (unsigned int i=0; i < units.size(); i++)
                    if (units[i] == NULL){
                        units[i] = &jt->second;
                        break;
                    }
            }
        }
    }

    // now iterate through the units and set them
    for (unsigned int i=0; i < units.size(); i++)
    {
        if (units[i] != NULL)
        {
            // if texture specified so assign it
            if (units[i]->t.valid())
            {
                // for each parent stateset we set apply the textures accordingly
                for (unsigned j=0; j < getNumParents(); j++)
                    getParent(j)->setTextureAttribute(i, units[i]->t.get());

                // setup default texture properties
                if (get("g_TextureWidth")) set(i, "g_TextureWidth", units[i]->t->getTextureWidth());
                if (get("g_TextureHeight")) set(i, "g_TextureHeight", units[i]->t->getTextureHeight());
                if (get("g_TextureDepth")) set(i, "g_TextureDepth", units[i]->t->getTextureDepth());
            }

            // set unit number for this uniform
            set(units[i]->element, units[i]->name, (int)i);
        }
    }

    mDirtyTextureBindings = false;
}

}; //end namespace

