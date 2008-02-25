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

#include <osgPPU/Shader.h>
#include <osg/StateAttribute>

#define DEBUG_SH 0

namespace osgPPU
{

//--------------------------------------------------------------------------
Shader::Shader()
{
    mProgram = new osg::Program();
    mDirtyBoundings = true;
    mMaxTextureUnits = 8;
}

//--------------------------------------------------------------------------
Shader::Shader(const Shader& sh, const osg::CopyOp& copyop)
{
    mTexUnits = sh.mTexUnits;
    mDirtyBoundings = sh.mDirtyBoundings;
    mMaxTextureUnits = sh.mMaxTextureUnits;
    setName(sh.getName());

    // copy program
    mProgram = new osg::Program();//*(sh.mProgram), copyop);
    mProgram->setName(sh.mProgram->getName());
    for (unsigned int i=0; i < sh.mProgram->getNumShaders(); i++)
        mProgram->addShader(new osg::Shader(*(sh.mProgram->getShader(i))));
    
    // copy attributes
    osg::Program::AttribBindingList::const_iterator it = sh.mProgram->getAttribBindingList().begin();
    for (; it != sh.mProgram->getAttribBindingList().end(); it ++)
        mProgram->addBindAttribLocation(it->first, it->second);

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
        /*osg::ref_ptr<osg::Uniform> u = copyop(jt->second.first.get());
        mUniforms[jt->first] = osg::StateSet::RefUniformPair(u, jt->second.second);*/
    }
}


//--------------------------------------------------------------------------
void Shader::setProgram(osg::Program* program)
{
    if (program == NULL) return;

    // copy program
    mProgram = new osg::Program();//*(sh.mProgram), copyop);
    mProgram->setName(program->getName());
    for (unsigned int i=0; i < program->getNumShaders(); i++)
    {
        mProgram->addShader(new osg::Shader(*(program->getShader(i))));
    }
    
    // copy attributes
    osg::Program::AttribBindingList::const_iterator it = program->getAttribBindingList().begin();
    for (; it != program->getAttribBindingList().end(); it ++)
        mProgram->addBindAttribLocation(it->first, it->second);

}

#if 0
//--------------------------------------------------------------------------
void Shader::copyToShader(Shader* sh)
{
    if (sh == NULL) return;
    sh->mProgram = new osg::Program(*mProgram);
    sh->mTexUnits = mTexUnits;
    sh->mUniforms = mUniforms;  
    sh->mPropertyMap = mPropertyMap;    
    sh->mDirtyBoundings = mDirtyBoundings;
}

//--------------------------------------------------------------------------
void Shader::copyFromShader(Shader* sh)
{
    if (sh == NULL) return;
    mTexUnits = sh->mTexUnits;
    mPropertyMap = sh->mPropertyMap;    
    mDirtyBoundings = sh->mDirtyBoundings;
    
    // copy program
    mProgram = new osg::Program();
    mProgram->setName(sh->mProgram->getName());
    for (unsigned int i=0; i < sh->mProgram->getNumShaders(); i++)
        mProgram->addShader(new osg::Shader(*sh->mProgram->getShader(i)));
    
    // copy attributes
    osg::Program::AttribBindingList::const_iterator it = sh->mProgram->getAttribBindingList().begin();
    for (; it != sh->mProgram->getAttribBindingList().end(); it ++)
        mProgram->addBindAttribLocation(it->first, it->second);
    
    // copy uniforms
    mUniforms.clear();
    osg::StateSet::UniformList::const_iterator jt = sh->mUniforms.begin();
    for (; jt!= sh->mUniforms.end(); jt++)
    {
        osg::ref_ptr<osg::Uniform> u = new osg::Uniform(
                        jt->second.first->getType(),
                        jt->second.first->getName(), 
                        jt->second.first->getNumElements()); 
        u->copyData(*jt->second.first);
        mUniforms[jt->first] = osg::StateSet::RefUniformPair(u, jt->second.second);              
    }
}
#endif

