#pragma once

#include "Vector.cuh"
#include "Tool.cuh"

// ============================================================
//  材质类型枚举
// ============================================================
enum MaterialType {
	DIFFUSE, REFLC, MIRCO, REFRACT, BLENDER
};

// ============================================================
//  GPUMaterial — 纯 POD，GPU 端材质数据
//  对应 CPU 端 Material，但去掉了 string/Texture/SetLight
// ============================================================
struct GPUMaterial {
	int    mtype = DIFFUSE;
	Vector3f Kd, Ks, Ka;
	bool   islight = false;
	bool   isTexture = false;
	float  roughness = 0.0f;
	float  ior = 0.0f;
	int    Illum = 2;
	float  Ns = 0.0f;
	Vector3f lightIntensity;
};

// ============================================================
//  BRDF 内部数学 — 全部 GPU/CPU 共享
// ============================================================

// ---- GGX 法线分布函数 ----
CUHD inline float D_GGX(const Vector3f& h, float r, const Vector3f& N) {
	float r2 = r * r;
	float NdotH = fmaxf(0.0f, dotProduct(N, h));
	float NdotH2 = NdotH * NdotH;
	float denom = NdotH2 * (r2 - 1.0f) + 1.0f;
	return r2 / (M_PI * denom * denom + 0.0001f);
}

// ---- Smith 几何遮蔽（单向）----
CUHD inline float G_s(float NdotV, float a) {
	float k = (a + 1.0f) * (a + 1.0f) / 8.0f;
	return NdotV / (NdotV * (1.0f - k) + k);
}

// ---- Smith 几何遮蔽（双向）----
CUHD inline float G(const Vector3f& wi, const Vector3f& wo, const Vector3f& N, float a) {
	float NdotI = fmaxf(0.0f, dotProduct(N, wi));
	float NdotO = fmaxf(0.0f, dotProduct(N, wo));
	return G_s(NdotI, a) * G_s(NdotO, a);
}

// ---- 局部坐标系 → 世界坐标系（Games101 方法）----
CUHD inline Vector3f toWorld(const Vector3f& a, const Vector3f& N) {
	Vector3f B, C;
	if (fabsf(N.x) > fabsf(N.y)) {
		float invLen = 1.0f / sqrtf(N.x * N.x + N.z * N.z);
		C = Vector3f(N.z * invLen, 0.0f, -N.x * invLen);
	} else {
		float invLen = 1.0f / sqrtf(N.y * N.y + N.z * N.z);
		C = Vector3f(0.0f, N.z * invLen, -N.y * invLen);
	}
	B = crossProduct(C, N);
	return (B * a.x) + (C * a.y) + (N * a.z);
}

// ---- 镜面反射方向 ----
CUHD inline Vector3f cuda_reflect(const Vector3f& in, const Vector3f& N) {
	return (in * -1.0f + (N * dotProduct(in, N)) * 2.0f).normalized();
}

// ---- 折射方向（Snell 定律）----
CUHD inline Vector3f cuda_refract(const Vector3f& II, const Vector3f& N, float ior) {
	Vector3f I = II * -1.0f;
	float cosi = fmaxf(-1.0f, fminf(1.0f, dotProduct(I, N)));
	float etai = 1.0f, etat = ior;
	Vector3f n = N;
	if (cosi < 0.0f) { cosi = -cosi; }
	else              { float tmp = etai; etai = etat; etat = tmp; n = N * -1.0f; }
	float eta = etai / etat;
	float k = 1.0f - eta * eta * (1.0f - cosi * cosi);
	if (k < 0.0f) return Vector3f(0.0f);
	return (I * eta + (n * (eta * cosi - sqrtf(k)))).normalized();
}

// ---- Schlick Fresnel（把 kr 写入输出参数）----
CUHD inline void cuda_fresnel(const Vector3f& in, const Vector3f& N, float ior, float& kr) {
	float Ro = ((1.0f - ior) / (1.0f + ior)) * ((1.0f - ior) / (1.0f + ior));
	float costheta = fmaxf(0.0f, dotProduct(N, in));
	float t = (1.0f - costheta) * (1.0f - costheta);
	kr = Ro + (1.0f - Ro) * t;
}

// ============================================================
//  方向采样（需要随机数）— 多一个 RNGState& 参数
// ============================================================
CUHD inline Vector3f GetFutureDir(const Vector3f& wi, const Vector3f& N,
                                   int mtype, RNGState& rng) {
	switch (mtype) {
	case MIRCO:
	case DIFFUSE: {
		float x_1 = gpuRand(rng), x_2 = gpuRand(rng);
		float z = fabsf(1.0f - 2.0f * x_1);
		float r = sqrtf(1.0f - z * z), phi = 2.0f * M_PI * x_2;
		Vector3f localRay(r * cosf(phi), r * sinf(phi), z);
		return toWorld(localRay, N);
	}
	case REFRACT:
	case REFLC: {
		return cuda_reflect(wi, N);
	}
	default:
		return Vector3f(0.0f);
	}
}

// ============================================================
//  PDF
// ============================================================
CUHD inline float cuda_pdf(const Vector3f& wi, const Vector3f& wo,
                            const Vector3f& N, int mtype) {
	switch (mtype) {
	case MIRCO:
	case DIFFUSE: {
		float ans = dotProduct(wo, N);
		if (ans > 0.0f) return 0.5f / M_PI;
		return 0.0f;
	}
	case REFRACT:
	case REFLC: {
		float ans = dotProduct(wo, N);
		if (ans > 0.0f) return 1.0f;
		return 0.0f;
	}
	default:
		return 0.0f;
	}
}

// ============================================================
//  BRDF
// ============================================================
CUHD inline Vector3f GetRefracBRDF(const Vector3f& wi, const Vector3f& wo,
                                    const Vector3f& N) {
	if (dotProduct(wo, N)) { return Vector3f(1.0f / dotProduct(wo, N)); }
	return Vector3f(0.0f);
}

CUHD inline Vector3f GetDiffuseBRDF(const Vector3f& wi, const Vector3f& wo,
                                     const Vector3f& N) {
	float ans = dotProduct(wo, N);
	if (ans > 0.0f) { return Vector3f(1.0f / M_PI); }
	return Vector3f(0.0f);
}

CUHD inline Vector3f GetReflectBRDF(const Vector3f& wi, const Vector3f& wo,
                                     const Vector3f& N) {
	float ans = dotProduct(wo, N);
	if (ans > 0.0f) { return Vector3f(1.0f / ans); }
	return Vector3f(0.0f);
}

CUHD inline Vector3f GetMicroBRDF(const Vector3f& wi, const Vector3f& wo,
                                   const Vector3f& N, float ior, float roughness) {
	float cosalpha = dotProduct(N, wo);
	if (cosalpha > 0.0f) {
		Vector3f h = (wi + wo).normalized();
		float F = 0.0f;
		cuda_fresnel(wi, N, ior, F);
		float down = 4.0f * fabsf(
			fmaxf(0.0f, dotProduct(N, wi)) * fmaxf(0.0f, dotProduct(N, wo))
		) + 0.00001f;
		float up = F * G(wi, wo, N, roughness) * D_GGX(h, roughness, N);
		return (up / down);
	}
	return Vector3f(0.0f);
}
