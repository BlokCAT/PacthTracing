#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <fstream>
#include  <omp.h>
#include <thread>
#include <mutex>
#include "Renderer.hpp"

extern char PATH[];
extern int spp;
extern int isThread;
extern bool AntiAliasing;
float Renderer::Crad(float deg)
{
	return deg * M_PI / 180.f;
}


void Renderer::Render( Scene &scene)
{
	const static Vector3f eyes( 0.3 , -0.8 , 8 );
	thread* th = new thread[scene.h];
	int progress = 0;
	int totalPix = scene.w * scene.h;
	std::vector<Vector3f> frbuf(totalPix);
	if (isThread == 1)
	{		
		std::mutex mtx1, mtx2;
		int POINT = totalPix / 50;

		for (int i = 0; i < scene.h; i++)
		{
			th[i] = thread([&, i]()
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

						for (int k = 0; k < spp; k++)
						{
							if (AntiAliasing)
							{
								float AntiAliasingOffset = (eryPixer / 2.0) * (RandomFloat() - 0.5);
								
								x = x  +  AntiAliasingOffset;
								y = y  +  AntiAliasingOffset;
							}		
							Vector3f dir(x, y, 0.6);
							Ray ray(eyes, dir); 
							Vector3f temp = (scene.PathTracing(ray, 0) / spp);
							int idx = (i * scene.w) + j;
							{
								lock_guard<std::mutex> lock(mtx1);
								frbuf[idx] = frbuf[idx] + temp;
							}
							{
								lock_guard<std::mutex> lock(mtx2);
								progress++;

								if (progress % POINT == 0)
									UpdateProgress((float)progress / (float)(scene.h * scene.w) / spp);
							}
						}
					}
				});
		}
		for (int i = 0; i < scene.h; i++)
		{
			th[i].join();
		}

	}
	else
	{
		int idx = 0;
		for (int i = 0; i < scene.h; i++)
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

				for (int k = 0; k < spp; k++)
				{
					if (AntiAliasing)
					{
						float AntiAliasingOffset = (eryPixer / 2) * RandomFloat();
						x = x - (eryPixer / 4.0) + AntiAliasingOffset;
						y = y - (eryPixer / 4.0) + AntiAliasingOffset;
					}
					Vector3f dir(x, y, 1.0);
					Ray ray(eyes, dir);
					frbuf[idx] = frbuf[idx] + (scene.PathTracing(ray, 0) / spp);
				}
				idx++;
			}
			UpdateProgress((float)idx / (float)(scene.h * scene.w));
		}
		cout << "\n\n" << frbuf[230].x;
	}
	

	FILE* fp = fopen(PATH, "wb");
	if (fp == nullptr) {
		std::cerr << "\n\nError opening file: " << PATH << std::endl;
		return;
	}

	(void)fprintf(fp, "P6\n%d %d\n255\n", scene.w, scene.h);
	for (auto i = 0; i < scene.h * scene.w; ++i) {
		static unsigned char color[3];
		color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, frbuf[i].x), 0.4f));
		color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, frbuf[i].y), 0.4f));
		color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, frbuf[i].z), 0.4f));
		fwrite(color, 1, 3, fp);
	}
	fclose(fp);
	if( isThread)delete[] th;
	
}
//
//多线程 ：release   300 * 300 * 32 -- - 4s      debug  300 * 300 * 32---- - 22s
//单线程：release     300 * 300 * 32------ 31s     debug  300 * 300 * 32---- - 大于60s