//--------------------------------------------------------------------------
Shader::~Shader()
{

}

//--------------------------------------------------------------------------
void Shader::setMaximalSupportedTextureUnits(int i)
{
    mMaxTextureUnits = i;
    mDirtyBoundings = true;
}

//--------------------------------------------------------------------------
osg::Uniform::Type Shader::convertToUniformType(const std::string& name)
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
    
    #ifdef _SH40_
    value("sampler1DArray", osg::Uniform::SAMPLER_1D_ARRAY)
    value("sampler2DArray", osg::Uniform::SAMPLER_2D_ARRAY)
    value("sampler1DArrayShadow", osg::Uniform::SAMPLER_1D_ARRAY_SHADOW)
    value("sampler2DArrayShadow", osg::Uniform::SAMPLER_2D_ARRAY_SHADOW)
    #endif
    
#undef value
    return osg::Uniform::FLOAT;
}

//--------------------------------------------------------------------------
void  Shader::addShader(osg::Shader* sh)
{
    if (sh != NULL)
    {
        mProgram->addShader(sh);
        onAddSource(sh);
    }
}

//--------------------------------------------------------------------------
void Shader::addParameter(const std::string& name, osg::Uniform* param)
{
    if (param){
        mUniforms[name] = osg::StateSet::RefUniformPair(param, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE | osg::StateAttribute::PROTECTED);
        onAddUniform(param);
        //printf("%s add uniform %s\n", getName().c_str(), param->getName().c_str());
    }
}

//--------------------------------------------------------------------------
void Shader::add(const std::string& name, osg::Uniform::Type type, unsigned int elementCount)
{
    addParameter(name, new osg::Uniform(type, name, elementCount));
}

//--------------------------------------------------------------------------
void Shader::add(osg::Uniform* uniform)
{
    if (!uniform) return;

    // create a copy of the uniform
    osg::Uniform* u = new osg::Uniform(
                    uniform->getType(),
                    uniform->getName(), 
                    uniform->getNumElements()); 
    u->copyData(*uniform);

    addParameter(u->getName(), u); 
}


//--------------------------------------------------------------------------
void Shader::add(osg::StateSet::RefUniformPair uniform)
{
    if (!uniform.first) return;

    // create a copy of the uniform
    osg::Uniform* u = new osg::Uniform(
                    uniform.first->getType(),
                    uniform.first->getName(), 
                    uniform.first->getNumElements()); 
    u->copyData(*(uniform.first));

    mUniforms[u->getName()] = osg::StateSet::RefUniformPair(u, uniform.second);
    onAddUniform(u);
}

//--------------------------------------------------------------------------
void Shader::del(const std::string& name)
{
    osg::StateSet::UniformList::iterator it = mUniforms.find(name);
    if (it != mUniforms.end()){
        onDelUniform(it->second.first.get());
        mUniforms.erase(it);
    }
}

//--------------------------------------------------------------------------
bool Shader::isUniformExists(const std::string& name)
{
    return get(name) != NULL;
}

//--------------------------------------------------------------------------
osg::Uniform* Shader::get(const std::string& name)
{
    osg::StateSet::UniformList::const_iterator it = mUniforms.begin();
    for (; it != mUniforms.end(); it++)
    {
        if (it->first == name) return it->second.first.get();    
    }
    return NULL;
}

