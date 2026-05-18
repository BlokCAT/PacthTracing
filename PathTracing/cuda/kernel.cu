#include "kernel.cuh"
#include "PathTracing.cuh"
#include <cstdio>
#include <cuda_runtime.h>

__global__ void renderKernel(
	const GPUBVHNode*   nodes, int rootIdx,
	const TriangleGeo*  geo,
	const TriangleMeta* meta,
	const GPUSphere*    spheres,
	const GPUMaterial*  materials,
	const int*          lightTriIndices, int lightCount,
	int width, int height, int spp, int maxDepth,
	Camera cam,
	float3* framebuffer,
	unsigned long long* randSeeds)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	int totalPix = width * height;
	if (idx >= totalPix) return;

	int i = idx / width;
	int j = idx % width;

	RNGState rng;
	rng.state = randSeeds[idx];

	Vector3f color(0.0f);

	for (int k = 0; k < spp; k++) {
		Ray ray = cam.generateRay(i, j, width, height);
		color = color + PathTracingGPU(
			nodes, rootIdx, geo, meta, spheres, materials,
			lightTriIndices, lightCount,
			ray, maxDepth, rng);
	}

	color = color / (float)spp;
	color.x = powf(fminf(1.0f, fmaxf(0.0f, color.x)), 0.4f);
	color.y = powf(fminf(1.0f, fmaxf(0.0f, color.y)), 0.4f);
	color.z = powf(fminf(1.0f, fmaxf(0.0f, color.z)), 0.4f);
	framebuffer[idx] = make_float3(color.x, color.y, color.z);
}

void cudaRender(
	const std::vector<TriangleGeo>&  cpuGeo,
	const std::vector<TriangleMeta>& cpuMeta,
	const std::vector<GPUSphere>&    cpuSpheres,
	const std::vector<GPUMaterial>&  cpuMaterials,
	const std::vector<GPUBVHNode>&   cpuBVHNodes,
	const std::vector<int> & lightTriIndices,
	int width, int height, int spp, int maxDepth,
	const Camera& cam,
	const char* outputPath)
{
	int totalPix = width * height;

	GPUBVHNode*  d_nodes;
	TriangleGeo*  d_geo;
	TriangleMeta* d_meta;
	GPUSphere*   d_spheres;
	GPUMaterial* d_materials;
	int*         d_lightIndices;
	float3*      d_framebuffer;
	unsigned long long* d_randSeeds;

	cudaMalloc(&d_nodes,        cpuBVHNodes.size()   * sizeof(GPUBVHNode));
	cudaMalloc(&d_geo,          cpuGeo.size()        * sizeof(TriangleGeo));
	cudaMalloc(&d_meta,         cpuMeta.size()       * sizeof(TriangleMeta));
	cudaMalloc(&d_spheres,      cpuSpheres.size()    * sizeof(GPUSphere));
	cudaMalloc(&d_materials,    cpuMaterials.size()  * sizeof(GPUMaterial));
	cudaMalloc(&d_lightIndices, lightTriIndices.size() * sizeof(int));
	cudaMalloc(&d_framebuffer,  totalPix * sizeof(float3));
	cudaMalloc(&d_randSeeds,    totalPix * sizeof(unsigned long long));

	cudaMemcpy(d_nodes,        cpuBVHNodes.data(),   cpuBVHNodes.size()   * sizeof(GPUBVHNode),  cudaMemcpyHostToDevice);
	cudaMemcpy(d_geo,          cpuGeo.data(),        cpuGeo.size()        * sizeof(TriangleGeo), cudaMemcpyHostToDevice);
	cudaMemcpy(d_meta,         cpuMeta.data(),       cpuMeta.size()       * sizeof(TriangleMeta),cudaMemcpyHostToDevice);
	cudaMemcpy(d_spheres,      cpuSpheres.data(),    cpuSpheres.size()    * sizeof(GPUSphere),   cudaMemcpyHostToDevice);
	cudaMemcpy(d_materials,    cpuMaterials.data(),  cpuMaterials.size()  * sizeof(GPUMaterial), cudaMemcpyHostToDevice);
	cudaMemcpy(d_lightIndices, lightTriIndices.data(), lightTriIndices.size() * sizeof(int),    cudaMemcpyHostToDevice);

	std::vector<unsigned long long> hostSeeds(totalPix);
	for (int i = 0; i < totalPix; i++)
		hostSeeds[i] = (unsigned long long)(i + 1) * 123456789ULL;
	cudaMemcpy(d_randSeeds, hostSeeds.data(), totalPix * sizeof(unsigned long long), cudaMemcpyHostToDevice);

	int blockSize = 256;
	int gridSize = (totalPix + blockSize - 1) / blockSize;
	int rootIdx = cpuBVHNodes.empty() ? -1 : 0;
	printf("GPU render: %dx%d, %d spp, %d blocks x %d threads\n", width, height, spp, gridSize, blockSize);

	renderKernel<<<gridSize, blockSize>>>(
		d_nodes, rootIdx,
		d_geo, d_meta, d_spheres, d_materials,
		d_lightIndices, (int)lightTriIndices.size(),
		width, height, spp, maxDepth,
		cam,
		d_framebuffer, d_randSeeds);

	cudaDeviceSynchronize();
	cudaError_t err = cudaGetLastError();
	if (err != cudaSuccess)
		fprintf(stderr, "CUDA kernel error: %s\n", cudaGetErrorString(err));

	std::vector<float3> hostFB(totalPix);
	cudaMemcpy(hostFB.data(), d_framebuffer, totalPix * sizeof(float3), cudaMemcpyDeviceToHost);

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

	cudaFree(d_nodes); cudaFree(d_geo); cudaFree(d_meta);
	cudaFree(d_spheres); cudaFree(d_materials);
	cudaFree(d_lightIndices); cudaFree(d_framebuffer); cudaFree(d_randSeeds);
}
