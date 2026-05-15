#include <cstdio>
#include <cuda_runtime.h>

__global__ void hello_kernel() {
    printf("GPU says hello from block %d thread %d\n", blockIdx.x, threadIdx.x);
}

int main() {
    int deviceCount;
    cudaGetDeviceCount(&deviceCount);
    printf("CUDA devices: %d\n", deviceCount);

    cudaDeviceProp prop;
    for (int i = 0; i < deviceCount; i++) {
        cudaGetDeviceProperties(&prop, i);
        printf("Device %d: %s, %.1f GB VRAM, compute %d.%d\n",
               i, prop.name, prop.totalGlobalMem / (1024.0 * 1024.0 * 1024.0),
               prop.major, prop.minor);
    }

    hello_kernel<<<2, 4>>>();
    cudaDeviceSynchronize();

    printf("CUDA works!\n");
    return 0;
}
