#include "kernel.cuh"
#include "PathTracing.cuh"
#include <cstdio>
#include <cuda_runtime.h>

// ============================================================
//  GPU Kernel：每个线程一个像素
// ============================================================
__global__ void renderKernel(
	const GPUBVHNode*   nodes, int rootIdx,
	const GPUTriangle*  triangles,
	const GPUSphere*    spheres,
	const GPUMaterial*  materials,
	const int*          lightTriIndices, int lightCount,
	int width, int height, int spp, int maxDepth,
	float eyeX, float eyeY, float eyeZ,
	float halfFOV,
	float3* framebuffer,
	unsigned long long* randSeeds)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	int totalPix = width * height;
	if (idx >= totalPix) return;

	int i = idx / width;   // 行
	int j = idx % width;   // 列

	// 初始化随机数
	RNGState rng;
	rng.state = randSeeds[idx];

	// 像素坐标 → 光线方向
	float eryPixer = halfFOV / (float)(height / 2);
	float x = (float)j * eryPixer + (eryPixer / 2.0f);
	float y = (float)i * eryPixer + (eryPixer / 2.0f);
	y = 2.0f * halfFOV - y;
	x = x - halfFOV;
	y = y - halfFOV;

	Vector3f eyePos(eyeX, eyeY, eyeZ);
	Vector3f color(0.0f);

	for (int k = 0; k < spp; k++) {
		Vector3f dir(x, y, 0.6f);
		Ray ray(eyePos, dir);
		color = color + PathTracingGPU(
			nodes, rootIdx, triangles, spheres, materials,
			lightTriIndices, lightCount,
			ray, maxDepth, rng);
	}

	color = color / (float)spp;

	// Gamma 2.5
	color.x = powf(fminf(1.0f, fmaxf(0.0f, color.x)), 0.4f);
	color.y = powf(fminf(1.0f, fmaxf(0.0f, color.y)), 0.4f);
	color.z = powf(fminf(1.0f, fmaxf(0.0f, color.z)), 0.4f);

	framebuffer[idx] = make_float3(color.x, color.y, color.z);
}

// ============================================================
//  Host 端封装
// ============================================================
void cudaRender(
	const std::vector<GPUTriangle>& cpuTriangles,
	const std::vector<GPUSphere>&    cpuSpheres,
	const std::vector<GPUMaterial>&  cpuMaterials,
	const std::vector<GPUBVHNode>&   cpuBVHNodes,
	const std::vector<int>&          lightTriIndices,
	int width, int height, int spp, int maxDepth,
	float eyeX, float eyeY, float eyeZ,
	float halfFOV,
	const char* outputPath)
{
	int totalPix = width * height;

	// ---- GPU 内存分配 ----
	GPUBVHNode*  d_nodes;
	GPUTriangle* d_triangles;
	GPUSphere*   d_spheres;
	GPUMaterial* d_materials;
	int*         d_lightIndices;
	float3*      d_framebuffer;
	unsigned long long* d_randSeeds;

	cudaMalloc(&d_nodes,       cpuBVHNodes.size()  * sizeof(GPUBVHNode));
	cudaMalloc(&d_triangles,   cpuTriangles.size() * sizeof(GPUTriangle));
	cudaMalloc(&d_spheres,     cpuSpheres.size()   * sizeof(GPUSphere));
	cudaMalloc(&d_materials,   cpuMaterials.size() * sizeof(GPUMaterial));
	cudaMalloc(&d_lightIndices,lightTriIndices.size() * sizeof(int));
	cudaMalloc(&d_framebuffer, totalPix * sizeof(float3));
	cudaMalloc(&d_randSeeds,   totalPix * sizeof(unsigned long long));

	// ---- 拷贝数据到 GPU ----
	cudaMemcpy(d_nodes,        cpuBVHNodes.data(),   cpuBVHNodes.size()  * sizeof(GPUBVHNode),  cudaMemcpyHostToDevice);
	cudaMemcpy(d_triangles,    cpuTriangles.data(),  cpuTriangles.size() * sizeof(GPUTriangle), cudaMemcpyHostToDevice);
	cudaMemcpy(d_spheres,      cpuSpheres.data(),    cpuSpheres.size()   * sizeof(GPUSphere),   cudaMemcpyHostToDevice);
	cudaMemcpy(d_materials,    cpuMaterials.data(),  cpuMaterials.size() * sizeof(GPUMaterial), cudaMemcpyHostToDevice);
	cudaMemcpy(d_lightIndices, lightTriIndices.data(), lightTriIndices.size() * sizeof(int),    cudaMemcpyHostToDevice);

	// ---- 初始化随机种子 ----
	std::vector<unsigned long long> hostSeeds(totalPix);
	for (int i = 0; i < totalPix; i++) {
		hostSeeds[i] = (unsigned long long)(i + 1) * 123456789ULL;
	}
	cudaMemcpy(d_randSeeds, hostSeeds.data(), totalPix * sizeof(unsigned long long), cudaMemcpyHostToDevice);

	// ---- 启动 Kernel ----
	int blockSize = 256;
	int gridSize = (totalPix + blockSize - 1) / blockSize;
	int rootIdx = cpuBVHNodes.empty() ? -1 : 0;

	printf("GPU render: %dx%d, %d spp, %d blocks x %d threads\n",
	       width, height, spp, gridSize, blockSize);

	renderKernel<<<gridSize, blockSize>>>(
		d_nodes, rootIdx,
		d_triangles, d_spheres, d_materials,
		d_lightIndices, (int)lightTriIndices.size(),
		width, height, spp, maxDepth,
		eyeX, eyeY, eyeZ, halfFOV,
		d_framebuffer, d_randSeeds);

	cudaDeviceSynchronize();
	cudaError_t err = cudaGetLastError();
	if (err != cudaSuccess) {
		fprintf(stderr, "CUDA kernel error: %s\n", cudaGetErrorString(err));
	}

	// ---- 取回 framebuffer ----
	std::vector<float3> hostFB(totalPix);
	cudaMemcpy(hostFB.data(), d_framebuffer, totalPix * sizeof(float3), cudaMemcpyDeviceToHost);

	// ---- 写 PPM ----
	FILE* fp = fopen(outputPath, "wb");
	if (!fp) {
		fprintf(stderr, "Error opening file: %s\n", outputPath);
	} else {
		fprintf(fp, "P6\n%d %d\n255\n", width, height);
		for (int i = 0; i < totalPix; i++) {
			unsigned char c[3];
			c[0] = (unsigned char)(255.0f * fminf(1.0f, fmaxf(0.0f, hostFB[i].x)));
			c[1] = (unsigned char)(255.0f * fminf(1.0f, fmaxf(0.0f, hostFB[i].y)));
			c[2] = (unsigned char)(255.0f * fminf(1.0f, fmaxf(0.0f, hostFB[i].z)));
			fwrite(c, 1, 3, fp);
		}
		fclose(fp);
	}

	// ---- 清理 GPU 内存 ----
	cudaFree(d_nodes);
	cudaFree(d_triangles);
	cudaFree(d_spheres);
	cudaFree(d_materials);
	cudaFree(d_lightIndices);
	cudaFree(d_framebuffer);
	cudaFree(d_randSeeds);
}
