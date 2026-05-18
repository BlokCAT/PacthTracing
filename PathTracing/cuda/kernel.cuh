#pragma once

#include <vector>
#include "BVH.cuh"
#include "Material.cuh"
#include "Triangle.cuh"
#include "Camera.cuh"

void cudaRender(
	const std::vector<TriangleGeo>&  cpuGeo,
	const std::vector<TriangleMeta>& cpuMeta,
	const std::vector<GPUSphere>&    cpuSpheres,
	const std::vector<GPUMaterial>&  cpuMaterials,
	const std::vector<GPUBVHNode>&   cpuBVHNodes,
	const std::vector<int>&          lightTriIndices,
	int width, int height, int spp, int maxDepth,
	const Camera& cam,
	const char* outputPath);
