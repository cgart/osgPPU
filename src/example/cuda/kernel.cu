/*
 * Demonstration of an osgPPU Module which using a CUDA-Kernel to blur the input texture.
 */

#ifndef __KERNEL_H_
#define __KERNEL_H_


#define _DEBUG

// here workaround for broken CUDA compiler (should be solved with CUDA 2.2)
using namespace std;

#include <osgPPU/UnitInOutModule.h>
#include <cuda_gl_interop.h>
#include "cutil.h"

using namespace osgPPU;


//-----------------------------------------------------------------------------
// clamp x to range [a, b]
// GPU code
//-----------------------------------------------------------------------------
__device__ float clamp(float x, float a, float b)
{
    return max(a, min(b, x));
}

//-----------------------------------------------------------------------------
// get pixel from 2D image, with clamping to border
// GPU code
//-----------------------------------------------------------------------------
__device__ float4 getPixel(float4 *data, int x, int y, int width, int height)
{
    x = clamp(x, 0, width-1);
    y = clamp(y, 0, height-1);
    return data[y*width+x];
}

// macros to make indexing shared memory easier
#define SMEM(X, Y) sdata[(Y)*tilew+(X)]

//-----------------------------------------------------------------------------
// CUDA kernel to do a simple blurring
//-----------------------------------------------------------------------------
__global__ void blurKernel(float4* g_data, float4* g_odata, int imgw, int imgh, int tilew, int r, float threshold, float highlight)
{
    extern __shared__ float4 sdata[];

    int tx = threadIdx.x;
    int ty = threadIdx.y;
    int bw = blockDim.x;
    int bh = blockDim.y;
    int x = blockIdx.x*bw + tx;
    int y = blockIdx.y*bh + ty;

    // copy tile to shared memory
    // center region
    SMEM(r + tx, r + ty) = getPixel(g_data, x, y, imgw, imgh);

    // borders
    if (threadIdx.x < r) {
        // left
        SMEM(tx, r + ty) = getPixel(g_data, x - r, y, imgw, imgh);
        // right
        SMEM(r + bw + tx, r + ty) = getPixel(g_data, x + bw, y, imgw, imgh);
    }
    if (threadIdx.y < r) {
        // top
        SMEM(r + tx, ty) = getPixel(g_data, x, y - r, imgw, imgh);
        // bottom
        SMEM(r + tx, r + bh + ty) = getPixel(g_data, x, y + bh, imgw, imgh);
    }

    // load corners
    if ((threadIdx.x < r) && (threadIdx.y < r)) {
        // tl
        SMEM(tx, ty) = getPixel(g_data, x - r, y - r, imgw, imgh);
        // bl
        SMEM(tx, r + bh + ty) = getPixel(g_data, x - r, y + bh, imgw, imgh);
        // tr
        SMEM(r + bw + tx, ty) = getPixel(g_data, x + bh, y - r, imgw, imgh);
        // br
        SMEM(r + bw + tx, r + bh + ty) = getPixel(g_data, x + bw, y + bh, imgw, imgh);
    }

    // wait for loads to complete
    __syncthreads();

    // perform convolution
    float samples = 0.0;
    float3 pixelSum = make_float3(0,0,0);

    for(int dy=-r; dy<=r; dy++) {
        for(int dx=-r; dx<=r; dx++) {
#if 0
            // try this to see the benefit of using shared memory
            float4 pixel = getPixel(g_data, x+dx, y+dy, imgw, imgh);
#else
            float4 pixel = SMEM(r+tx+dx, r+ty+dy);
#endif
            // only sum pixels within disc-shaped kernel
            float l = dx*dx + dy*dy;
            if (l <= r*r)
            {
                pixelSum.x += pixel.x;
                pixelSum.y += pixel.y;
                pixelSum.z += pixel.z;
                samples += 1.0;
            }
        }
    }

    // normalize
    pixelSum.x /= samples;
    pixelSum.y /= samples;
    pixelSum.z /= samples;

    g_odata[y*imgw+x] = make_float4(pixelSum.x, pixelSum.y, pixelSum.z, 1.0);
}

//-----------------------------------------------------------------------------
// Class capable of rendering the stuff through cuda
//-----------------------------------------------------------------------------
class ProcessingModule : public UnitInOutModule::Module
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
                fprintf(stderr, "error: device does not support CUDA.\n");     
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
            CUDA_SAFE_CALL(cudaGLRegisterBufferObject(ipbo->buffer(0)));
            CUDA_SAFE_CALL(cudaGLMapBufferObject( (void**)&in_data, ipbo->buffer(0)));

            // map output data
            float4* out_data = NULL;
            CUDA_SAFE_CALL(cudaGLRegisterBufferObject(opbo->buffer(0)));
            CUDA_SAFE_CALL(cudaGLMapBufferObject( (void**)&out_data, opbo->buffer(0)));


            //-----------------------------------------------------------------------------
            // run CUDA Kernel on the loaded data

            // radius of how much sampling points around the current one        
            int radius = 4;

            // specifies the number of threads per block (here 16*16*1=256 threads)
            dim3 block(16, 16, 1);

            // specifies the number of blocks used
            dim3 grid(width / block.x, height / block.y, 1);

            // size of the shared memory (memory used by each block) reflects the size of sampling points around
            int sbytes = (block.x+(2*radius))*(block.y+(2*radius)) * sizeof(float4);
        
            // run kernel with the specified parameters
            blurKernel<<< grid, block, sbytes>>>(in_data, out_data, width, height, block.x+(2*radius), radius, 0.8f, 4.0f);

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
            CUDA_SAFE_CALL(cudaGLUnmapBufferObject(opbo->buffer(0)));
            CUDA_SAFE_CALL(cudaGLUnregisterBufferObject(opbo->buffer(0)));

            // map output data
            CUDA_SAFE_CALL(cudaGLUnmapBufferObject(ipbo->buffer(0)));
            CUDA_SAFE_CALL(cudaGLUnregisterBufferObject(ipbo->buffer(0)));
        }

};


//-----------------------------------------------------------------------------
osg::ref_ptr<ProcessingModule> g_ProcessingModule;

//-----------------------------------------------------------------------------
// Register the module by the corresponding UnitInOutModule, so that it can be
// used in former processing operations.
//-----------------------------------------------------------------------------
extern "C" bool OSGPPU_MODULE_ENTRY(UnitInOutModule* parent) 
{
    g_ProcessingModule = new ProcessingModule(parent);
    parent->setModule(g_ProcessingModule.get());

    return true;
}


#endif 

