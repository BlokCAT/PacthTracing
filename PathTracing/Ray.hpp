#pragma once

#ifndef RAYTRACING_RAY_H
#define RAYTRACING_RAY_H
#include"Vector.hpp"

class Ray
{
public:
	Vector3f pos;
	Vector3f dir;

    Ray ( const Vector3f &p , const Vector3f &d ):pos(p) , dir(d){}
	Vector3f Xt_pos(float t){return pos + (dir*t);}
	
};
#endif