#pragma once
#ifndef PATHTRACING_REN_H
#define PATHTRACING_REN_H



#include "Scene.hpp"

class Renderer
{
public:
	Renderer() {}
	float Crad(float deg);
	void Render(Scene &scene);

};
#endif // PATHTRACING_REN_H