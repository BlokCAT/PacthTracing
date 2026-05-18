#pragma once
#ifndef PATHTRACING_REN_H
#define PATHTRACING_REN_H



#include "Scene.hpp"
#include "cuda/Camera.cuh"

class Renderer
{
public:
	Renderer() {}
	float Crad(float deg);
	void Render(Scene &scene, const Camera& cam = Camera());

};
#endif // PATHTRACING_REN_H