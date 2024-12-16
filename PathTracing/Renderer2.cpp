#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "Scene.hpp"
#include "Renderer.hpp"
#include <fstream>
#include <iostream>
#include <omp.h>
#include <cstdio>
#include  <omp.h>

inline float Crad(float deg)
{
	return deg * M_PI / 180.f;
}

void Renderer::Render( Scene &scene)
{
	const static Vector3f eyes(0);
	int idx = 0;
	std::vector<Vector3f> frbuf (scene.w * scene.h);
	
	for (int i = 0; i < scene.h ; i++)
	{
		for (int j = 0; j < scene.w; j++)
		{
			float halfH = std::tan(M_PI / 9.0);
			float eryPixer = halfH / (scene.h / 2.0);
			float x = (float)j * eryPixer + (eryPixer / 2.0);
			float y = (float)i * eryPixer + (eryPixer / 2.0);
			y = 2.0 * halfH - y;
			x = x - halfH;
			y = y - halfH;

			Vector3f dir( x, y, 1.0);
			Ray ray(eyes, dir);

			
			for (int k = 0; k < spp; k++)
			{
				frbuf[idx] = frbuf[idx] + (scene.PathTracing(ray, 0) / spp);
			}
			idx++;
		}
		UpdateProgress((float)idx / (float)(scene.h * scene.w));
	}

	char path[999] = "all/12月2号spp=8多物体NO性能.ppm";
	FILE* fp = fopen(path, "wb");
	(void)fprintf(fp, "P6\n%d %d\n255\n", scene.w, scene.h);
	for (auto i = 0; i < scene.h * scene.w; ++i) {
		static unsigned char color[3];
		color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, frbuf[i].x), 0.4f));
		color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, frbuf[i].y), 0.4f));
		color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, frbuf[i].z), 0.4f));
		fwrite(color, 1, 3, fp);
	}
	fclose(fp);
	
}