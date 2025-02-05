#pragma once
#ifndef PATHTRACING_SCEN_H
#define PATHTRACING_SCEN_H
#include <vector>
#include <string>
#include "Vector.hpp"
#include "Ray.hpp"
#include "Object.hpp"
#include "Boll.hpp"
#include "BVHStruct.hpp"
#include "Texture.hpp"
enum BuildAccelerationWay
{
	NO , BVH 
};

class Scene
{
public:
	int w = 500, h = 500;
	float fov = 40;
	Texture HDRI;
	bool isHDRI = false;
	std::vector< Object* > objs;
	std::vector< Object* > Lightsobjs;
	BuildAccelerationWay method;
	BVHstruct bvh;
	Scene ( int a , int b , BuildAccelerationWay m , string _HDRIfile):w(a),h(b), method(m)
	{
		HDRI = Texture(_HDRIfile);
		isHDRI = true;
	}
	Scene(int a, int b, BuildAccelerationWay m) :w(a), h(b), method(m)
	{
		isHDRI = false;
	}
	void BuildAccl() ;
	Vector3f PathTracing(Ray &ray, int depth) ;
	void Add( Object *t);
	void FindHit(Ray &ray ,  HitPoint &hp) ;
	void sampleLight(HitPoint &hp, float &pdf) ;
	Vector3f getHDRIcolor(const Vector3f& dir);

};
#endif // PATHTRACING_SCEN_H