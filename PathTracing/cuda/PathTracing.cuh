#pragma once

#include "BVH.cuh"
#include "Material.cuh"

// ============================================================
//  矩形面光源采样
//
//  在场景的所有光源三角形中随机选一个，在上面均匀采样一个点。
//  返回采样点的几何信息和 PDF（概率密度）。
//
//  参数:
//    lightTriIndices - 光源三角形在 triangles[] 中的索引数组
//    lightCount      - 光源三角形的总数
//    triangles       - 全局三角形数组
//    rng             - 本线程的随机数状态
//    hp              - [输出] 采样点信息（位置、法线、材质索引）
//    pdf             - [输出] 概率密度 = 1/(光源数量 × 三角形面积)
// ============================================================
CUHD inline void PlaneSampleLight(
	const int* lightTriIndices, int lightCount,
	const GPUTriangle* triangles,
	RNGState& rng,
	GPUKitPoint& hp, float& pdf)
{
	if (lightCount == 0) { pdf = 0.0f; return; }

	// 第一步：均匀随机选一个光源三角形
	float gs = gpuRand(rng);
	if (gs >= 1.0f) gs = 0.999f;
	int idx = (int)(gs * (float)lightCount);
	const GPUTriangle& tri = triangles[lightTriIndices[idx]];

	// 第二步：在选中的三角形上均匀采样一个点（用重心坐标 + sqrt 映射）
	float r1 = gpuRand(rng), r2 = gpuRand(rng);
	float sqrtR1 = sqrtf(r1);
	float b1 = 1.0f - sqrtR1;
	float b2 = r2 * sqrtR1;
	float b3 = 1.0f - b1 - b2;

	// 世界空间坐标
	hp.hitcoord = (tri.v0 * b1) + (tri.v1 * b2) + (tri.v2 * b3);
	hp.hitN     = tri.faceNormal;
	hp.materialIdx = tri.materialIdx;
	hp.happened = true;

	// PDF = 选这个三角形的概率 × 在这个三角形里选到这个点的概率
	float triArea = 0.5f * crossProduct(tri.v1 - tri.v0, tri.v2 - tri.v0).len();
	pdf = (1.0f / (float)lightCount) * (1.0f / triArea);
}

