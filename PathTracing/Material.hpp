#pragma once

#ifndef RAYTRACING_MATERIAL_H
#define RAYTRACING_MATERIAL_H
#include "Vector.hpp"
#include "Texture.hpp"
#include <algorithm>
#include <string>

enum  MaterialType
{
	DIFFUSE , REFLC , MIRCO  , REFRACT ,BLENDER 
};

class Material
{
private:
	float D_GGX(Vector3f& h, float& r, const  Vector3f& N)
	{
		float r2 = r * r;
		float NdotH = clamp(0.0, 1.0, dotProduct(N, h));
		float NdotH2 = NdotH * NdotH;
		float res = r2 / ((M_PI * (NdotH2 * (r2 - 1) + 1) * (NdotH2 * (r2 - 1) + 1))) + 0.0001;
		return res;
	}

	float G_s(float& NdotV, float& a)
	{
		float k = (a + 1) * (a + 1) / 8;
		return NdotV / (NdotV * (1 - k) + k);
	}

	float G(const Vector3f& i, const Vector3f& o, const Vector3f& N, float& a)
	{
		float NdotI = clamp(0.0, 1.0, dotProduct(N, i));
		float NdotO = clamp(0.0, 1.0, dotProduct(N, o));
		return G_s(NdotI, a) * G_s(NdotO, a);
	}
	//games101 7
	Vector3f toWorld(const Vector3f& a, const Vector3f& N) {
		Vector3f B, C;
		if (std::fabs(N.x) > std::fabs(N.y)) {
			float invLen = 1.0f / std::sqrt(N.x * N.x + N.z * N.z);
			C = Vector3f(N.z * invLen, 0.0f, -N.x * invLen);
		}
		else {
			float invLen = 1.0f / std::sqrt(N.y * N.y + N.z * N.z);
			C = Vector3f(0.0f, N.z * invLen, -N.y * invLen);
		}
		B = crossProduct(C, N);
		return(B * a.x) + (C * a.y) + (N * a.z);
	}

public:
	Material() {};
	Material(Vector3f kd, MaterialType t) :
		Kd(kd),
		mtype(t),
		islight(false)
	{}
	MaterialType mtype;
	Vector3f Kd;  //Diffuse Color   ������������ɫ
	Vector3f Ks;  //Specular Color  ���淴���Ч��ǿ�Ⱥ���ɫ
	Vector3f Ka;  //Ambient Color   ��������ɫ������û�й��յ�ʱ�����ɫ

	std::string MaterialName ;
	
	Texture texture;
	
	bool islight = false;
	bool isTexture = false;

	float roughness = 0.0f;
	float ior = 0.0f; //������ʵ�������
	int Illum = 2;
	float Ns = 0.0;
	Vector3f lightIntensity;


	//Vector3f getKdat(const Vector3f& hitpos)
	//{
	//	if (isTexture)
	//	{
	//		std::tuple<float, float, float> tup = computeBarycentric3D( )
	//			
	//			computeBarycentric3D
	//		(Vector3f & v1, Vector3f & v2, Vector3f & v3, Vector3f & hit)
	//	}
	//	else return Kd;
	//}

	void SetLight(const Vector3f& light_intensity)
	{
		this->islight = true;
		this->lightIntensity = light_intensity;
	}

	Vector3f reflect(const Vector3f &in, const Vector3f &N)
	{
		return (in * -1 + ((N * dotProduct(in, N)) * 2.0)).normalized();
	}

	Vector3f refract(const Vector3f &II, const Vector3f &N, const float &ior) 
	{
		Vector3f I = II * -1;
		// ��������ǵ�����ֵ
		float cosi = clamp(-1, 1, dotProduct(I, N));
		// ȷ�������ʱȺͷ��߷���
		float etai = 1, etat = ior;
		Vector3f n = N;
		if (cosi < 0)cosi = -cosi; // ������ߴ��ⲿ���������ڲ�
		else{std::swap(etai, etat); n = N * -1;}
		// ���������ʱ�
		float eta = etai / etat;
		// ���������Ĺ��߷���
		float k = 1 - eta * eta * (1 - cosi * cosi);
		if (k < 0) return Vector3f(0.0f);
		else
		{
			// ����������ߵķ���
			return (I* eta + ( n * (eta * cosi - sqrtf(k)))).normalized();
		}
	}