//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, bool b0){
    osg::Uniform* u = get(name);
    if (!u) return false;
    u->setElement(index, b0);
    onSetUniform(u);
    return true;
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, bool b0, bool b1){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, b0, b1);
    onSetUniform(u);
    return true;
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, bool b0, bool b1, bool b2){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, b0,b1,b2);
    onSetUniform(u);
    return true;
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, bool b0, bool b1, bool b2, bool b3){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, b0,b1,b2,b3);
    onSetUniform(u);
    return true;
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, int i0){
    osg::Uniform* u = get(name);
    if (!u) return false;
    u->setElement(index, i0);   
    onSetUniform(u);
    return true;
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, int i0, int i1){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, i0,i1);
    onSetUniform(u);
    return true;
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, int i0, int i1, int i2){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, i0,i1,i2);
    onSetUniform(u);
    return true;
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, int i0, int i1, int i2, int i3){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, i0,i1,i2,i3);
    onSetUniform(u);
    return true;
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, float f0){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, f0);
    onSetUniform(u);
    return true;
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, float f0, float f1){
    return set(index, name, osg::Vec2(f0,f1));
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, float f0, float f1, float f2){
    return set(index, name, osg::Vec3(f0,f1,f2));
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, float f0, float f1, float f2, float f3){
    return set(index, name, osg::Vec4(f0,f1,f2,f3));
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, const osg::Vec2& v){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, v);
    onSetUniform(u);
    return true;
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, const osg::Vec3& v){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, v);
    onSetUniform(u);
    return true;
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, const osg::Vec4& v){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, v);
    onSetUniform(u);
    return true;
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, const osg::Matrix2& m){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, m);
    onSetUniform(u);
    return true;
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, const osg::Matrix3& m){
    osg::Uniform* u = get(name);
    if (!u) return false; u->setElement(index, m);
    onSetUniform(u);
    return true;
}
//--------------------------------------------------------------------------
bool Shader::set(unsigned int index, const std::string& name, const osg::Matrixf& m){
    osg::Uniform* u = get(name);
    if (!u) return false;
    u->setElement(index, m);
    onSetUniform(u);
    return true;
}

//--------------------------------------------------------------------------
bool Shader::bind(unsigned int index, const std::string& name, osg::Texture* t, int unit)
{   
    // check whenever such an uniform already exists, if not so add it
    if (!isUniformExists(name)) add(name, osg::Uniform::INT, 1);
    /*else{
        // non valid texture, so remove it
        if (t == NULL)
        {
            TexUnitDb::iterator it = mTexUnits.find(name);
            if (it != mTexUnits.end()) mTexUnits.erase(it);
            return false;
        }
    }*/

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
    mDirtyBoundings = true;

    //printf("%s bind %s-%d\n", getName().c_str(), name.c_str(), unit);
    
    // just set value if predefined
    if (unit >= 0)
    {
        set(index, name, (int)unit);
    }

    // inform derived classes  about this 
    onBindTexture(tu);
   
    return true;
}

//--------------------------------------------------------------------------
bool Shader::bindAttribute(const std::string& name, unsigned int index)
{
    if (!mProgram) return false;
    mProgram->addBindAttribLocation(name, index);
    onBindAttribute(name, index);
    return true;
}

//--------------------------------------------------------------------------
bool Shader::bindFragData(const std::string& name, unsigned int index)
{
    if (!mProgram) return false;
    mProgram->addBindFragDataLocation(name, index);
    onBindFragData(name, index);
    return true;
}

//--------------------------------------------------------------------------
void Shader::setUniformList(const osg::StateSet::UniformList& list)
{
    mUniforms = list;
    mDirtyBoundings = false;
}

//--------------------------------------------------------------------------
void Shader::enable(osg::StateSet* ss, bool updateBindings)
{    
    // reset all boundings
    if (mDirtyBoundings || updateBindings) resetBindings(ss);
       
    // add all uniforms from the list to the state
    ss->setUniformList(mUniforms);
    
    // setup callback function for this program
    mProgram->setUpdateCallback(new Shader::UpdateCallback(this, updateBindings ? ss : NULL));

    // enable the program
    ss->setAttributeAndModes(getProgram(), osg::StateAttribute::ON);
    
    // inform abotu enable state changing
    onEnable(ss);

    #if DEBUG_SH
    printf("Enable Shader %s\n", getName().c_str());
    osg::StateSet::UniformList::const_iterator jt = getUniformList().begin();
    for (; jt != getUniformList().end(); jt++)
    {
        float fval = -1.0;
        int ival = -1;
        if (jt->second.first->getType() == osg::Uniform::INT || jt->second.first->getType() == osg::Uniform::SAMPLER_2D)
        {
            jt->second.first->get(ival);
            printf("\t%s : %d \n", jt->first.c_str(), ival);//, (jt->second.second & osg::StateAttribute::ON) != 0);
        }else if (jt->second.first->getType() == osg::Uniform::FLOAT){
            jt->second.first->get(fval);
            printf("\t%s : %f \n", jt->first.c_str(), fval);//, (jt->second.second & osg::StateAttribute::ON) != 0);
        }
    }
    printf("\n");
    #endif
}

