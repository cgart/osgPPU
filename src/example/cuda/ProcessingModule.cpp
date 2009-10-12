/*
 * Demonstration of an osgPPU Module which using a CUDA-Kernel to blur the input texture.
 */

#ifndef __PROCESSING_MODUL_H_
#define __PROCESSING_MODUL_H_


//#define _DEBUG

#include "Export.h"
#include <osgPPU/UnitInOutModule.h>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#include "cutil.h"
#include <cstdlib>

using namespace osgPPU;



extern void blurKernelWrapper(float4* in_data, float4* out_data, int width, int height, int radius, float threshold, float highlight);



//-----------------------------------------------------------------------------
// Class capable of rendering the stuff through cuda
//-----------------------------------------------------------------------------
class OSGPPU_CUDAK_EXPORT ProcessingModule : public UnitInOutModule::Module
{
    public:
        ProcessingModule(UnitInOutModule* parent) : UnitInOutModule::Module(parent)
        {
            // to get all thing properly we have to specify one input and one output pbo
            parent->setUsePBOForInputTexture(0);
            parent->setUsePBOForOutputTexture(0);

            osg::notify(osg::INFO) << "osgPPU - Module - cudaKernel initialize" << std::endl;
        }

        ~ProcessingModule()
        {
            // force exit of CUDA
            cudaThreadSynchronize();
            cudaThreadExit();
            osg::notify(osg::INFO) << "osgPPU - Module - cudaKernel release" << std::endl;
        }

        //-----------------------------------------------------------------------------
        // Initialize cuda environment
        //-----------------------------------------------------------------------------
        bool init()
        {
            int deviceCount;                                                         
            CUDA_SAFE_CALL_NO_SYNC(cudaGetDeviceCount(&deviceCount));                
            if (deviceCount == 0) {                                                  
                fprintf(stderr, "error: no devices supporting CUDA.\n");       
                exit(EXIT_FAILURE);                                                  
            }                                                                        
            int dev = 0;                                                             
            if (dev > deviceCount-1) dev = deviceCount - 1;                          
            cudaDeviceProp deviceProp;                                               
            CUDA_SAFE_CALL_NO_SYNC(cudaGetDeviceProperties(&deviceProp, dev));       
            if (deviceProp.major < 1) {                                              
                fprintf(stderr, "error: device does not support CUDA, deviceProp(%d): %d.%d\n", dev, deviceProp.major, deviceProp.minor);     
                exit(EXIT_FAILURE);                                                  
            }                                                                        
            CUDA_SAFE_CALL(cudaSetDevice(dev));

            // print some debug info
            printf("Cuda BlurKernel Module for osgPPU:\n");
            printf("\tDevice: %s\n", deviceProp.name);
            printf("\tTotal Memory: %d MB\n", deviceProp.totalGlobalMem/1000000);
            printf("\tClock Rate: %d MHz\n", deviceProp.clockRate/1000);
            printf("\tMultiprocessors: %d\n", deviceProp.multiProcessorCount);
            printf("\tShared Mem per Block: %d\n", deviceProp.sharedMemPerBlock);
            printf("\tMax Threads per Block: %d\n", deviceProp.maxThreadsPerBlock);
            printf("\tCompute Capability: %d.%d\n", deviceProp.major, deviceProp.minor);

            return true;
        }


        //-----------------------------------------------------------------------------
        // Register/Map textures into CUDA space and process them
        //-----------------------------------------------------------------------------
        bool beginAndProcess()
        {
            // get first input pbo
            const osg::PixelDataBufferObject* ipbo = _parent->getInputPBO(0);
            const osg::PixelDataBufferObject* opbo = _parent->getOutputPBO(0);
            if (ipbo == NULL || opbo == NULL) return false;

            // get dimensions of the input data
            int width = _parent->getViewport() ? (int)_parent->getViewport()->width() : 0;
            int height = _parent->getViewport() ? (int)_parent->getViewport()->height() : 0;
            if (width == 0 || height == 0) return false;

            // map input data
            float4* in_data = NULL;
            CUDA_SAFE_CALL(cudaGLRegisterBufferObject(ipbo->getOrCreateGLBufferObject(0)->getGLObjectID()));
            CUDA_SAFE_CALL(cudaGLMapBufferObject( (void**)&in_data, ipbo->getGLBufferObject(0)->getGLObjectID()));

            // map output data
            float4* out_data = NULL;
            CUDA_SAFE_CALL(cudaGLRegisterBufferObject(opbo->getOrCreateGLBufferObject(0)->getGLObjectID()));
            CUDA_SAFE_CALL(cudaGLMapBufferObject( (void**)&out_data, opbo->getGLBufferObject(0)->getGLObjectID()));


            //-----------------------------------------------------------------------------
            // run CUDA Kernel on the loaded data

            // radius of how much sampling points around the current one        
            int radius = 4;

            // run kernel with the specified parameters
            blurKernelWrapper(in_data, out_data, width, height, radius, 0.8f, 4.0f);
            //-----------------------------------------------------------------------------

            // don't render anything afterwards
            return false;
        }


        //-----------------------------------------------------------------------------
        // Unmap/Unregister data, so that results are copied back
        //-----------------------------------------------------------------------------
        void end()
        {
            // get first input pbo
            const osg::PixelDataBufferObject* ipbo = _parent->getInputPBO(0);
            const osg::PixelDataBufferObject* opbo = _parent->getOutputPBO(0);
            if (ipbo == NULL || opbo == NULL) return;

            // unmap and unregister input data
            CUDA_SAFE_CALL(cudaGLUnmapBufferObject(opbo->getGLBufferObject(0)->getGLObjectID()));
            CUDA_SAFE_CALL(cudaGLUnregisterBufferObject(opbo->getGLBufferObject(0)->getGLObjectID()));

            // map output data
            CUDA_SAFE_CALL(cudaGLUnmapBufferObject(ipbo->getGLBufferObject(0)->getGLObjectID()));
            CUDA_SAFE_CALL(cudaGLUnregisterBufferObject(ipbo->getGLBufferObject(0)->getGLObjectID()));
        }

};


//-----------------------------------------------------------------------------
osg::ref_ptr<ProcessingModule> g_ProcessingModule;

//-----------------------------------------------------------------------------
// Register the module by the corresponding UnitInOutModule, so that it can be
// used in former processing operations.
//-----------------------------------------------------------------------------
extern "C" bool OSGPPU_CUDAK_EXPORT OSGPPU_MODULE_ENTRY(UnitInOutModule* parent) 
{
    osg::notify(osg::INFO) << "osgPPU - Module - Registering the module by the corresponding UnitInOutModule" << std::endl;

    g_ProcessingModule = new ProcessingModule(parent);
    parent->setModule(g_ProcessingModule.get());

    return true;
}

#endif // __PROCESSING_MODUL_H_
