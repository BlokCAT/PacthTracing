#pragma once

#include "Scene.hpp"

class Renderer
{
public:
	Renderer (){}
	static float Crad(float deg);
	void Render(Scene &scene);

};
