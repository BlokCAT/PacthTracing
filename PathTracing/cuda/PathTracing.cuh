#pragma once

#include "BVH.cuh"
#include "Material.cuh"

// ============================================================
//  矩形面光源采样
// ============================================================
CUHD inline void PlaneSampleLight(
	const int* lightTriIndices, int lightCount,
	const GPUTriangle* triangles,
	RNGState& rng,
	GPUKitPoint& hp, float& pdf)
{
	if (lightCount == 0) { pdf = 0.0f; return; }
	float gs = gpuRand(rng);
	if (gs >= 1.0f) gs = 0.999f;
	int idx = (int)(gs * (float)lightCount);
	const GPUTriangle& tri = triangles[lightTriIndices[idx]];

	float r1 = gpuRand(rng), r2 = gpuRand(rng);
	float sqrtR1 = sqrtf(r1);
	float b1 = 1.0f - sqrtR1;
	float b2 = r2 * sqrtR1;
	hp.hitcoord = (tri.v0 * b1) + (tri.v1 * b2) + (tri.v2 * (1.0f - b1 - b2));
	hp.hitN = tri.faceNormal;
	hp.materialIdx = tri.materialIdx;
	hp.happened = true;

	float triArea = 0.5f * crossProduct(tri.v1 - tri.v0, tri.v2 - tri.v0).len();
	pdf = (1.0f / (float)lightCount) * (1.0f / triArea);
}

