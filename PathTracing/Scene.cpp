#pragma once

#include"Scene.hpp"
#include "Tool.hpp"
#include <cstdio>
extern int MAX_RENDER_DEPTH;

void Scene::BuildAccl()
{
	switch (method)
	{
	case NO:
		return;
		break;
	case BVH:
		this->bvh.objects = objs;
		this->bvh.BuiltBVH(1);
		std::cout << "BVH构建完毕！\n";
		break;
	default:
		break;
	}
}

void Scene::sampleLight(HitPoint &hp, float &pdf) 
{
	float gs = RandomFloat();
	if (gs == 1)gs = 0.9;
	int aimidx = gs * Lightsobjs.size();
	Lightsobjs[aimidx]->SampleLight(hp, pdf);
	return;
}



void Scene::FindHit( Ray &ray ,  HitPoint &hp) 
{
	switch (method)
	{
	case NO:
		for (int i = 0; i < objs.size(); i++)
		{
			objs[i]->getHitPoint(ray, hp);
			if (hp.happened == false)continue;
		}
		return;
		break;
	case BVH:
		bvh.getHitposition(ray , hp);
		break;
	default:
		break;
	}
}

void Scene::Add( Object *t) { 
	objs.push_back(t); 
	if (t->m->islight)
	{
		Lightsobjs.push_back(t);
	}
}