// ============================================================
//  迭代路径追踪 — 完整的渲染方程求解
//
//  【算法流程】
//  对于一根初始光线，循环 depth = 0..maxDepth：
//    1. BVH 求交 → 找到最近的命中点
//    2. 没命中？→ 终止（后续可加 HDRI 环境光）
//    3. 命中光源？→ 累加光强，终止
//    4. NEE 直接光采样（REFLC/REFRACT 跳过）
//       → 在光源上随机选点 → 发 shadow ray → 无遮挡则累加 L_dir
//    5. Russian Roulette 概率终止（depth > 0 时）
//    6. 按材质类型采样下一个方向 → 更新 throughput → 构造新光线
//    7. 回到步骤 1
//
//  【参数说明】
//  场景数据（GPU 端指针，host 端通过 cudaMemcpy 拷入）:
//    nodes            - 扁平 BVH 节点数组
//    rootIdx          - BVH 根节点索引（通常为 0）
//    triangles        - 所有三角形数据（顶点、法线、UV、材质索引等）
//    spheres          - 所有球体数据（球心、半径、材质索引）
//    materials        - 所有材质数据（Kd, roughness, ior, mtype 等）
//    lightTriIndices  - 光源三角形的索引数组（NEE 采样用）
//    lightCount       - 光源三角形的数量
//
//  光线与渲染参数:
//    initialRay       - 从相机发出的初始光线
//    maxDepth         - 最大递归深度（控制间接光照 bounce 次数）
//    rng              - 本线程的随机数状态（引用传递，每次调用会更新）
//
//  【返回值】
//    0 号像素最终累积的 RGB 辐射度（已经 clamp 到 [0,1]）
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
	// L:  这个像素最终累积的总辐射度（逐步累加每层 bounce 的贡献）
	// throughput: 路径一路上的衰减因子 = BRDF × cos / PDF 的连乘
	//            初始为 (1,1,1)，越往后越小 → 深层 bounce 贡献越来越弱
	Vector3f L(0.0f);
	Vector3f throughput(1.0f);
	Ray ray = initialRay;
	bool prevDidNEE = false;   // 上一轮是否做了 NEE

	// ================================================================
	// 主循环：每个 depth 对应一次 bounce
	// ================================================================
	for (int depth = 0; depth < maxDepth; depth++) {

		// ----- 第 1 步：BVH 求交 -----
		GPUKitPoint hp;
		BVHTraverse(nodes, rootIdx, triangles, spheres, materials, ray, hp);

		// ----- 第 2 步：没命中任何物体 → 终止（后续可返回 HDRI 环境颜色）-----
		if (!hp.happened) break;

		// ----- 第 3 步：命中光源 → 累加光源辐射，路径终止 -----
		const GPUMaterial& mat = materials[hp.materialIdx];
		if (mat.islight) {
			L = L + ((prevDidNEE ? Vector3f(0.0f) : throughput * mat.lightIntensity));
			break;
		}

		// ----- 第 4 步：整理命中点的局部几何信息 -----
		Vector3f hitPos = hp.hitcoord;
		Vector3f N = hp.hitN;
		float nL = N.len();
		N = (nL < 0.0001f) ? Vector3f(0, 0, 1) : (N / nL);

		// wi = 入射方向（从命中点指向光线来的方向）
		Vector3f wi = ray.dir * -1.0f;
		float wiL = wi.len();
		wi = (wiL < 0.0001f) ? Vector3f(0, 0, 1) : (wi / wiL);

		// hitKd = 命中点的颜色（如果是纹理材质则查纹理，否则 = 材质 Kd）
		Vector3f hitKd = hp.hitColor;

		// ----- 第 5 步：NEE 直接光照 -----
		// REFLC（纯镜面）、REFRACT（折射体）不做 NEE，因为它们是 delta 分布
		// 方向是确定的，不需要"随机选光源方向"
		if (mat.mtype != REFLC && mat.mtype != REFRACT) {
			float pdf_L = 0.0f;
			GPUKitPoint sampleLP;
			PlaneSampleLight(lightTriIndices, lightCount, triangles, rng, sampleLP, pdf_L);

			if (pdf_L > 0.0001f) {
				// 构造 shadow ray：从命中点指向光源采样点
				Vector3f sampleDir = (sampleLP.hitcoord - hitPos).normalized();
				float d  = (hitPos - sampleLP.hitcoord).len();
				Ray shadowRay(hitPos + (N * 0.0001f), sampleDir);

				// 检测有没有遮挡
				GPUKitPoint shadowHP;
				BVHTraverse(nodes, rootIdx, triangles, spheres, materials, shadowRay, shadowHP);

				// 无遮挡 → 累加直接光贡献
				if (shadowHP.happened && materials[shadowHP.materialIdx].islight) {
					Vector3f L_i   = materials[shadowHP.materialIdx].lightIntensity;
					float cosT1 = fmaxf(0.0f, dotProduct(N, sampleDir));
					float cosT2 = fmaxf(0.0f, dotProduct(shadowHP.hitN, sampleDir * -1.0f));
					if (cosT1 > 0.0001f && cosT2 > 0.0001f) {
						// BRDF 选择：微表面用 GGX，其余用 Lambertian
						Vector3f brdf;
						if (mat.mtype == MIRCO)
							brdf = GetMicroBRDF(wi, sampleDir, N, mat.ior, mat.roughness);
						else
							brdf = GetDiffuseBRDF(wi, sampleDir, N);
						brdf = brdf * hitKd;
						float d2 = d * d;
						// Monte Carlo 估计：
						//   L_dir = L_i × BRDF × cosθ₁ × cosθ₂ / (距离² × PDF)
						L = L + (throughput * L_i * brdf * cosT1 * cosT2 / (d2 * pdf_L));
					}
				}
			}
		}

		// ----- 第 6 步：Russian Roulette 路径终止 -----
		// 以 80% 概率存活、20% 概率终止
		// 存活的路径 throughput 乘以 1/0.8 补偿（确保期望值不变）
		if (depth > 0) {
			if (gpuRand(rng) > RussianRoulette) break;
			throughput = throughput / RussianRoulette;
		}

		// ----- 第 7 步：按材质类型采样下一个方向 -----
		// CPU 的 switch(mat->mtype) 原样搬过来
		Vector3f futureDir;
		Vector3f brdf;
		float pdf_;

		switch (mat.mtype) {

		// ============ REFLC：纯镜面反射 ============
		case REFLC: {
			futureDir = GetFutureDir(wi, N, REFLC, rng);     // 确定性的反射方向
			brdf      = GetReflectBRDF(wi, futureDir, N) * hitKd;
			pdf_      = 1.0f;   // delta 分布，PDF = 1
			break;
		}

		// ============ MIRCO：微表面 GGX ============
		case MIRCO: {
			futureDir = GetFutureDir(wi, N, MIRCO, rng);     // 半球均匀采样
			brdf      = GetMicroBRDF(wi, futureDir, N, mat.ior, mat.roughness) * hitKd;
			pdf_      = cuda_pdf(wi, futureDir, N, MIRCO);    // 0.5/π
			break;
		}

		// ============ DIFFUSE：Lambertian 漫反射 ============
		case DIFFUSE: {
			futureDir = GetFutureDir(wi, N, DIFFUSE, rng);
			brdf      = GetDiffuseBRDF(wi, futureDir, N) * hitKd;
			pdf_      = cuda_pdf(wi, futureDir, N, DIFFUSE);  // 0.5/π
			break;
		}

		// ============ REFRACT：折射体（玻璃、水等）============
		// 使用 Fresnel 系数随机选择反射或折射路径（重要性采样）
		case REFRACT: {
			float K = 0.0f;
			cuda_fresnel(wi, N, mat.ior, K);   // K = 反射比例

			if (gpuRand(rng) < K) {
				// ---- 反射分支（概率 K）----
				futureDir = GetFutureDir(wi, N, REFRACT, rng);
				brdf      = GetRefracBRDF(wi, futureDir, N) * hitKd;
				pdf_      = 1.0f;
				ray       = Ray(hitPos + (N * 0.0001f), futureDir);
			} else {
				// ---- 折射分支（概率 1-K）----
				// 第 1 段：射入物体内部
				Vector3f refractIn = cuda_refract(wi, N, mat.ior);
				if (refractIn.x == 0.0f && refractIn.y == 0.0f && refractIn.z == 0.0f) break;

				Ray innerRay(hitPos - (N * 0.001f), refractIn);
				GPUKitPoint innerHP;
				BVHTraverse(nodes, rootIdx, triangles, spheres, materials, innerRay, innerHP);
				if (!innerHP.happened) break;

				// 第 2 段：在物体内部找到背面的命中点，折射射出
				Vector3f innerN = innerHP.hitN;
				float inL = innerN.len();
				innerN = (inL < 0.0001f) ? Vector3f(0, 0, 1) : (innerN / inL);
				Vector3f nn = innerN * -1.0f;   // 法线翻转向外

				const GPUMaterial& innerMat = materials[innerHP.materialIdx];
				Vector3f innerWi = innerRay.dir * -1.0f;
				float iwL = innerWi.len();
				innerWi = (iwL < 0.0001f) ? Vector3f(0, 0, 1) : (innerWi / iwL);

				Vector3f refractOut = cuda_refract(innerWi, nn, innerMat.ior);
				if (refractOut.x == 0.0f && refractOut.y == 0.0f && refractOut.z == 0.0f) break;

				brdf = GetRefracBRDF(innerWi, refractOut, nn);
				pdf_ = 1.0f;

				// 下一根光线从射出点出发
				ray = Ray(innerHP.hitcoord + (nn * 0.001f), refractOut);
				float cosT = dotProduct(refractOut, nn);
				if (cosT < 0.0001f) break;
				throughput = throughput * brdf * cosT / pdf_;
				continue;   // 跳过底部通用的 throughput 更新（折射路径已经手动更新了）
			}
			break;
		}

		// ============ BLENDER：Diffuse + Specular 混合 ============
		// Illum=2：纯漫反射
		// Illum=3：按 smoothness 随机选镜面或漫反射路径
		case BLENDER: {
			float smoothness = mat.Ns / 1000.0f;
			if (smoothness < 0.0f) smoothness = 0.0f;
			if (smoothness > 1.0f) smoothness = 1.0f;

			if (gpuRand(rng) < smoothness) {
				// 镜面路径
				futureDir = GetFutureDir(wi, N, REFLC, rng);
				brdf      = GetReflectBRDF(wi, futureDir, N);         // 不乘 hitKd（跟 CPU 一致）
				pdf_      = 1.0f;
			} else {
				// 漫反射路径
				futureDir = GetFutureDir(wi, N, DIFFUSE, rng);
				brdf      = GetDiffuseBRDF(wi, futureDir, N) * hitKd;
				pdf_      = cuda_pdf(wi, futureDir, N, DIFFUSE);
			}
			break;
		}

		default:
			break;
		}

		// ----- 第 7.5 步：NEE 双计检测 -----
		// 对于做了 NEE 的材质（DIFFUSE/MIRCO/BLENDER），半球随机采样的方向
		// 有可能恰好击中光源——这会跟 NEE 重复计数。
		// 故发一条探测光线：如果下个命中点是光源，就跳过（NEE 已经算过这段光了）。
				// NEE 双计检测已改为 prevDidNEE 标记法（见循环头）

		// ----- 第 8 步：更新 throughput，构造下一根光线 -----
		// throughput = 路径衰减因子
		// 公式：throughput *= BRDF × cosθ / PDF
		//   BRDF: 材质在这个方向上的反射率
		//   cosθ: 光线方向与法线的夹角余弦（Lambert 余弦项）
		//   PDF:  采样这个方向的概率密度（分母，保证 Monte Carlo 无偏）
		float cosT = fmaxf(0.0f, dotProduct(futureDir, N));
		if (cosT < 0.0001f || pdf_ < 0.0001f) break;

		throughput = throughput * brdf * cosT / pdf_;

		// 新光线的起点 = 命中点 + 微小的法线偏移（防止自交）
		ray = Ray(hitPos + (N * 0.0001f), futureDir);
	} // end for depth

	// 最终 clamp 防 firefly
	L.x = fminf(1.0f, L.x);
	L.y = fminf(1.0f, L.y);
	L.z = fminf(1.0f, L.z);
	return L;
}
