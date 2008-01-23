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

#include <osgPPU/Processor.h>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osg/Image>
#include <osg/Texture2D>
#include <osg/Depth>

#include <assert.h>


#define DEBUG_PPU 0

namespace osgPPU
{

//------------------------------------------------------------------------------
Processor::Processor(osg::State* state) : mState(state)
{
    mTime = 0.0f;

    // create default state for post processing effects
    //if (state == NULL) mState = new osg::State();
    assert(state != NULL);
    mStateSet = new osg::StateSet();
    
    // setup default state set 
    mStateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
    mStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    mStateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    mStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

    // we should not write to the depth buffer 
    osg::Depth* ds = new osg::Depth();
    ds->setWriteMask(false);
    mStateSet->setAttribute(ds, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
}

//------------------------------------------------------------------------------
Processor::Processor(const Processor& pp, const osg::CopyOp& copyop) :
    osg::Object(pp, copyop),
    mState(pp.mState),
    mStateSet(copyop(pp.mStateSet.get())),
    mCamera(pp.mCamera),
    mPipeline(pp.mPipeline),
    mTime(pp.mTime)
{

}

//------------------------------------------------------------------------------
Processor::~Processor(){

}

//------------------------------------------------------------------------------
/*void Processor::setState(osg::State* state)
{
    mState = state;
}*/

//------------------------------------------------------------------------------
void Processor::setCamera(osg::Camera* camera)
{
    // setup camera
    mCamera = camera;
    mStateSet->setAttribute(const_cast<osg::Viewport*>(mCamera->getViewport()), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    // we have to setup a post draw callback to get all things updated
    //mCamera->setPostDrawCallback(new Callback(this));
}

//------------------------------------------------------------------------------
void Processor::initPostDrawCallback(osg::Camera* camera)
{
    if (!camera) return;
    camera->setPostDrawCallback(new Callback(this));
}

//------------------------------------------------------------------------------
osg::StateSet* Processor::getOrCreateStateSet()
{
    if (!mStateSet.valid()) mStateSet = new osg::StateSet();
    return mStateSet.get();
}

//------------------------------------------------------------------------------
struct less_comparisonFX : std::less<osg::ref_ptr<Unit> >
{
    public:
        bool operator () (const osg::ref_ptr<Unit>& a, const osg::ref_ptr<Unit>& b)
        {
            return (*a) < (*b);
        }
};

//------------------------------------------------------------------------------
void Processor::setPipeline(const Pipeline& pipeline)
{
    // first we clean out our current pipeline
    mPipeline.clear();
    Pipeline offlinePPUs;

    // iterate through the list 
    Pipeline::const_iterator it = pipeline.begin();
    for (; it != pipeline.end(); it++)
    {
        // add the element into the pipeline
        if ((*it)->getOfflineMode() == false)
            mPipeline.push_back((*it));
        else
            offlinePPUs.push_back((*it));
    }
    
    // sort the pipeline
    mPipeline.sort(less_comparisonFX());

    // now combine the output and inputs of the pipeline
    // This is done by set camera texture as input for the first unit 
    // and combining hte output texture of ppu_i with the input texture
    // of ppu_i+1
    
    // get texture attachment from the camera
    osg::Camera::BufferAttachmentMap& map = mCamera->getBufferAttachmentMap();

    // save pointer to the previous texture
    osg::Texture* input = map[osg::Camera::COLOR_BUFFER]._texture.get();
    osg::Viewport* vp = const_cast<osg::Viewport*>(mCamera->getViewport());
        
    // iterate through the whole pipeline
    Pipeline::iterator jt = mPipeline.begin();
    for (; jt != mPipeline.end(); jt++)
    {
		// check if we have an online ppu, then do connect it
		if ((*jt)->getOfflineMode() == false)
		{
			// set input texture
			(*jt)->setInputTexture(input, 0);
			(*jt)->setCamera(mCamera.get());

			// setup default settings
			(*jt)->setViewport(vp);
            if (onPPUInit((*jt).get()))
    			(*jt)->init();
	
			// set now the input texture for the next from the output tex of the current one
			input = (*jt)->getOutputTexture(0);
			vp = (*jt)->getViewport();
        }
    }
    
    // add now offline ppus
    for (jt = offlinePPUs.begin(); jt != offlinePPUs.end(); jt++)
    {
        // check if we have an online ppu, then do connect it 
        if ((*jt)->getOfflineMode() == true)
        {
            if (onPPUInit((*jt).get()))
                (*jt)->init();            
            addPPUToPipeline((*jt).get());
        }
    }
}

//------------------------------------------------------------------------------
Unit* Processor::getPPU(const std::string& ppuName)
{
    // iterate through pipeline
    Pipeline::iterator jt = mPipeline.begin();
    for (; jt != mPipeline.end(); jt++ )
        if ((*jt)->getName() == ppuName)
            return (*jt).get();

    return NULL;
}

//------------------------------------------------------------------------------
Pipeline::iterator Processor::removePPUFromPipeline(const std::string& ppuName)
{
    // iterate through pipeline
    Pipeline::iterator jt = mPipeline.begin();
    for (; jt != mPipeline.end(); jt++ )
    {
        if ((*jt)->getName() == ppuName)
        {
            // check if we have only one ppu, so just clean pipeline
            if (mPipeline.size()  <= 1)
            {
                mPipeline.clear();
                return mPipeline.end();
            }
         
			// if we have an offline ppu, then we just remove it from the list 
			if ((*jt)->getOfflineMode())
			{
				return mPipeline.erase(jt);
			}
   
            // we are not at the end
            if (jt != mPipeline.end()--)
            {
                // get next element
                Pipeline::iterator it = jt; it++;

                // set input for the next from input of the current
                (*it)->setInputTextureMap((*jt)->getInputTextureMap());
                (*it)->setViewport((*jt)->getViewport());
                //(*it)->init();
                //onPPUInit((*it).get());

            // this is the last ppu, so just remove it
            }else{
                // get element before
                Pipeline::iterator it = jt; it--;

                // remove outputs
                (*it)->setOutputTexture(NULL, 0);
                //(*it)->init();
                //onPPUInit((*it).get());
            }

            // remove ppu from pipeline
            return mPipeline.erase(jt);
        }
    }
    return mPipeline.end();
}

//------------------------------------------------------------------------------
void Processor::addPPUToPipeline(Unit* ppu)
{
    // check for the case if pipeline is empty
    if (mPipeline.size() == 0) mPipeline.push_back(ppu);

    // offscreen ppus are inserted from the end
    if (ppu->getOfflineMode())
    {
        // iterate through the whole pipeline        
        bool foundPlace = false;
        for(Pipeline::iterator it = mPipeline.begin(); it!=mPipeline.end(); ++it)
        {
            // do add only if it is an offline ppu and we met a first ppu with bigger index
            if ((*it)->getOfflineMode() && (*it)->getIndex() > ppu->getIndex())
            {
                mPipeline.insert(it, ppu);
                foundPlace = true;
            }
        }
        
        // if we haven't found any right place, then just add at the end of the pipeline
        if(!foundPlace)
        {
            mPipeline.push_back(ppu);
            return;
        }
    }

    // add it based on index
    // iterate through the whole pipeline
    Pipeline::iterator jt = mPipeline.begin();
    Pipeline::iterator it = mPipeline.begin();
    for (++jt; jt != mPipeline.end(); jt++, it++)
    {
        // jt is the ppu after ours, it is the one before
        if ((*jt)->getIndex() > ppu->getIndex())
        {
            // input to this ppu is the output of the one before
            ppu->setInputTexture((*it)->getOutputTexture(0), 0);
            ppu->setViewport((*it)->getViewport());
            
            // add the new ppu 
            mPipeline.insert(jt, ppu);
            
            return;
        }
    }
    
    // we are here, so no such place were found, so add at the end
    Pipeline::reverse_iterator rit = mPipeline.rbegin();
    
    // input to this ppu is the output of the one before
    ppu->setInputTexture((*rit)->getOutputTexture(0), 0);
    ppu->setViewport((*rit)->getViewport());
    
    // add the new ppu 
    mPipeline.push_back(ppu);
}

//------------------------------------------------------------------------------
void Processor::update(float dTime)
{
    mTime += dTime;

    float texmat[16];
    float projmat[16];
    float mvmat[16];
    
    // save current matricies
    glGetFloatv(GL_TEXTURE_MATRIX, texmat);
    glGetFloatv(GL_PROJECTION_MATRIX, projmat);
    glGetFloatv(GL_MODELVIEW_MATRIX, mvmat);

    // push some attributes, TODO: should be avoided since we are using StateSet's
    //glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_VIEWPORT_BIT | GL_TEXTURE_BIT | GL_TRANSFORM_BIT);
    //glClearColor(0,1,0,0);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glDepthMask(GL_FALSE);

    // apply state set 
    mState->apply(mStateSet.get());

    #if DEBUG_PPU    
    printf("--------------------------------------------------------------------\n");
    printf("Start pipeline %s (%f)\n", getName().c_str(), mTime);
    printf("--------------------------------------------------------------------\n");
    #endif
    // iterate through Processoring units
    for (Pipeline::iterator it = mPipeline.begin(); it != mPipeline.end(); it ++)
    {
        if ((*it)->isActive())
        {
            #if DEBUG_PPU
            printf("%s (%d):\n", (*it)->getName().c_str(), (*it)->getIndex());
            printf("\t vp (ref %d): %d %d %d %d\n", (*it)->getInputTextureIndexForViewportReference(), (int)(*it)->getViewport()->x(), (int)(*it)->getViewport()->y(),(int)(*it)->getViewport()->width(), (int)(*it)->getViewport()->height());
            printf("\t alpha: %f (%f %f)\n", (*it)->getCurrentBlendValue(), (*it)->getStartBlendValue(), (*it)->getEndBlendValue());
            printf("\t time: %f-%f\n", (*it)->getStartBlendTime(), (*it)->getEndBlendTime());//, Engine::sClock()->getTime());
            printf("\t shader: %p\n", (*it)->getShader());

            if ((*it)->getShader() != NULL)
            {
                osg::StateSet::UniformList::const_iterator jt = (*it)->getShader()->getUniformList().begin();
                for (; jt != (*it)->getShader()->getUniformList().end(); jt++)
                {
                    float fval = -1.0;
                    int ival = -1;
                    if (jt->second.first->getType() == osg::Uniform::INT || jt->second.first->getType() == osg::Uniform::SAMPLER_2D)
                    {
                        jt->second.first->get(ival);
                        printf("\t\t%s : %d \n", jt->first.c_str(), ival);//, (jt->second.second & osg::StateAttribute::ON) != 0);
                    }else if (jt->second.first->getType() == osg::Uniform::FLOAT){
                        jt->second.first->get(fval);
                        printf("\t\t%s : %f \n", jt->first.c_str(), fval);//, (jt->second.second & osg::StateAttribute::ON) != 0);
                    }
                }
            }

			printf("\t input: ");
			for (unsigned int i=0; i < (*it)->getInputTextureMap().size(); i++)
			{
                osg::Texture* tex = (*it)->getInputTexture(i);
                printf(" %p", tex);
                if (tex)
                {
                    if ((*it)->getStateSet()->getTextureAttribute(i, osg::StateAttribute::TEXTURE))
                        printf("-attr");
                    printf(" (%dx%d), ", tex->getTextureWidth(), tex->getTextureHeight());
                }
            }

			printf("\n\t output: ");
			for (unsigned int i=0; i < (*it)->getOutputTextureMap().size(); i++)
			{
                osg::Texture* tex = (*it)->getOutputTexture(i);
				printf(" %p ", tex);
                if (tex) printf("(%dx%d)", tex->getTextureWidth(), tex->getTextureHeight());
            }

            printf("\n");
			#endif

            // apply the post processing unit
            if (onPPUApply(it->get()))
            {
                (*it)->setTime(mTime);
                (*it)->apply(0.0f);
            }

            // restore the matricies 
            glMatrixMode(GL_TEXTURE); glLoadMatrixf(texmat);
            glMatrixMode(GL_PROJECTION); glLoadMatrixf(projmat);
            glMatrixMode(GL_MODELVIEW); glLoadMatrixf(mvmat);
        }
        
    }
    #if DEBUG_PPU
    printf("\n\n");
    #endif

    // give me all things back
    //glPopAttrib();
}


//--------------------------------------------------------------------------
GLenum Processor::createSourceTextureFormat(GLenum internalFormat)
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


}; //end namespace