//Path Tracing
Vector3f Scene::PathTracing( Ray &ray, int depth)
{

	Vector3f L_dir(0.0), L_indir(0.0);

	if (MAX_RENDER_DEPTH < 998 && depth >= MAX_RENDER_DEPTH)return L_dir;
	
	HitPoint hit_to_scene;
	hit_to_scene.happened = false;
	Scene::FindHit(ray , hit_to_scene);
	if (!hit_to_scene.happened) return Vector3f(0);
	if (hit_to_scene.m->islight) return hit_to_scene.m->lightIntensity;
	
	

	//取出所有光线在场景的第一个交点的信息
	Vector3f hit_pos = hit_to_scene.hitcoord;
	Vector3f N = hit_to_scene.hitN.normalized();
	Material * mat = hit_to_scene.m;
	Vector3f wi = (ray.dir * -1).normalized();

	switch (mat->mtype)
	{
	case REFLC:
	{
		//计算间接光照
		float gs = RandomFloat();
		if (gs < RussianRoulette)
		{
			Vector3f futureDir = mat->GetFutureDir(wi, N);
			Ray newRay(hit_pos, futureDir);
			HitPoint test;
			FindHit(newRay, test);
			if (test.happened )
			{
				Vector3f brdf_ = mat->GetBRDF(wi, futureDir, N);
				float costheta3 = dotProduct(futureDir, N);
				float pdf_ = mat->pdf(wi, futureDir, N);
				if ( pdf_ > 0.0001)
					L_indir = PathTracing(newRay, depth + 1) * brdf_ * costheta3 / pdf_ / RussianRoulette;
			}
		}
		break;
	}
	case MIRCO:
	case DIFFUSE:
	{
		//直接光照，随机采样光源
		float pdf_L = 0;
		HitPoint sample_light;
		Scene::sampleLight(sample_light, pdf_L);

		Vector3f sample_hit = sample_light.hitcoord;
		Vector3f sampleRayDir = (sample_hit - hit_pos).normalized();
		float d = (hit_pos - sample_hit).len();

		Ray sampleRay(hit_pos + (N * 0.01), sampleRayDir);
		HitPoint hit_to_sample;

		Scene::FindHit(sampleRay, hit_to_sample);

		if (hit_to_sample.happened && hit_to_sample.m->islight)
		{
			Vector3f L_i = hit_to_sample.m->lightIntensity;
			Vector3f brdf1 = mat->GetBRDF(wi, sampleRayDir, N);
			float cos_theta1 = clamp(0.0,1.0, dotProduct(N, sampleRayDir));
			float cos_theta2 = clamp(0.0, 1.0, dotProduct(hit_to_sample.hitN, sampleRayDir * -1));
			if (pdf_L > 0.0001)
				L_dir = (L_i * brdf1 * cos_theta1 * cos_theta2 / (d * d)) / pdf_L;
		}

		//计算间接光照
		float gs = RandomFloat();
		if (gs < RussianRoulette)
		{
			
			Vector3f futureDir = mat->GetFutureDir(wi, N);
			Ray newRay(hit_pos + (N * 0.0001), futureDir);
			HitPoint test;
			Scene::FindHit(newRay, test);
			if (test.happened && !test.m->islight)
			{
				Vector3f brdf_ = mat->GetBRDF(wi, futureDir, N);
				float costheta3 = clamp(0.0, 1.0, dotProduct(futureDir, N));
				float pdf_ = mat->pdf(wi, futureDir, N);
				L_indir = PathTracing(newRay, depth + 1) * brdf_ * costheta3 / pdf_ / RussianRoulette;
			}
		}
		break;
	}
	case REFRACT:
	{
		float gs = RandomFloat();
		if( gs < RussianRoulette)
		{
			Vector3f Color_rflcet(0), Color_rfract(0);
			float K = 0.0;
			mat->fresnel(wi, N, mat->ior, K);

			//计算折射颜色
			Vector3f futureDir1 = mat->refract(wi, N , mat->ior);
			Ray newRay1(hit_pos - (N * 0.001), futureDir1);
			HitPoint test1;
			test1.happened = false;
			FindHit(newRay1, test1);
	
			if (test1.happened)
			{
				//取出所有光线在出去的点的信息
				Vector3f hitpos = test1.hitcoord;
				Vector3f nn = test1.hitN.normalized();
				Material* mt = test1.m;
				Vector3f new_wi = (newRay1.dir * -1).normalized();
				Vector3f futureOutDir = mt->refract(new_wi, nn, mt->ior);

				if (!(!futureOutDir.x && !futureOutDir.y && !futureOutDir.z)) {
					//折射后射出去的光线
	
					Ray outRay(hitpos + (nn * 0.001), futureOutDir);
					HitPoint Obj_rfract_hit;
					FindHit(outRay, Obj_rfract_hit);

					if (Obj_rfract_hit.happened)
					{
						Vector3f Brdf_ = mt->GetRefracBRDF(new_wi, futureOutDir, nn , 1);
						float costheta4 = dotProduct(futureOutDir, nn);
						float Pdf_ = mt->pdf(new_wi, futureOutDir, nn);
						if (Pdf_ > 0.0001)
							Color_rfract = PathTracing(outRay, depth + 1) * Brdf_ * costheta4 / Pdf_ / RussianRoulette;
					}
				}
			
			}
			
			//计算折射体表面反射的颜色
			Vector3f futureDir2 = mat->GetFutureDir(wi, N);
			Ray newRay2(hit_pos, futureDir2);
			HitPoint test2;
			FindHit(newRay2, test2);
			if (test2.happened)
			{
				Vector3f brdf_ = mat->GetRefracBRDF(wi, futureDir2, N , 2);
				float costheta5 = dotProduct(futureDir2, N);
				float pdf_ = mat->pdf(wi, futureDir2, N);
				if (pdf_ > 0.0001)
					Color_rflcet = PathTracing(newRay2, depth + 1) * brdf_ * costheta5 / pdf_ / RussianRoulette;
			}
			L_indir = (Color_rfract) * (1 - K) + (Color_rflcet)*K;
		}

		break;

	}
	default:
		break;
	}
	
	Vector3f res = L_dir + L_indir ;
	res.x = clamp(0.0, 1, res.x);
	res.y = clamp(0.0, 1, res.y);
	res.z = clamp(0.0, 1, res.z);
	return res;
}
	


