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

#ifndef _C_UNIT_H_
#define _C_UNIT_H_


//-------------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------------
#include <osg/Texture>
#include <osg/Geode>
#include <osg/Geometry>
#include <osgPPU/ColorAttribute.h>

#include <osgPPU/Export.h>
#include <osgPPU/Shader.h>

#define OSGPPU_VIEWPORT_WIDTH_UNIFORM "osgppu_ViewportWidth"
#define OSGPPU_VIEWPORT_HEIGHT_UNIFORM "osgppu_ViewportHeight"

namespace osgPPU
{

// Forward declaration to simplify the work
class Processor;
class Visitor;

//! Abstract base class of any unit
/**
 * Units represents renderable units of the osgPPU library.
 * Each unit has its own functionality. Units can be setted up
 * as a graph, where child units always get as input the output of the
 * parental units. Units has to be a subgraph of the Processor otherwise
 * the rendering wouldn't work, because processor do setup the units in a proper way.
 *
 **/
class OSGPPU_EXPORT Unit : public osg::Group {
    public:

        META_Node(osgPPU, Unit);

        typedef std::map<int, osg::ref_ptr<osg::Texture> > TextureMap;
        typedef std::vector<unsigned int> IgnoreInputList;
        typedef std::map<osg::ref_ptr<Unit>, std::pair<std::string, unsigned int> > InputToUniformMap;

        /**
        * Empty constructor. The unit will be initialized with default values.
        **/
        Unit();

        /**
         * Copy constructor.
        **/
        Unit(const Unit&, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY);

        /**
        * Release used memory by the ppu.
        **/
        virtual ~Unit();

        /**
        * Set an input from the given parent to be linked with the given
        * uniform name. This is required to automatically setup uniforms for
        * input textures of the assigned shader, which is based on the index
        * of the given parent unit in the parent list.
        * The type of the given uniform will be equivalent to the type of the
        * input texture (e.g. SAMPLER_2D = Texture2D).
        * @param parent Pointer to the parent which output to use
        * @param uniform Name of the uniform to use to bind the texture to
        * @param add if true will add the given parent to the parent list
        *             (same as calling parent->addChild()) [default=false]
        * @return true if uniform is set or false otherwise
        **/
        bool setInputToUniform(Unit* parent, const std::string& uniform, bool add = false);

        /**
        * Set input texture of this unit as uniform of this unit.
        * @param index Input texture index to be used as input
        * @param uniform Name of the uniform
        **/
        bool setInputToUniform(int index, const std::string& uniform);

        /**
        * Remove an assigned parent output uniform. @see assignParentToUniform()
        * @param uniform Name of the uniform
        * @param del Should this unit be removed from the child list of the parent
        * connected with the given uniform [default=false]
        **/
        void removeInputToUniform(const std::string& uniform, bool del = false);

        /**
        * Remove an assigned parent output uniform. @see assignParentToUniform()
        * @param parent Pointer to the parent node
        * @param del Should this unit be removed from the child list of this parent [default=false]
        **/
        void removeInputToUniform(Unit* parent, bool del = false);

        /**
        * Get the map which maps uniform to input units
        **/
        inline const InputToUniformMap& getInputToUniformMap() const { return mInputToUniformMap; }

        /**
         * Return an input texture of a certain index.
         * @param inputIndex Index of the input texture (index is equal to the texture unit)
        **/
        inline osg::Texture* getInputTexture(int inputIndex) { return mInputTex[inputIndex].get(); }

        /**
        * Return complete index to texture mapping
        **/
        const TextureMap& getInputTextureMap() const {return mInputTex;}

        /**
        * Get output texture of certain MRT index.
        * NOTE: If you haven't initialized the Unit before calling this method
        * it might end up in a NULL as output texture. For this purpose do use
        * the getOrCreateOutputTexture().
        **/
        inline osg::Texture* getOutputTexture(int mrt = 0) { return mOutputTex[mrt].get(); }

        /**
        * Return an output texture of the certain MRT index.
        * If there is no output texture for that index is specified, then
        * it will be allocated. The method should be overwriten by the derived
        * Units if they use any Output texture. Otherwise the result is always
        * the same as from getOutputTexture().
        **/
        virtual osg::Texture* getOrCreateOutputTexture(int mrt = 0) { return getOutputTexture(mrt); }