//--------------------------------------------------------------------------
void Shader::disable(osg::StateSet* ss)
{
    if (!ss) return;

    // remove update callback
    mProgram->setUpdateCallback(NULL);
    
    // disable the program
    ss->setAttributeAndModes(getProgram(), osg::StateAttribute::OFF);
    
    // we mark our bindings as dirty, so they will be updated by the next enable
    mDirtyBoundings = true;

    // inofrm about disablign the shader
    onDisable(ss);    
}

//--------------------------------------------------------------------------
void Shader::resetBindings(osg::StateSet* ss)
{    
    // do nothing if not valid stateset
    if (!ss) return;
    
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
    
    // remove all previous texture attributes
    //for (int i=0; i < mMaxTextureUnits; i++)
    //{
    //    ss->removeTextureAttribute(i, osg::StateAttribute::TEXTURE);
    //}
    
    #if DEBUG_SH    
    printf("%s-%s set:\n", getName().c_str(), mProgram->getName().c_str());
    #endif
    
    // now iterate through the units and set them
    for (unsigned int i=0; i < units.size(); i++)
    {
        if (units[i] != NULL)
        {
            #if DEBUG_SH    
            printf("\t%s[%d] = %d (%p)", units[i]->name.c_str(), units[i]->element, i, units[i]->t.get());
            #endif 

            // if texture specified so assign it
            if (units[i]->t.valid())
            {
                ss->setTextureAttribute(i, units[i]->t.get());// osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE | osg::StateAttribute::PROTECTED);
                
                #if DEBUG_SH    
                printf(" added (%dx%d - internal_fmt = 0x%x)", units[i]->t->getTextureWidth(), units[i]->t->getTextureHeight(), units[i]->t.get()->getInternalFormat());
                #endif

                // setup default texture properties
                if (isUniformExists("g_TextureWidth")) set(i, "g_TextureWidth", units[i]->t->getTextureWidth());
                if (isUniformExists("g_TextureHeight")) set(i, "g_TextureHeight", units[i]->t->getTextureHeight());
                if (isUniformExists("g_TextureDepth")) set(i, "g_TextureDepth", units[i]->t->getTextureDepth());
            }
            #if DEBUG_SH    
            printf("\n");
            #endif

            // set unit number for this uniform
            set(units[i]->element, units[i]->name, (int)i);
        }
    }

    #if DEBUG_SH    
    printf("done\n\n");
    #endif

    mDirtyBoundings = false;
}

//--------------------------------------------------------------------------
Shader::UpdateCallback::UpdateCallback(Shader* sh, osg::StateSet* ss) : osg::StateAttribute::Callback()
{
    mShader = sh;
    mStateSet = ss;
}

//--------------------------------------------------------------------------
void Shader::UpdateCallback::operator()(osg::StateAttribute* ss, osg::NodeVisitor* nv)
{    
    // if this is an update visitor
    //if (nv && nv->getVisitorType() == osg::NodeVisitor::UPDATE_VISITOR))
    {
        // check if texture bindings changed
        if (mShader->mDirtyBoundings && mStateSet.valid()) mShader->resetBindings(mStateSet.get());
        
        // do nothing if shader is not specified
        mShader->update();

        //printf("update shader: %s (%s): %d %d\n", mShader->getName().c_str(), mShader->getResourceName().c_str(), mShader->mDirtyBoundings, mStateSet.valid());
    }
}

}; //end namespace

