/*
 * Demonstration of an osgPPU Module which using a CUDA-Kernel to blur the input texture.
 */

#ifndef __KERNEL_H_
#define __KERNEL_H_


//#define _DEBUG

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


void blurKernelWrapper(float4* in_data, float4* out_data, int width, int height, int radius, float threshold, float highlight)
{
    // run CUDA Kernel on the loaded data

    // specifies the number of threads per block (here 16*16*1=256 threads)
    dim3 block(16, 16, 1);

    // specifies the number of blocks used
    dim3 grid(width/block.x, height/block.y, 1);

    // size of the shared memory (memory used by each block) reflects the size of sampling points around
    int sbytes = (block.x+(2*radius))*(block.y+(2*radius)) * sizeof(float4);

    // run kernel with the specified parameters
    blurKernel<<< grid, block, sbytes>>>(in_data, out_data, width, height, block.x+(2*radius), radius, threshold, highlight);
}

#endif // __KERNEL_H_