	void fresnel(const Vector3f &in, const Vector3f &N, float &ior, float &kr) //kr�Ƿ���ı���
	{
		in.normalized();
		N.normalized();
		float Ro = ((1.0 - ior) / (1.0 + ior)) * ((1.0 - ior) / (1.0 + ior));
		float costheta = clamp( 0.0f , 1.0 ,  dotProduct(N, in));
		float t = (1.0 - costheta) * (1.0 - costheta);
		kr = Ro + (1 - Ro) * t;
		return;
	}

	Vector3f GetFutureDir(const Vector3f &wi, const Vector3f &N , const MaterialType &_what)
	{
		switch (_what)
		{
		case MIRCO:
		case DIFFUSE:
		{
			// uniform sample on the hemisphere
			float x_1 = RandomFloat(), x_2 = RandomFloat();
			float z = std::fabs(1.0f - 2.0f * x_1);
			float r = std::sqrt(1.0f - z * z), phi = 2 * M_PI * x_2;
			Vector3f localRay(r * std::cos(phi), r * std::sin(phi), z);
			return toWorld(localRay, N);
			break;
		}
		case REFRACT:
		case REFLC :
		{
			Vector3f localRay = reflect(wi, N);
			return localRay;
			break;
		}
		default:
			break;
		}
	}

	float pdf(const Vector3f &wi, const Vector3f &wo, const Vector3f &N ,const MaterialType &_what)
	{
		switch (_what)
		{
		case MIRCO:
		case DIFFUSE:
		{
			float ans = dotProduct(wo, N);
			if (ans > 0.0f)
			{
				return 0.5f / M_PI;
			}
			return 0.0f;
			break;
		}
		case REFRACT:
		{
			float ans = dotProduct(wo, N);
			if (ans > 0.0)
			{
				return 1.0;
			}
			return 0.0f;

			break;
		}
		case REFLC:
		{
			float ans = dotProduct(wo, N);
			if (ans > 0.0)
			{
				return 1.0;
			}
			return 0.0f;
			break;
		}
		default:
			break;
		}
	}

	Vector3f GetRefracBRDF(const Vector3f& wi, const Vector3f& wo, const Vector3f& N )
	{
		if (dotProduct(wo, N)){return 1.0 / dotProduct(wo, N);}
		return Vector3f(0);
	}

	Vector3f GetDiffuseBRDF(const Vector3f& wi, const Vector3f& wo, const Vector3f& N)
	{
		float ans = dotProduct(wo, N);
		if (ans > 0.0f)
		{
			Vector3f res = 1 / M_PI;
			return res;
		}
		return Vector3f(0.0f);
	}

	Vector3f GetReflectBRDF(const Vector3f& wi, const Vector3f& wo, const Vector3f& N)
	{
		float ans = dotProduct(wo, N);
		if (ans > 0.0f)
		{
			Vector3f res = 1 / ans;
			return res;
		}
		return Vector3f(0.0f);
	}

	Vector3f GetMicroBRDF(const Vector3f& wi, const Vector3f& wo, const Vector3f& N)
	{
		float cosalpha = dotProduct(N, wo); //wo�ǹ۲ⷽ��
		if (cosalpha > 0.0f) {
			Vector3f h = (wi + wo).normalized();
			float F = 0.f;
			fresnel(wi, N, ior, F);
			float down = 4 * fabs(clamp(0.0, 1.0, dotProduct(N, wi)) * clamp(0.0, 1.0, dotProduct(N, wi))) + 0.00001;
			float up = F * G(wi, wo, N, roughness) * D_GGX(h, roughness, N); //F * G * D
			return  (up / down); //����Kd��ʵ������ɫ����Ȼ�������ǵø���������Kd
		}
		else
			return Vector3f(0.0f);
	}

};
#endif

