#pragma once

#include "BVH.cuh"
#include "Material.cuh"

// ============================================================
//  矩形面光源采样（GPU 端）
//  参数:
//    lightTriIdx  - 光源在 triangles 数组中的起始索引
//    lightTriCount- 光源三角形数量（目前都是 2 个三角形 = 1 个矩形）
//    triangles    - 全局三角形数组
//    rng          - 随机数状态
//    hp           - 输出：采样到的命中点信息
//    pdf          - 输出：采样概率密度
// ============================================================
CUHD inline void PlaneSampleLight(
	const int* lightTriIndices, int lightCount,
	const GPUTriangle* triangles,
	RNGState& rng,
	GPUKitPoint& hp, float& pdf)
{
	if (lightCount == 0) { pdf = 0.0f; return; }

	// 均匀随机选一个光源三角形
	float gs = gpuRand(rng);
	if (gs >= 1.0f) gs = 0.999f;
	int idx = (int)(gs * (float)lightCount);
	const GPUTriangle& tri = triangles[lightTriIndices[idx]];

	// 在三角形上均匀采样（用重心坐标）
	float r1 = gpuRand(rng);
	float r2 = gpuRand(rng);
	float sqrtR1 = sqrtf(r1);
	float b1 = 1.0f - sqrtR1;
	float b2 = r2 * sqrtR1;
	float b3 = 1.0f - b1 - b2;

	// 世界空间采样点
	hp.hitcoord = (tri.v0 * b1) + (tri.v1 * b2) + (tri.v2 * b3);
	hp.hitN = tri.faceNormal;
	hp.materialIdx = tri.materialIdx;
	hp.happened = true;

	// PDF = 选光源概率 × 面积采样概率
	float triArea = 0.5f * crossProduct(tri.v1 - tri.v0, tri.v2 - tri.v0).len();
	pdf = (1.0f / (float)lightCount) * (1.0f / triArea);
}

// ============================================================
//  迭代路径追踪（设备端）
//  将原 Scene::PathTracing 的递归改为 for 循环
// ============================================================
CUHD Vector3f PathTracing(
	const GPUBVHNode* nodes, int rootIdx,
	const GPUTriangle* triangles,
	const GPUMaterial* materials,
	const int* lightTriIndices, int lightCount,
	const Ray& initialRay,
	int maxDepth,
	RNGState& rng)
{
	Vector3f L(0.0f);           // 累积辐射度
	Vector3f throughput(1.0f);  // 路径吞吐量
	Ray ray = initialRay;

	for (int depth = 0; depth < maxDepth; depth++) {

		// ---- 找交点 ----
		GPUKitPoint hp;
		BVHTraverse(nodes, rootIdx, triangles, materials, ray, hp);

		// 未命中 → 返回黑色（HDRI 后续再加）
		if (!hp.happened) break;

		const GPUMaterial& mat = materials[hp.materialIdx];

		// 命中光源 → 累加光强并结束
		if (mat.islight) {
			L = L + (throughput * mat.lightIntensity);
			break;
		}

		Vector3f hitPos = hp.hitcoord;
		Vector3f N = hp.hitN;
		if (N.len() < 0.0001f) N = Vector3f(0.0f, 0.0f, 1.0f);
		N = N.normalized();
		Vector3f wi = (ray.dir * -1.0f).normalized();  // 入射方向
		Vector3f hitKd = hp.hitColor;

		// ---- 直接光 NEE（所有非 REFLC 材质）----
		if (mat.mtype != REFLC) {
			float pdf_L = 0.0f;
			GPUKitPoint sampleLP;
			PlaneSampleLight(lightTriIndices, lightCount, triangles, rng, sampleLP, pdf_L);

			if (pdf_L > 0.0001f) {
				Vector3f sampleDir = (sampleLP.hitcoord - hitPos).normalized();
				float d = (hitPos - sampleLP.hitcoord).len();

				// Shadow ray
				Ray shadowRay(hitPos + (N * 0.0001f), sampleDir);
				GPUKitPoint shadowHP;
				BVHTraverse(nodes, rootIdx, triangles, materials, shadowRay, shadowHP);

				if (shadowHP.happened && materials[shadowHP.materialIdx].islight) {
					Vector3f L_i = materials[shadowHP.materialIdx].lightIntensity;
					float cosT1 = fmaxf(0.0f, dotProduct(N, sampleDir));
					float cosT2 = fmaxf(0.0f, dotProduct(shadowHP.hitN, sampleDir * -1.0f));
					float d2 = d * d + 0.0001f;

					Vector3f brdf;
					if (mat.mtype == MIRCO) {
						brdf = GetMicroBRDF(wi, sampleDir, N, mat.ior, mat.roughness);
					} else {
						// DIFFUSE / REFRACT / BLENDER → 漫反射 BRDF
						brdf = GetDiffuseBRDF(wi, sampleDir, N);
					}
					brdf = brdf * hitKd;
					L = L + (throughput * L_i * brdf * cosT1 * cosT2 / d2 / pdf_L);
				}
			}
		}

		// ---- Russian Roulette ----
		if (depth > 1) {
			if (gpuRand(rng) > RussianRoulette) break;
			throughput = throughput / RussianRoulette;
		}

		// ---- 采样下一个方向 ----
		Vector3f futureDir = GetFutureDir(wi, N, mat.mtype, rng);
		float cosT = fmaxf(0.0f, dotProduct(futureDir, N));
		if (cosT < 0.0001f) break;

		Vector3f brdf;
		float pdf_ = cuda_pdf(wi, futureDir, N, mat.mtype);

		switch (mat.mtype) {
		case DIFFUSE:
			brdf = GetDiffuseBRDF(wi, futureDir, N);
			break;
		case REFLC:
			brdf = GetReflectBRDF(wi, futureDir, N);
			break;
		case MIRCO:
			brdf = GetMicroBRDF(wi, futureDir, N, mat.ior, mat.roughness);
			break;
		case REFRACT:
			brdf = GetRefracBRDF(wi, futureDir, N);
			break;
		case BLENDER:
			brdf = GetDiffuseBRDF(wi, futureDir, N);
			break;
		default:
			brdf = Vector3f(0.0f);
			break;
		}
		brdf = brdf * hitKd;

		if (pdf_ < 0.0001f) break;
		throughput = throughput * brdf * cosT / pdf_;

		// 下一根光线
		ray = Ray(hitPos + (N * 0.0001f), futureDir);
	}

	// 最终 clamp
	L.x = fminf(1.0f, L.x);
	L.y = fminf(1.0f, L.y);
	L.z = fminf(1.0f, L.z);
	return L;
}
