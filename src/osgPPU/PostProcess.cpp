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

#include <osgPPU/PostProcess.h>
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
PostProcess::PostProcess(osg::State* state) : mState(state) 
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
PostProcess::PostProcess(const PostProcess& pp, const osg::CopyOp& copyop) : 
    osg::Object(pp, copyop),
    mState(pp.mState),
    mStateSet(copyop(pp.mStateSet.get())),
    mCamera(pp.mCamera),
    mFXPipeline(pp.mFXPipeline),
    mTime(pp.mTime)
{

}

//------------------------------------------------------------------------------
PostProcess::~PostProcess(){

}

//------------------------------------------------------------------------------
/*void PostProcess::setState(osg::State* state)
{
    mState = state;
}*/

//------------------------------------------------------------------------------
void PostProcess::setCamera(osg::Camera* camera)
{
    // setup camera
    mCamera = camera;
    mStateSet->setAttribute(const_cast<osg::Viewport*>(mCamera->getViewport()), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    // we have to setup a post draw callback to get all things updated
    //mCamera->setPostDrawCallback(new Callback(this));
}

//------------------------------------------------------------------------------
void PostProcess::initPostDrawCallback(osg::Camera* camera)
{
    if (!camera) return;
    camera->setPostDrawCallback(new Callback(this));
}

//------------------------------------------------------------------------------
osg::StateSet* PostProcess::getOrCreateStateSet()
{
    if (!mStateSet.valid()) mStateSet = new osg::StateSet();
    return mStateSet.get();
}

//------------------------------------------------------------------------------
struct less_comparisonFX : std::less<osg::ref_ptr<PostProcessUnit> >
{
    public:
        bool operator () (const osg::ref_ptr<PostProcessUnit>& a, const osg::ref_ptr<PostProcessUnit>& b)
        {
            return (*a) < (*b);
        }
};

//------------------------------------------------------------------------------
void PostProcess::setPipeline(const FXPipeline& pipeline)
{
    // first we clean out our current pipeline
    mFXPipeline.clear();
    FXPipeline offlinePPUs;

    // iterate through the list 
    FXPipeline::const_iterator it = pipeline.begin();
    for (; it != pipeline.end(); it++)
    {
        // add the element into the pipeline
        if ((*it)->getOfflineMode() == false)
            mFXPipeline.push_back((*it));
        else
            offlinePPUs.push_back((*it));
    }
    
    // sort the pipeline
    mFXPipeline.sort(less_comparisonFX());

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
    FXPipeline::iterator jt = mFXPipeline.begin();
    for (; jt != mFXPipeline.end(); jt++)
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
PostProcessUnit* PostProcess::getPPU(const std::string& ppuName)
{
    // iterate through pipeline
    FXPipeline::iterator jt = mFXPipeline.begin();
    for (; jt != mFXPipeline.end(); jt++ )
        if ((*jt)->getName() == ppuName)
            return (*jt).get();

    return NULL;
}

//------------------------------------------------------------------------------
PostProcess::FXPipeline::iterator PostProcess::removePPUFromPipeline(const std::string& ppuName)
{
    // iterate through pipeline
    FXPipeline::iterator jt = mFXPipeline.begin();
    for (; jt != mFXPipeline.end(); jt++ )
    {
        if ((*jt)->getName() == ppuName)
        {
            // check if we have only one ppu, so just clean pipeline
            if (mFXPipeline.size()  <= 1)
            {
                mFXPipeline.clear();
                return mFXPipeline.end();
            }
         
			// if we have an offline ppu, then we just remove it from the list 
			if ((*jt)->getOfflineMode())
			{
				return mFXPipeline.erase(jt);
			}
   
            // we are not at the end
            if (jt != mFXPipeline.end()--)
            {
                // get next element
                FXPipeline::iterator it = jt; it++;

                // set input for the next from input of the current
                (*it)->setInputTextureMap((*jt)->getInputTextureMap());
                (*it)->setViewport((*jt)->getViewport());
                //(*it)->init();
                //onPPUInit((*it).get());

            // this is the last ppu, so just remove it
            }else{
                // get element before
                FXPipeline::iterator it = jt; it--;

                // remove outputs
                (*it)->setOutputTexture(NULL, 0);
                //(*it)->init();
                //onPPUInit((*it).get());
            }

            // remove ppu from pipeline
            return mFXPipeline.erase(jt);
        }
    }
    return mFXPipeline.end();
}

//------------------------------------------------------------------------------
void PostProcess::addPPUToPipeline(PostProcessUnit* ppu)
{
    // check for the case if pipeline is empty
    if (mFXPipeline.size() == 0) mFXPipeline.push_back(ppu);

    // offscreen ppus are inserted from the end
    if (ppu->getOfflineMode())
    {
        FXPipeline::reverse_iterator jt = mFXPipeline.rbegin();
        for (int i=0; jt != mFXPipeline.rend(); jt++, i++)
        {
            if ((*jt)->getOfflineMode() && (*jt)->getIndex() <= ppu->getIndex())
            {
                // create iterator pointing to this position
                FXPipeline::iterator it = mFXPipeline.begin(); for (int k=mFXPipeline.size(); k > i; k--) it++;
                mFXPipeline.insert(it++, ppu);
                return;

            // if we meet the first occurence of non offline, then add after it
            }else if ((*jt)->getOfflineMode() == false)
            {
                // create iterator pointing to this position
                FXPipeline::iterator it = mFXPipeline.begin(); for (int k=mFXPipeline.size(); k > i; k--) it++;
                mFXPipeline.insert(it++, ppu);
                return;
            }
        }
        return;
    }

    //printf("add ppu %s (%d)\n", ppu->getName().c_str(), ppu->getIndex());

    // add it based on index
    // iterate through the whole pipeline
    FXPipeline::iterator jt = mFXPipeline.begin();
    FXPipeline::iterator it = mFXPipeline.begin();
    for (++jt; jt != mFXPipeline.end(); jt++, it++)
    {
        //printf("%s (%d) ", (*it)->getName().c_str(), (*it)->getIndex());
        //printf("  -- %s (%d)\n", (*jt)->getName().c_str(), (*jt)->getIndex());

        // jt is the ppu after ours, it is the one before
        if ((*jt)->getIndex() > ppu->getIndex())
        {
            // get ppu before
            //FXPipeline::iterator it = jt;
           
            // input to this ppu is the output of the one before
            ppu->setInputTexture((*it)->getOutputTexture(0), 0);
            ppu->setViewport((*it)->getViewport());
            
            //printf("add ppu: %s --- %s --- %s\n", (*it)->getName().c_str(), ppu->getName().c_str(), (*jt)->getName().c_str());
            //printf("%fx%f\n", ppu->getViewport()->width(), ppu->getViewport()->height());

            // add the new ppu 
            mFXPipeline.insert(jt, ppu);
            
            return;
        }
    }
    
    // we are here, so no such place were found, so add at the end
    FXPipeline::reverse_iterator rit = mFXPipeline.rbegin();
    
    // input to this ppu is the output of the one before
    ppu->setInputTexture((*rit)->getOutputTexture(0), 0);
    ppu->setViewport((*rit)->getViewport());
    
    // add the new ppu 
    mFXPipeline.push_back(ppu);   
}

//------------------------------------------------------------------------------
void PostProcess::update(float dTime)
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
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_VIEWPORT_BIT | GL_TEXTURE_BIT | GL_TRANSFORM_BIT);
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
    // iterate through postprocessing units
    for (FXPipeline::iterator it = mFXPipeline.begin(); it != mFXPipeline.end(); it ++)
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
    glPopAttrib();
}


//--------------------------------------------------------------------------
GLenum PostProcess::createSourceTextureFormat(GLenum internalFormat)
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




