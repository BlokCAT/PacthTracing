#pragma once

#include <vector>
#include "BVH.cuh"
#include "Material.cuh"
#include "Triangle.cuh"

// Host 端调用：组装 GPU 数据 → 启动 kernel → 取回 framebuffer → 写 PPM
void cudaRender(
	const std::vector<GPUTriangle>& cpuTriangles,
	const std::vector<GPUSphere>&    cpuSpheres,
	const std::vector<GPUMaterial>&  cpuMaterials,
	const std::vector<GPUBVHNode>&   cpuBVHNodes,
	const std::vector<int>&          lightTriIndices,
	int width, int height, int spp, int maxDepth,
	float eyeX, float eyeY, float eyeZ,
	float halfFOV,
	const char* outputPath);
