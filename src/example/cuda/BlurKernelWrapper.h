#ifndef _BLUR_KERNEL_WRAPPER_
#define _BLUR_KERNEL_WRAPPER_


#include <cuda.h>
#include <cuda_runtime.h>


extern "C" void blurKernelWrapper(float4* in_data, float4* out_data, int width, int height, int radius, float threshold, float highlight);


#endif // _CUDA_KERNEL_WRAPPER_
