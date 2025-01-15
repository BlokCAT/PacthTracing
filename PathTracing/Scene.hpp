#pragma once
#ifndef PATHTRACING_SCEN_H
#define PATHTRACING_SCEN_H
#include <vector>
#include "Vector.hpp"
#include "Ray.hpp"
#include "Object.hpp"
#include "Boll.hpp"
#include "BVHStruct.hpp"

enum BuildAccelerationWay
{
	NO , BVH 
};

class Scene
{
public:
	int w = 500, h = 500;
	float fov = 40;
	std::vector< Object* > objs;
	std::vector< Object* > Lightsobjs;
	BuildAccelerationWay method;
	BVHstruct bvh;
	Scene ( int a , int b , BuildAccelerationWay m):w(a),h(b), method(m){}
	void BuildAccl() ;
	Vector3f PathTracing(Ray &ray, int depth) ;
	void Add( Object *t);
	void FindHit(Ray &ray ,  HitPoint &hp) ;
	void sampleLight(HitPoint &hp, float &pdf) ;

};
#endif // PATHTRACING_SCEN_H