// ============================================================
//  迭代路径追踪（GPU 端，纯迭代，无递归）
// ============================================================
CUHD Vector3f PathTracingGPU(
	const GPUBVHNode* nodes, int rootIdx,
	const GPUTriangle* triangles,
	const GPUSphere*    spheres,
	const GPUMaterial* materials,
	const int* lightTriIndices, int lightCount,
	const Ray& initialRay,
	int maxDepth,
	RNGState& rng)
{
	Vector3f L(0.0f);
	Vector3f throughput(1.0f);
	Ray ray = initialRay;

	for (int depth = 0; depth < maxDepth; depth++) {

		GPUKitPoint hp;
		BVHTraverse(nodes, rootIdx, triangles, spheres, materials, ray, hp);

		if (!hp.happened) break;   // miss → black (HDRI later)

		const GPUMaterial& mat = materials[hp.materialIdx];
		if (mat.islight) {
			L = L + (throughput * mat.lightIntensity);
			break;
		}

		Vector3f hitPos = hp.hitcoord;
		Vector3f N = hp.hitN;
		float nL = N.len();
		N = (nL < 0.0001f) ? Vector3f(0, 0, 1) : (N / nL);
		Vector3f wi = ray.dir * -1.0f;
		float wiL = wi.len();
		wi = (wiL < 0.0001f) ? Vector3f(0, 0, 1) : (wi / wiL);
		Vector3f hitKd = hp.hitColor;

		// ======== NEE 直接光照（REFLC 跳过）========
		if (mat.mtype != REFLC && mat.mtype != REFRACT) {
			float pdf_L = 0.0f;
			GPUKitPoint sampleLP;
			PlaneSampleLight(lightTriIndices, lightCount, triangles, rng, sampleLP, pdf_L);

			if (pdf_L > 0.0001f) {
				Vector3f sampleDir = (sampleLP.hitcoord - hitPos).normalized();
				float d = (hitPos - sampleLP.hitcoord).len();
				Ray shadowRay(hitPos + (N * 0.0001f), sampleDir);
				GPUKitPoint shadowHP;
				BVHTraverse(nodes, rootIdx, triangles, spheres, materials, shadowRay, shadowHP);

				if (shadowHP.happened && materials[shadowHP.materialIdx].islight) {
					Vector3f L_i = materials[shadowHP.materialIdx].lightIntensity;
					float cosT1 = fmaxf(0.0f, dotProduct(N, sampleDir));
					float cosT2 = fmaxf(0.0f, dotProduct(shadowHP.hitN, sampleDir * -1.0f));
					if (cosT1 > 0.0001f && cosT2 > 0.0001f) {
						Vector3f brdf;
						if (mat.mtype == MIRCO)
							brdf = GetMicroBRDF(wi, sampleDir, N, mat.ior, mat.roughness);
						else
							brdf = GetDiffuseBRDF(wi, sampleDir, N);
						brdf = brdf * hitKd;
						float d2 = d * d;
						L = L + (throughput * L_i * brdf * cosT1 * cosT2 / (d2 * pdf_L));
					}
				}
			}
		}

		// ======== Russian Roulette ========
		if (depth > 0) {
			if (gpuRand(rng) > RussianRoulette) break;
			throughput = throughput / RussianRoulette;
		}

		// ======== 采样下一个方向（按材质分支，跟 CPU 一致）========
		Vector3f futureDir;
		Vector3f brdf;
		float pdf_;

		switch (mat.mtype) {

		case REFLC: {
			// 纯镜面反射（确定性的）
			futureDir = GetFutureDir(wi, N, REFLC, rng);
			brdf = GetReflectBRDF(wi, futureDir, N) * hitKd;
			pdf_ = 1.0f;
			break;
		}

		case MIRCO: {
			futureDir = GetFutureDir(wi, N, MIRCO, rng);
			brdf = GetMicroBRDF(wi, futureDir, N, mat.ior, mat.roughness) * hitKd;
			pdf_ = cuda_pdf(wi, futureDir, N, MIRCO);
			break;
		}

		case DIFFUSE: {
			futureDir = GetFutureDir(wi, N, DIFFUSE, rng);
			brdf = GetDiffuseBRDF(wi, futureDir, N) * hitKd;
			pdf_ = cuda_pdf(wi, futureDir, N, DIFFUSE);
			break;
		}

		case REFRACT: {
			// Fresnel 计算 K
			float K = 0.0f;
			cuda_fresnel(wi, N, mat.ior, K);

			// 概率 K 选反射，(1-K) 选折射（跟 CPU 的加权混合在统计上等价）
			if (gpuRand(rng) < K) {
				// ---- 表面反射 ----
				futureDir = GetFutureDir(wi, N, REFRACT, rng);
				brdf = GetRefracBRDF(wi, futureDir, N) * hitKd;
				pdf_ = 1.0f;
				// 下一根光线从表面出发
				ray = Ray(hitPos + (N * 0.0001f), futureDir);
			}
			else {
				// ---- 折射入体 → 内部交点 → 折射射出 ----
				Vector3f refractIn = cuda_refract(wi, N, mat.ior);
				if (refractIn.x == 0.0f && refractIn.y == 0.0f && refractIn.z == 0.0f) break;

				Ray innerRay(hitPos - (N * 0.001f), refractIn);
				GPUKitPoint innerHP;
				BVHTraverse(nodes, rootIdx, triangles, spheres, materials, innerRay, innerHP);

				if (!innerHP.happened) break;

				Vector3f innerN = innerHP.hitN;
				float inL = innerN.len();
				innerN = (inL < 0.0001f) ? Vector3f(0, 0, 1) : (innerN / inL);
				Vector3f nn = innerN * -1.0f;   // 指向物体外侧

				const GPUMaterial& innerMat = materials[innerHP.materialIdx];
				Vector3f innerWi = innerRay.dir * -1.0f;
				float iwL = innerWi.len();
				innerWi = (iwL < 0.0001f) ? Vector3f(0, 0, 1) : (innerWi / iwL);

				Vector3f refractOut = cuda_refract(innerWi, nn, innerMat.ior);
				if (refractOut.x == 0.0f && refractOut.y == 0.0f && refractOut.z == 0.0f) break;

				// 传输 BRDF = 1/dot(refractOut, nn)
				brdf = GetRefracBRDF(innerWi, refractOut, nn);
				pdf_ = 1.0f;

				// 下一根光线从射出点出发
				ray = Ray(innerHP.hitcoord + (nn * 0.001f), refractOut);
				float cosT = dotProduct(refractOut, nn);
				if (cosT < 0.0001f) break;
				throughput = throughput * brdf * cosT / pdf_;
				continue;   // 跳过后面通用的 throughput 更新
			}
			break;
		}

		case BLENDER: {
			// 概率 Smoothness 选镜面，(1-Smoothness) 选漫反射
			float smoothness = mat.Ns / 1000.0f;
			if (smoothness < 0.0f) smoothness = 0.0f;
			if (smoothness > 1.0f) smoothness = 1.0f;

			if (gpuRand(rng) < smoothness) {
				// 镜面路径（对应 CPU BRDF=GetReflectBRDF）
				futureDir = GetFutureDir(wi, N, REFLC, rng);
				brdf = GetReflectBRDF(wi, futureDir, N);
				pdf_ = 1.0f;
			} else {
				// 漫反射路径
				futureDir = GetFutureDir(wi, N, DIFFUSE, rng);
				brdf = GetDiffuseBRDF(wi, futureDir, N) * hitKd;
				pdf_ = cuda_pdf(wi, futureDir, N, DIFFUSE);
			}
			break;
		}

		default:
			break;   // 未知材质类型，终止
		}

		// ======== NEE 重复检测：跟 CPU 一样，间接光线不能打到光源 ========
		// （REFLC 没有 NEE，反射打到光源是正常的）
		if (mat.mtype != REFLC && mat.mtype != REFRACT) {
			Ray nextRay(hitPos + (N * 0.0001f), futureDir);
			GPUKitPoint nextHP;
			BVHTraverse(nodes, rootIdx, triangles, spheres, materials, nextRay, nextHP);
			if (nextHP.happened && materials[nextHP.materialIdx].islight)
				break;   // NEE 已处理，跳过双计
		}

		// ======== 更新 throughput，准备下一根光线 ========
		float cosT = fmaxf(0.0f, dotProduct(futureDir, N));
		if (cosT < 0.0001f || pdf_ < 0.0001f) break;

		throughput = throughput * brdf * cosT / pdf_;
		ray = Ray(hitPos + (N * 0.0001f), futureDir);
	}

	L.x = fminf(1.0f, L.x);
	L.y = fminf(1.0f, L.y);
	L.z = fminf(1.0f, L.z);
	return L;
}