        /**
        * Get mrt index to texture mapping
        **/
        inline const TextureMap& getOutputTextureMap() const {return mOutputTex;}

        /**
        * If you like that a unit doesn't use a certain input you can specify its index here.
        * This allows to place units on certain positions in the unit graph without using its
        * parents as inputs.
        **/
        void setIgnoreInput(unsigned int index, bool b = true);

        /**
        * Get input ignorance map which list all indices of input data which are ignored
        **/
        const IgnoreInputList& getIgnoreInputList() const { return mIgnoreList; }

        /**
        * Check whenever the input of the given index is ignored or not
        **/
        bool getIgnoreInput(unsigned int index) const;

        /**
        * Initialze the unit. This method should be overwritten by the
        * derived classes to support non-standard initialization routines.
        * If an unit is marked as dirty this method will be used to resetup the unit.
        * Hence do provide a "reinitialable"-code here ;-)
        **/
        virtual void init();

        /**
        * Update the unit. Call this method every time you want to update
        * the unit. It is a good idea to call this method every frame otherwise
        * the behaviour of the unit might be unpredictable.
        **/
        virtual void update();

        /**
        * Set viewport which is used for this Unit while rendering
        **/
        void setViewport(osg::Viewport* vp);

        /**
        * Get viewport of this unit
        **/
        inline osg::Viewport* getViewport() const { return mViewport.get(); }

        /**
         * Activate or deactive the ppu. An active ppu is updated during the update
         * of the post processor.
         * @param b True to activate, false to deactive
        **/
        inline void setActive(bool b) { mbActive = b; }

        /**
        * Check if the Unit's active flag
        **/
        inline bool getActive() const { return mbActive; }

        /**
         * Change drawing position and size of this ppu by using the
         * new frustum planes in the orthogonal projection matrix.
         * This changes the projection matrix,
         * therefor it is better not to change this parameters until you really
         * need this. If you just want to place the ppu on another position, then just
         * play with the viewport.
        **/
        void setRenderingFrustum(float left, float top, float right, float bottom);

        /**
        * Assign a shader used when computing the output data of the ppu.
        * Shaders are one of the main aspects of the ppu rendering.
        * @param sh Shader used by this ppu to generate output from the input.
        * DEPRECATED !!!!
        **/
        inline void setShader(Shader* sh) { mShader = sh; dirty(); }

        /**
        * Get currently assigned shader.
        * DEPRECATED !!!!
        **/
        inline Shader* getShader() const { return mShader.get(); }

        /**
        * Set index of an input texture which size is used as reference
        * for the viewport size. The viewport size will be changed according
        * to the texture size. If you change the input texture the size will
        * be also changed. Specify -1 if you do not want to have this behaviour.
        * If -1, then by next change of the input texture the viewport size
        * will not be changed.
        **/
        void setInputTextureIndexForViewportReference(int index);

        /**
        * Get index of the input texture which dimension is used for setting up the viewport.
        **/
        inline int getInputTextureIndexForViewportReference() const { return mInputTexIndexForViewportReference; }

        /**
        * Mark this unit as dirty. This will force it to resetup its data
        * on next update. Also every child unit will be marked as dirty. This yields of
        * reinitialization of children units on next update method too.
        **/
        void dirty();

        /**
        * Checks whenever the unit is marked as dirty or not.
        **/
        inline bool isDirty() const { return mbDirty; }

        /**
        * Get geode to which the unit's drawables are attached. The geodes
        * are used to render the unit.
        **/
        osg::Geode* getGeode() { return mGeode.get(); }
        const osg::Geode* getGeode() const { return mGeode.get(); }

        /**
        * Setup children nodes if they are connected by a barrier node.
        * This method don't need to be called outside of the unit. The method
        * is placed here to allow acess to it from the derived units.
        **/
        void setupBlockedChildren();

        /**
        * Set a new color attribute for this unit. This will replace the color attribute
        * if it is already set. The color attribute can be used to bind a color to
        * an unit. Very useful for blending units.
        **/
        void setColorAttribute(ColorAttribute* ca);

        /**
        * Get current color attribute of the unit.
        **/
        ColorAttribute* getColorAttribute() { return mColorAttribute.get(); }
        const ColorAttribute* getColorAttribute() const { return mColorAttribute.get(); }

    protected:

        /**
        * This draw callback is used to setup correct drawing
        * of the unit's drawable. The callback is setted up automatically,
        * hence you don't need to do anything.
        **/
        class DrawCallback : public osg::Drawable::DrawCallback
        {
            public:
                DrawCallback(Unit* parent) : osg::Drawable::DrawCallback(), _parent(parent) {}
                ~DrawCallback() {}

                inline void setParent(Unit* parent) { _parent = parent; }
                inline Unit* getParent() { return _parent; }
                inline const Unit* getParent() const { return _parent; }
                void drawImplementation (osg::RenderInfo& ri, const osg::Drawable* dr) const;

            private:
                Unit* _parent;
        };

        /**
        * Use this method in the derived classes to implement and update some unit
        * specific uniforms. The base class do only update uniforms like viewport size
        * or, if defined, input texture indices.
        **/
        virtual void updateUniforms();

        /**
        * Setup the input textures based on the parents. Each unit has to setup its
        * input textures properly. This method do scan for all parents up to the Processor
        * and use the output textures of that parents units and the processor as
        * input to this unit. Call this method from derived units to setup inputs properly.
        **/
        virtual void setupInputsFromParents();

        //! Method to let the unit know that the rendering will now beginns
        virtual void  noticeBeginRendering (osg::RenderInfo&, const osg::Drawable* ) {};

        //! Let the unit know that the drawing is done.
        virtual void  noticeFinishRendering(osg::RenderInfo&, const osg::Drawable* ) {};

        //! Notice underlying classes, that viewport size is changed
        virtual void noticeChangeViewport() {}

        //! Notice derived classes, when inpu ttexture was changed.
        virtual void noticeChangeInput() {}

        //! Notice derived classes, that new shader is assigned
        virtual void noticeAssignShader() {}

        //! Notice derived classes, that no shader is assigned now
        virtual void noticeRemoveShader() {}

        //! Assign the input texture to the quad object
        void assignInputTexture();

        //! Assign a shader to the input texture to the quad object
        void assignShader();

        //! Assign currently choosen viewport to the stateset
        void assignViewport();

        //! disable shader
        void removeShader();

        //! Helper function to create screen sized quads
        osg::Drawable* createTexturedQuadDrawable(const osg::Vec3& corner = osg::Vec3(0,0,0),const osg::Vec3& widthVec=osg::Vec3(1,0,0),const osg::Vec3& heightVec=osg::Vec3(0,1,0), float l=0.0, float b=0.0, float r=1.0, float t=1.0);

        //! Input texture
        TextureMap  mInputTex;

        //! Output textures
        TextureMap  mOutputTex;

        //! List of ignored inputs
        IgnoreInputList mIgnoreList;

        //! Map of the uniform to parent links
        InputToUniformMap mInputToUniformMap;

        //! Shader which will be used for rendering
        osg::ref_ptr<Shader>   mShader;

        //! Here we store a screen sized quad, so it can be used for rendering
        osg::ref_ptr<osg::Drawable> mDrawable;

        //! Projection matrix of the ppu (default: 2D ortho view)
        osg::ref_ptr<osg::RefMatrix> sProjectionMatrix;

        //! Modelview matrix of the ppu (default: identity matrix)
        osg::ref_ptr<osg::RefMatrix> sModelviewMatrix;

        //! Store here the viewport of the camera, to which one this PPUs are applied
        osg::ref_ptr<osg::Viewport> mViewport;

        //! This geode is used to setup the unit's drawable
        osg::ref_ptr<osg::Geode> mGeode;

        //! Color attribute for fast direct access
        osg::ref_ptr<ColorAttribute> mColorAttribute;

        //! Is the unit dirty
        bool mbDirty;

        //! Index of the input texture which size is used as viewport
        int mInputTexIndexForViewportReference;

    private:
        bool mbActive;

        // Separate both folowing variables to allow update and cull traversal in different threads
        bool mbUpdateTraversed; // requires to check whenever unit was already traversed by update visitor
        bool mbCullTraversed; // requires to check whenever unit was already traversed by cull visitor

        void printDebugInfo(const osg::Drawable* dr);
        void traverse(osg::NodeVisitor& nv);

        // it is good to have friends
        friend class Processor;
        friend class Pipeline;
        friend class CleanUpdateTraversedVisitor;
        friend class CleanCullTraversedVisitor;
        friend class SetMaximumInputsVisitor;
};

};

#endif
